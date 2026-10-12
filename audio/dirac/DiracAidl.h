/*
 * Copyright (C) 2026 The PenguinOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <hardware/audio_effect.h>

#include "effect-impl/EffectContext.h"
#include "effect-impl/EffectImpl.h"

namespace aidl::qti::effects {

class DiracContext final : public EffectContext {
  public:
    DiracContext(const Parameter::Common& common, bool processData);
    ~DiracContext();

    bool valid() const { return mDesc != nullptr; }
    void enable();
    void disable();
    void release();
    RetCode setCommon(const Parameter::Common& common) override;
    RetCode setOutputDevice(const std::vector<aidl::android::media::audio::common::AudioDeviceDescription>&
                                    devices) override;
    RetCode setVolumeStereo(const Parameter::VolumeStereo& volume) override;
    RetCode setParam(const std::vector<uint8_t>& bytes);
    RetCode getParam(std::vector<uint8_t>& bytes);
    IEffect::Status process(float* in, float* out, int samples);

  private:
    bool acquireLocked();
    void releaseLocked();
    RetCode commandLocked(uint32_t cmd, uint32_t size = 0, void* data = nullptr);
    RetCode configureLocked();
    void applyDeviceLocked();
    void applyVolumeLocked();

    void* mLib = nullptr;
    audio_effect_library_t* mDesc = nullptr;
    effect_handle_t mHandle = nullptr;
    size_t mChannels = 2;
    bool mBypass = false;
};

class DiracAidl final : public EffectImpl {
  public:
    static const std::string kEffectName;
    static const Descriptor kDescriptor;

    DiracAidl() {
        mDescriptor = &kDescriptor;
        mEffectName = &kEffectName;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* desc) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;
    ndk::ScopedAStatus commandImpl(CommandId id) override;
    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common,
                                                 bool processData) override;
    RetCode releaseContext() override;
    IEffect::Status effectProcessImpl(float* in, float* out, int samples) override;
    std::string getEffectName() override { return kEffectName; }

  private:
    std::shared_ptr<DiracContext> mContext;
};

}  // namespace aidl::qti::effects
