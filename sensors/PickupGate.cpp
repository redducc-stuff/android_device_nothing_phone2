/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "PickupGate"

#include "PickupGate.h"

#include <hardware_legacy/power.h>
#include <log/log.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace sensors {

namespace {

constexpr char kWakelockName[] = "SensorsHAL_PICKUP";
constexpr int64_t kAccelPeriodNs = 20'000'000;      // 50 Hz
constexpr int64_t kSettleNs = 200'000'000;          // look at gravity after the motion
constexpr int64_t kGiveUpNs = 1'500'000'000;        // the accelerometer can be slow to start
constexpr int kMinSamples = 5;
constexpr float kAlpha = 0.3f;                      // low pass on the accelerometer

// Upright enough and not facing away; flat (put down) or face down is not a pick up.
constexpr float kMinY = 3.0f;
constexpr float kMinZ = -7.0f;

}  // namespace

PickupGate& PickupGate::get() {
    static PickupGate gate;
    return gate;
}

void PickupGate::init(int32_t pickupHandle, int32_t accelHandle, Activate activate, Batch batch) {
    std::lock_guard<std::mutex> lock(mLock);
    mPickupHandle = pickupHandle;
    mAccelHandle = accelHandle;
    mActivate = std::move(activate);
    mBatch = std::move(batch);
    if (!mThread.joinable()) {
        mThread = std::thread(&PickupGate::worker, this);
    }
}

void PickupGate::setEmit(Emit emit) {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mEmit) mEmit = std::move(emit);
}

int32_t PickupGate::pickupHandle() {
    std::lock_guard<std::mutex> lock(mLock);
    return mPickupHandle;
}

bool PickupGate::onActivate(int32_t handle, bool enabled) {
    std::unique_lock<std::mutex> lock(mLock);
    if (handle == mAccelHandle) {
        mAccelEnabledByFramework = enabled;
        // Keep the accelerometer on while we still need it.
        return enabled || !mAccelEnabledByUs;
    }
    if (handle == mPickupHandle) {
        mPickupEnabled = enabled;
        if (!enabled && mCollecting) {
            mTiltTimestamp.reset();
            finishLocked(lock);
        }
    }
    return true;
}

bool PickupGate::onEvent(const V2_1::Event& event) {
    std::lock_guard<std::mutex> lock(mLock);
    if (mPickupHandle < 0) return true;

    if (event.sensorHandle == mPickupHandle) {
        if (!mPickupEnabled || event.u.scalar != 1) return false;
        mTiltTimestamp = event.timestamp;
        mDeadlineNs = elapsedRealtimeNano() + kSettleNs;
        if (!mCollecting) {
            mCollecting = true;
            mSamples = 0;
            mGravity[0] = mGravity[1] = mGravity[2] = 0;
            acquire_wake_lock(PARTIAL_WAKE_LOCK, kWakelockName);
            mCv.notify_one();
        }
        return false;
    }

    if (event.sensorHandle == mAccelHandle && mCollecting) {
        const float sample[3] = {event.u.uncal.x, event.u.uncal.y, event.u.uncal.z};
        for (int i = 0; i < 3; i++) {
            mGravity[i] = mSamples == 0 ? sample[i]
                                        : mGravity[i] + kAlpha * (sample[i] - mGravity[i]);
        }
        mSamples++;
        return mAccelEnabledByFramework;
    }

    return true;
}

void PickupGate::worker() {
    std::unique_lock<std::mutex> lock(mLock);
    while (true) {
        mCv.wait(lock, [this] { return mCollecting && !mAccelEnabledByUs; });

        if (!mAccelEnabledByFramework) {
            mAccelEnabledByUs = true;
            auto activate = mActivate;
            auto batch = mBatch;
            int32_t accel = mAccelHandle;
            lock.unlock();
            batch(accel, kAccelPeriodNs);
            activate(accel, true);
            lock.lock();
        }

        // Wait for the motion to settle and enough samples; another tilt pushes the
        // deadline out.
        int64_t giveUp = elapsedRealtimeNano() + kGiveUpNs;
        while (mCollecting) {
            int64_t now = elapsedRealtimeNano();
            if (now >= giveUp || (now >= mDeadlineNs && mSamples >= kMinSamples)) break;
            mCv.wait_for(lock, std::chrono::milliseconds(20));
        }

        if (mCollecting) {
            finishLocked(lock);
        }
    }
}

void PickupGate::finishLocked(std::unique_lock<std::mutex>& lock) {
    bool pickedUp = mTiltTimestamp && mSamples > 0 && mGravity[1] >= kMinY &&
                    mGravity[2] >= kMinZ;
    ALOGD("tilt: gravity %.1f %.1f %.1f (%d samples) -> %s", mGravity[0], mGravity[1],
          mGravity[2], mSamples, pickedUp ? "pick up" : "ignored");

    V2_1::Event event = {};
    if (pickedUp) {
        event.sensorHandle = mPickupHandle;
        event.sensorType = V2_1::SensorType::PICK_UP_GESTURE;
        event.timestamp = *mTiltTimestamp;
        event.u.scalar = 1;
    }

    mCollecting = false;
    mTiltTimestamp.reset();
    // Pick up is one shot: stay off until the framework asks again.
    bool disablePickup = pickedUp;
    if (pickedUp) mPickupEnabled = false;
    bool disableAccel = mAccelEnabledByUs && !mAccelEnabledByFramework;
    mAccelEnabledByUs = false;
    auto activate = mActivate;
    auto emit = mEmit;
    int32_t accel = mAccelHandle;
    int32_t pickup = mPickupHandle;

    lock.unlock();
    if (disableAccel) activate(accel, false);
    if (disablePickup) activate(pickup, false);
    if (pickedUp && emit) emit(event);
    release_wake_lock(kWakelockName);
    lock.lock();
}

}  // namespace sensors
}  // namespace hardware
}  // namespace android
