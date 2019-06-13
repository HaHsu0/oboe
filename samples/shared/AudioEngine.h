/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef SAMPLES_TAPAUDIOENGINE_H
#define SAMPLES_TAPAUDIOENGINE_H


#include <oboe/Oboe.h>
#include "shared/RenderableTap.h"
#include "../debug-utils/logging_macros.h"
#include "AudioEngineBase.h"
#include "DefaultAudioStreamCallback.h"
#include "RenderableTap.h"

// T is a RenderableTap Source, U is a Callback
// The stream is float only
// The engine should be responsible for mutating and restarting the stream, as well as owning the callback

class DefaultAudioStreamCallback;

template <class T, class U = DefaultAudioStreamCallback>
class AudioEngine : AudioEngineBase {
public:

    AudioEngine() {
        LOGD("Creating playback stream");
        AudioEngine<T, U>::createPlaybackStream(this->configureBuilder());
    }
    virtual ~AudioEngine() = default;

    void restartStream() override {
        this->createPlaybackStream(this->configureBuilder());
    }
    // This is for overriding the default config
    void configureRestartStream(oboe::AudioStreamBuilder&& builder) {
        createPlaybackStream(std::move(builder));
    }

    void toggleTone() {
        isToneOn = !isToneOn;
        mSource->setToneOn(isToneOn);
    }
protected:
    // Default config properties of the stream, can be changed
    // These properties will be used on start and restart on disconnect
    virtual oboe::AudioStreamBuilder configureBuilder() {
        oboe::AudioStreamBuilder builder;
        builder.setSampleRate(48000)->setChannelCount(2);
        return builder;
    }
    U* getCallbackPtr() {
        return dynamic_cast<U*>(mCallback.get());
    }

    oboe::ManagedStream mStream;
    std::unique_ptr<RenderableTap> mSource;
    std::unique_ptr<DefaultAudioStreamCallback> mCallback = std::make_unique<U>();
    bool isToneOn = false;

private:
    void createPlaybackStream(oboe::AudioStreamBuilder &&builder) {
        oboe::Result result = builder.setSharingMode(oboe::SharingMode::Exclusive)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setFormat(oboe::AudioFormat::Float)
            ->setCallback(mCallback.get())
            ->openManagedStream(mStream);
        if (result == oboe::Result::OK) {
            mStream->setBufferSizeInFrames(mStream->getFramesPerBurst() * 2);
            mSource = std::make_unique<T>(mStream->getSampleRate(),
                                      mStream->getChannelCount());
            mCallback->setCallbackSource(mSource.get());
            mCallback->setEnginePtr(this);
            mCallback->onSetupComplete();
            auto startResult = mStream->requestStart();
            if (startResult != oboe::Result::OK) {
                LOGE("Error starting stream. %s", oboe::convertToText(result));
            }
        } else {
            LOGE("Error starting stream. Error: %s", oboe::convertToText(result));
        }
    }
};

#endif //SAMPLES_TAPAUDIOENGINE_H