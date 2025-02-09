// Copyright 2025 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "audio_core/oboe_input.h"
#include "audio_core/sink.h" // For auto_device_name
#include "common/logging/log.h"
#include "common/threadsafe_queue.h"

namespace AudioCore {

using SampleQueue = Common::SPSCQueue<Samples>;

struct OboeInput::Impl : public oboe::AudioStreamDataCallback,
                         public oboe::AudioStreamErrorCallback {
    oboe::AudioStream* stream = nullptr;
    SampleQueue sample_queue{};
    u8 sample_size_in_bytes = 0;
    InputParameters current_params{};

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream, void* audioData,
                                          int32_t numFrames) override {
        if (!audioData || numFrames <= 0) {
            return oboe::DataCallbackResult::Continue;
        }

        const auto* inputBuffer = static_cast<const int16_t*>(audioData);
        std::vector<u8> samples;

        if (sample_size_in_bytes == 1) {
            samples.reserve(numFrames);
            for (int i = 0; i < numFrames; ++i) {
                samples.push_back(
                    static_cast<u8>((static_cast<uint16_t>(inputBuffer[i]) >> 8) & 0xFF));
            }
        } else {
            const auto* data = reinterpret_cast<const u8*>(inputBuffer);
            samples.insert(samples.end(), data, data + numFrames * sample_size_in_bytes);
        }

        sample_queue.Push(samples);
        return oboe::DataCallbackResult::Continue;
    }

    void onErrorAfterClose(oboe::AudioStream* stream, oboe::Result error) override {
        if (error == oboe::Result::ErrorDisconnected) {
            LOG_WARNING(Audio, "Oboe input stream disconnected.");
        }
    }
};

OboeInput::OboeInput(std::string device_id)
    : impl(std::make_unique<Impl>()), device_id(std::move(device_id)) {}

OboeInput::~OboeInput() {
    StopSampling();
}

void OboeInput::StartSampling(const InputParameters& params) {
    if (IsSampling()) {
        return;
    }

    if (params.sign == Signedness::Unsigned) {
        LOG_WARNING(
            Audio,
            "Application requested unsupported unsigned PCM format. Falling back to signed.");
    }

    impl->current_params = params;
    impl->sample_size_in_bytes = params.sample_size / 8;

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Input)
        ->setSharingMode(oboe::SharingMode::Exclusive)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setAudioApi(oboe::AudioApi::Unspecified)
        ->setFormat(oboe::AudioFormat::I16)
        ->setChannelCount(oboe::ChannelCount::Mono)
        ->setSampleRate(params.sample_rate)
        ->setDataCallback(impl.get())
        ->setErrorCallback(impl.get());

    // Oboe doesn't support named device selection - use default device
    if (device_id != auto_device_name && !device_id.empty()) {
        LOG_WARNING(Audio, "Oboe input doesn't support specific device selection - using default");
    }

    oboe::Result result = builder.openStream(&impl->stream);
    if (result != oboe::Result::OK || !impl->stream) {
        LOG_CRITICAL(Audio, "Failed to open Oboe input stream: {}", static_cast<int>(result));
        StopSampling();
        return;
    }

    result = impl->stream->requestStart();
    if (result != oboe::Result::OK) {
        LOG_CRITICAL(Audio, "Failed to start Oboe input stream: {}", static_cast<int>(result));
        StopSampling();
        return;
    }
}

void OboeInput::StopSampling() {
    if (impl->stream) {
        impl->stream->stop();
        impl->stream->close();
        impl->stream = nullptr;
    }
}

bool OboeInput::IsSampling() {
    return impl->stream && impl->stream->getState() == oboe::StreamState::Started;
}

void OboeInput::AdjustSampleRate(u32 sample_rate) {
    if (!IsSampling()) {
        return;
    }

    auto new_params = impl->current_params;
    new_params.sample_rate = sample_rate;
    StopSampling();
    StartSampling(new_params);
}

Samples OboeInput::Read() {
    if (!IsSampling()) {
        return {};
    }

    Samples samples{};
    Samples queue;
    while (impl->sample_queue.Pop(queue)) {
        samples.insert(samples.end(), queue.begin(), queue.end());
    }
    return samples;
}

std::vector<std::string> ListOboeInputDevices() {
    // Oboe doesn't support enumerating input devices by name
    return {auto_device_name};
}

} // namespace AudioCore
