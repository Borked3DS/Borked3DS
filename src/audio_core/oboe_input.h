// Copyright 2025 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <oboe/Oboe.h>
#include "audio_core/input.h"

namespace AudioCore {

class OboeInput final : public Input {
public:
    explicit OboeInput(std::string device_id);
    ~OboeInput() override;

    void StartSampling(const InputParameters& params) override;
    void StopSampling() override;
    bool IsSampling() override;
    void AdjustSampleRate(u32 sample_rate) override;
    Samples Read() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    std::string device_id;
};

std::vector<std::string> ListOboeInputDevices();

} // namespace AudioCore
