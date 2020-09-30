/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VIEWER_AUTOMATION_ENGINE_H
#define VIEWER_AUTOMATION_ENGINE_H

#include <viewer/AutomationSpec.h>

namespace filament {

class Renderer;
class View;

namespace viewer {

// Iterates through an AutomationSpec, applying settings periodically and exporting screenshots.
//
// Upon construction, an automation engine is given an immutable reference to an AutomationSpec.
// The engine is always in one of two states: running or idle. The running state can be entered
// either immediately (startRunning) or by requesting batch mode (requestBatchMode).
//
// Clients must call tick() after each frame is rendered, which gives the engine an opportunity to
// increment the current test (if enough time has elapsed) and request an asychronous screenshot.
// The time to sleep between tests is configurable and can be set to zero. The engine also waits a
// specified minimum number of frames between tests.
//
// Batch mode is meant for non-interactive applications. In batch mode, the engine defers applying
// the first test case until the client unblocks it via allowRunning(). This is useful when waiting
// for a large model file to become fully loaded. Batch mode also offers a query (shouldClose) that
// is triggered after the last screenshot has been written to disk.
class AutomationEngine {
public:
    AutomationEngine(const AutomationSpec* spec) : mSpec(spec) {}

    // Immediately enters the running state and applies the first test case to the given View.
    // If a settings object is provided, it is modified to reflect the first test case.
    void startRunning(View* view, Settings* settings = nullptr);

    // Requests that the running state be entered and enables batch mode.
    void requestBatchMode();

    // Notifies the engine that time has passed and a new frame has been rendered.
    // This is when settings get applied, screenshots are (optionally) exported, etc.
    // If a settings object is provided, it is potentially modified to reflect the new test case.
    void tick(View* view, Renderer* renderer, float deltaTime, Settings* settings = nullptr);

    // Signals that batch mode can begin. Call this after all meshes and textures finish loading.
    void allowBatchMode() { mBatchModeAllowed = true; }

    // Cancels an in-progress automation session.
    void stopRunning() { mIsRunning = false; }

    // Convenience function that writes out a JSON file to disk.
    static void exportSettings(const Settings& settings, const char* filename);

    struct Options {
        // Minimum time that the engine waits between applying a settings object and subsequently
        // taking a screenshot. After the screenshot is taken, the engine immediately advances to
        // the next test case. Specified in seconds.
        float sleepDuration = 0.2;

        // If true, the tick function writes out a screenshot before advancing to the next test.
        bool exportScreenshots = false;

        // If true, the tick function writes out a settings JSON file before advancing.
        bool exportSettings = false;

        // Similar to sleepDuration, but expressed as a frame count. Both the minimum sleep time
        // and the minimum frame count must be elapsed before the engine advances to the next test.
        int minFrameCount = 2;
    };

    Options getOptions() const { return mOptions; }
    void setOptions(Options options) { mOptions = options; }

    bool isRunning() const { return mIsRunning; }
    size_t currentTest() const { return mCurrentTest; }
    size_t testCount() const { return mSpec->size(); }
    bool shouldClose() const { return mShouldClose; }
    bool isBatchModeEnabled() const { return mBatchModeEnabled; }
    const char* getStatusMessage() const;

private:
    Options mOptions;
    size_t mCurrentTest;
    float mElapsedTime;
    int mElapsedFrames;
    bool mIsRunning = false;
    bool mBatchModeEnabled = false;
    bool mBatchModePending = false;
    bool mShouldClose = false;
    bool mBatchModeAllowed = false;
    Settings mSettings;
    AutomationSpec const * const mSpec = nullptr;

public:
    // For internal use from a screenshot callback.
    void requestClose() { mShouldClose = true; }
};

} // namespace viewer
} // namespace filament

#endif // VIEWER_AUTOMATION_ENGINE_H
