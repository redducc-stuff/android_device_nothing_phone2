/*
 * SPDX-FileCopyrightText: Paranoid Android
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <android/hardware/sensors/2.1/types.h>

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace android {
namespace hardware {
namespace sensors {

/*
 * The SSC has no pick up gesture, only a tilt detector that fires on any
 * large tilt, putting the phone down included. Turn tilts into pick ups by
 * looking at gravity shortly after the tilt: only report one when the phone
 * ends up upright and facing the user.
 */
class PickupGate {
  public:
    using Activate = std::function<void(int32_t handle, bool enabled)>;
    using Batch = std::function<void(int32_t handle, int64_t periodNs)>;
    using Emit = std::function<void(const V2_1::Event& event)>;

    static PickupGate& get();

    void init(int32_t pickupHandle, int32_t accelHandle, Activate activate, Batch batch);

    // Wake up events can only be posted through a sub HAL callback.
    void setEmit(Emit emit);
    int32_t pickupHandle();

    // Called with the framework's activate() requests. Returns false if the
    // request must not reach the sub HAL.
    bool onActivate(int32_t handle, bool enabled);

    // Filters a sub HAL event. Returns false if it must not reach the framework.
    bool onEvent(const V2_1::Event& event);

  private:
    PickupGate() = default;
    void worker();
    void finishLocked(std::unique_lock<std::mutex>& lock);

    std::mutex mLock;
    std::condition_variable mCv;
    std::thread mThread;

    int32_t mPickupHandle = -1;
    int32_t mAccelHandle = -1;
    Activate mActivate;
    Batch mBatch;
    Emit mEmit;

    bool mPickupEnabled = false;
    bool mAccelEnabledByFramework = false;
    bool mAccelEnabledByUs = false;
    bool mCollecting = false;
    int64_t mDeadlineNs = 0;
    float mGravity[3] = {};
    int mSamples = 0;
    std::optional<int64_t> mTiltTimestamp;
};

}  // namespace sensors
}  // namespace hardware
}  // namespace android
