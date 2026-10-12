/*
 * Copyright (C) 2026 The PenguinOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "AHAL_Effect_DiracAidl"

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
#include <mutex>

#include <aidl/android/hardware/audio/effect/DefaultExtension.h>
#include <android-base/logging.h>
#include <system/audio.h>

#include "DiracAidl.h"

using aidl::android::hardware::audio::effect::DefaultExtension;
using aidl::android::hardware::audio::effect::Descriptor;
using aidl::android::hardware::audio::effect::Flags;
using aidl::android::hardware::audio::effect::IEffect;
using aidl::android::hardware::audio::effect::VendorExtension;
using aidl::android::media::audio::common::AudioDeviceDescription;
using aidl::android::media::audio::common::AudioDeviceType;
using aidl::android::media::audio::common::AudioUuid;
using aidl::qti::effects::DiracAidl;

namespace {

constexpr char kLibrary[] = "/vendor/lib64/soundfx/libdiraceffect_NT.so";

constexpr effect_uuid_t kLegacyUuid = {
        0xae737c63, 0xf2c0, 0x5457, 0x909e, {0x1e, 0x94, 0x0c, 0x91, 0xb6, 0x7b}};

AudioUuid toAidl(const effect_uuid_t& u) {
    return {.timeLow = static_cast<int32_t>(u.timeLow),
            .timeMid = u.timeMid,
            .timeHiAndVersion = u.timeHiAndVersion,
            .clockSeq = u.clockSeq,
            .node = std::vector<uint8_t>(u.node, u.node + 6)};
}

const AudioUuid kTypeUuid = toAidl(
        {0xec7178ec, 0xe5e1, 0x4432, 0xa3f4, {0x46, 0x57, 0xe6, 0x79, 0x52, 0x10}});
const AudioUuid kImplUuid = toAidl(kLegacyUuid);

audio_devices_t toLegacy(const AudioDeviceDescription& d) {
    switch (d.type) {
        case AudioDeviceType::OUT_SPEAKER:
            return AUDIO_DEVICE_OUT_SPEAKER;
        case AudioDeviceType::OUT_SPEAKER_EARPIECE:
            return AUDIO_DEVICE_OUT_EARPIECE;
        case AudioDeviceType::OUT_HEADSET:
            if (d.connection == AudioDeviceDescription::CONNECTION_BT_SCO)
                return AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET;
            if (d.connection == AudioDeviceDescription::CONNECTION_USB)
                return AUDIO_DEVICE_OUT_USB_HEADSET;
            if (d.connection == AudioDeviceDescription::CONNECTION_BT_LE)
                return AUDIO_DEVICE_OUT_BLE_HEADSET;
            return AUDIO_DEVICE_OUT_WIRED_HEADSET;
        case AudioDeviceType::OUT_HEADPHONE:
            if (d.connection == AudioDeviceDescription::CONNECTION_BT_A2DP)
                return AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES;
            return AUDIO_DEVICE_OUT_WIRED_HEADPHONE;
        case AudioDeviceType::OUT_DEVICE:
            if (d.connection == AudioDeviceDescription::CONNECTION_BT_A2DP)
                return AUDIO_DEVICE_OUT_BLUETOOTH_A2DP;
            if (d.connection == AudioDeviceDescription::CONNECTION_USB)
                return AUDIO_DEVICE_OUT_USB_DEVICE;
            return AUDIO_DEVICE_NONE;
        default:
            return AUDIO_DEVICE_NONE;
    }
}

}  // namespace

extern "C" binder_exception_t createEffect(const AudioUuid* uuid,
                                           std::shared_ptr<IEffect>* instanceSpp) {
    if (uuid == nullptr || *uuid != kImplUuid || instanceSpp == nullptr) {
        return EX_ILLEGAL_ARGUMENT;
    }
    *instanceSpp = ndk::SharedRefBase::make<DiracAidl>();
    return EX_NONE;
}

extern "C" binder_exception_t queryEffect(const AudioUuid* uuid, Descriptor* desc) {
    if (uuid == nullptr || *uuid != kImplUuid || desc == nullptr) {
        return EX_ILLEGAL_ARGUMENT;
    }
    *desc = DiracAidl::kDescriptor;
    return EX_NONE;
}

namespace aidl::qti::effects {

const std::string DiracAidl::kEffectName = "DiracSound";

const Descriptor DiracAidl::kDescriptor = {
        .common = {.id = {.type = kTypeUuid, .uuid = kImplUuid, .proxy = std::nullopt},
                   .flags = {.type = Flags::Type::INSERT,
                             .volume = Flags::Volume::IND,
                             .deviceIndication = true},
                   .name = DiracAidl::kEffectName,
                   .implementor = "Dirac Audio Effect"}};

static std::mutex gDiracLock;
static DiracContext* gDiracOwner = nullptr;

DiracContext::DiracContext(const Parameter::Common& common, bool processData)
    : EffectContext(common, processData) {
    mLib = dlopen(kLibrary, RTLD_NOW);
    if (mLib == nullptr) {
        LOG(ERROR) << "dlopen " << kLibrary << ": " << dlerror();
        return;
    }
    mDesc = static_cast<audio_effect_library_t*>(dlsym(mLib, AUDIO_EFFECT_LIBRARY_INFO_SYM_AS_STR));
    mChannels = getChannelCount(common.input.base.channelMask);
}

DiracContext::~DiracContext() {
    release();
    if (mLib != nullptr) dlclose(mLib);
}

bool DiracContext::acquireLocked() {
    if (mDesc == nullptr) return false;
    if (gDiracOwner != nullptr && gDiracOwner != this) return false;
    if (mHandle != nullptr) return true;
    if (mDesc->create_effect(&kLegacyUuid, mCommon.session, mCommon.ioHandle, &mHandle) != 0) {
        mHandle = nullptr;
        return false;
    }
    gDiracOwner = this;
    if (commandLocked(EFFECT_CMD_INIT) != RetCode::SUCCESS ||
        configureLocked() != RetCode::SUCCESS) {
        releaseLocked();
        return false;
    }
    applyDeviceLocked();
    applyVolumeLocked();
    return true;
}

void DiracContext::releaseLocked() {
    if (mHandle != nullptr) {
        mDesc->release_effect(mHandle);
        mHandle = nullptr;
    }
    if (gDiracOwner == this) gDiracOwner = nullptr;
}

void DiracContext::release() {
    std::lock_guard lock(gDiracLock);
    releaseLocked();
}

void DiracContext::enable() {
    std::lock_guard lock(gDiracLock);
    if (acquireLocked()) commandLocked(EFFECT_CMD_ENABLE);
}

void DiracContext::disable() {
    std::lock_guard lock(gDiracLock);
    if (mHandle == nullptr) return;
    commandLocked(EFFECT_CMD_DISABLE);
    releaseLocked();
}

RetCode DiracContext::commandLocked(uint32_t cmd, uint32_t size, void* data) {
    if (mHandle == nullptr) return RetCode::ERROR_ILLEGAL_PARAMETER;
    int32_t reply = 0;
    uint32_t replySize = sizeof(reply);
    int ret = (*mHandle)->command(mHandle, cmd, size, data, &replySize, &reply);
    if (ret != 0 || reply != 0) {
        LOG(ERROR) << "command " << cmd << " failed: " << ret << "/" << reply;
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    return RetCode::SUCCESS;
}

RetCode DiracContext::configureLocked() {
    effect_config_t config = {};
    for (buffer_config_t* cfg : {&config.inputCfg, &config.outputCfg}) {
        cfg->samplingRate = mCommon.input.base.sampleRate;
        cfg->channels = audio_channel_out_mask_from_count(mChannels);
        cfg->format = AUDIO_FORMAT_PCM_FLOAT;
        cfg->accessMode = EFFECT_BUFFER_ACCESS_WRITE;
        cfg->mask = EFFECT_CONFIG_ALL;
    }
    config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    return commandLocked(EFFECT_CMD_SET_CONFIG, sizeof(config), &config);
}

void DiracContext::applyDeviceLocked() {
    uint32_t device = AUDIO_DEVICE_NONE;
    for (const auto& d : mOutputDevice) device |= toLegacy(d);
    uint32_t replySize = 0;
    (*mHandle)->command(mHandle, EFFECT_CMD_SET_DEVICE, sizeof(device), &device, &replySize,
                        nullptr);
}

void DiracContext::applyVolumeLocked() {
    uint32_t vol[2] = {
            static_cast<uint32_t>(std::clamp(mVolumeStereo.left, 0.f, 1.f) * (1 << 24)),
            static_cast<uint32_t>(std::clamp(mVolumeStereo.right, 0.f, 1.f) * (1 << 24))};
    uint32_t reply[2] = {vol[0], vol[1]};
    uint32_t replySize = sizeof(reply);
    (*mHandle)->command(mHandle, EFFECT_CMD_SET_VOLUME, sizeof(vol), vol, &replySize, reply);
}

RetCode DiracContext::setCommon(const Parameter::Common& common) {
    std::lock_guard lock(gDiracLock);
    RetCode ret = EffectContext::setCommon(common);
    if (ret != RetCode::SUCCESS) return ret;
    mChannels = getChannelCount(common.input.base.channelMask);
    if (mHandle != nullptr) configureLocked();
    return RetCode::SUCCESS;
}

RetCode DiracContext::setOutputDevice(const std::vector<AudioDeviceDescription>& devices) {
    std::lock_guard lock(gDiracLock);
    mOutputDevice = devices;
    mBypass = std::any_of(devices.begin(), devices.end(), [](const auto& d) {
        return d.type != AudioDeviceType::OUT_SPEAKER;
    });
    if (mHandle != nullptr) applyDeviceLocked();
    return RetCode::SUCCESS;
}

RetCode DiracContext::setVolumeStereo(const Parameter::VolumeStereo& volume) {
    std::lock_guard lock(gDiracLock);
    mVolumeStereo = volume;
    if (mHandle != nullptr) applyVolumeLocked();
    return RetCode::SUCCESS;
}

RetCode DiracContext::setParam(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < sizeof(effect_param_t)) return RetCode::ERROR_ILLEGAL_PARAMETER;
    std::lock_guard lock(gDiracLock);
    if (mHandle == nullptr) return RetCode::SUCCESS;
    std::vector<uint8_t> data(bytes);
    return commandLocked(EFFECT_CMD_SET_PARAM, data.size(), data.data());
}

RetCode DiracContext::getParam(std::vector<uint8_t>& bytes) {
    std::lock_guard lock(gDiracLock);
    if (mHandle == nullptr || bytes.size() < sizeof(effect_param_t)) {
        return RetCode::ERROR_ILLEGAL_PARAMETER;
    }
    auto* request = reinterpret_cast<effect_param_t*>(bytes.data());
    uint32_t paddedPSize = ((request->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
    uint32_t cmdSize = sizeof(effect_param_t) + request->psize;
    std::vector<uint8_t> reply(std::max<size_t>(bytes.size(),
                                                sizeof(effect_param_t) + paddedPSize +
                                                        request->vsize));
    uint32_t replySize = reply.size();
    int ret = (*mHandle)->command(mHandle, EFFECT_CMD_GET_PARAM, cmdSize, bytes.data(),
                                  &replySize, reply.data());
    if (ret != 0) return RetCode::ERROR_ILLEGAL_PARAMETER;
    reply.resize(std::max<size_t>(replySize, sizeof(effect_param_t)));
    bytes = std::move(reply);
    return RetCode::SUCCESS;
}

IEffect::Status DiracContext::process(float* in, float* out, int samples) {
    std::lock_guard lock(gDiracLock);
    if (mHandle != nullptr && gDiracOwner == this && mChannels > 0 && !mBypass) {
        audio_buffer_t inBuf = {.frameCount = samples / mChannels, .f32 = in};
        audio_buffer_t outBuf = {.frameCount = samples / mChannels, .f32 = out};
        int ret = (*mHandle)->process(mHandle, &inBuf, &outBuf);
        if (ret == 0) return {STATUS_OK, samples, samples};
    }
    if (in != out) std::copy(in, in + samples, out);
    return {STATUS_OK, samples, samples};
}

ndk::ScopedAStatus DiracAidl::getDescriptor(Descriptor* desc) {
    *desc = kDescriptor;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus DiracAidl::setParameterSpecific(const Parameter::Specific& specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    RETURN_IF(specific.getTag() != Parameter::Specific::vendorEffect, EX_ILLEGAL_ARGUMENT,
              "unsupportedTag");
    std::optional<DefaultExtension> ext;
    specific.get<Parameter::Specific::vendorEffect>().extension.getParcelable(&ext);
    RETURN_IF(!ext.has_value(), EX_ILLEGAL_ARGUMENT, "noExtension");
    RETURN_IF(mContext->setParam(ext->bytes) != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT,
              "setParamFailed");
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus DiracAidl::getParameterSpecific(const Parameter::Id& id,
                                                   Parameter::Specific* specific) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    RETURN_IF(id.getTag() != Parameter::Id::vendorEffectTag, EX_ILLEGAL_ARGUMENT,
              "unsupportedTag");
    std::optional<DefaultExtension> ext;
    id.get<Parameter::Id::vendorEffectTag>().extension.getParcelable(&ext);
    RETURN_IF(!ext.has_value(), EX_ILLEGAL_ARGUMENT, "noExtension");
    RETURN_IF(mContext->getParam(ext->bytes) != RetCode::SUCCESS, EX_ILLEGAL_ARGUMENT,
              "getParamFailed");
    VendorExtension out;
    out.extension.setParcelable(*ext);
    *specific = Parameter::Specific::make<Parameter::Specific::vendorEffect>(out);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus DiracAidl::commandImpl(CommandId id) {
    RETURN_IF(!mContext, EX_NULL_POINTER, "nullContext");
    switch (id) {
        case CommandId::START:
            mContext->enable();
            break;
        case CommandId::STOP:
            mContext->disable();
            break;
        case CommandId::RESET:
            mContext->resetBuffer();
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                    "commandIdNotSupported");
    }
    return ndk::ScopedAStatus::ok();
}

std::shared_ptr<EffectContext> DiracAidl::createContext(const Parameter::Common& common,
                                                        bool processData) {
    if (!mContext) {
        mContext = std::make_shared<DiracContext>(common, processData);
        if (!mContext->valid()) LOG(ERROR) << "Dirac unavailable, passing audio through";
    }
    return mContext;
}

RetCode DiracAidl::releaseContext() {
    if (mContext) mContext->release();
    mContext.reset();
    return RetCode::SUCCESS;
}

IEffect::Status DiracAidl::effectProcessImpl(float* in, float* out, int samples) {
    RETURN_VALUE_IF(!mContext, (IEffect::Status{EX_NULL_POINTER, 0, 0}), "nullContext");
    return mContext->process(in, out, samples);
}

}  // namespace aidl::qti::effects
