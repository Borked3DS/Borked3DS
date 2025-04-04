// Copyright 2016 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <span>
#include <QString>
#include <QWidget>

namespace Ui {
class ConfigureGraphics;
}

namespace ConfigurationShared {
enum class CheckState;
}

class ConfigureGraphics : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGraphics(QString gl_renderer, std::span<const QString> physical_devices,
                               bool is_powered_on, QWidget* parent = nullptr);
    ~ConfigureGraphics() override;

    void ApplyConfiguration();
    void RetranslateUI();
    void SetConfiguration();

    void UpdateBackgroundColorButton(const QColor& color);

private:
    void SetupPerGameUI();
    void SetPhysicalDeviceComboVisibility(int index);

    ConfigurationShared::CheckState use_gles;
    ConfigurationShared::CheckState use_hw_shader;
    ConfigurationShared::CheckState shaders_accurate_mul;
    ConfigurationShared::CheckState skip_slow_draw;
    ConfigurationShared::CheckState skip_texture_copy;
    ConfigurationShared::CheckState skip_cpu_write;
    ConfigurationShared::CheckState upscaling_hack;
    ConfigurationShared::CheckState use_disk_shader_cache;
    ConfigurationShared::CheckState use_vsync_new;
    ConfigurationShared::CheckState async_shader_compilation;
    ConfigurationShared::CheckState core_downcount_hack;
    ConfigurationShared::CheckState async_presentation;
    ConfigurationShared::CheckState spirv_shader_gen;
    ConfigurationShared::CheckState geometry_shader;
    ConfigurationShared::CheckState sample_shading;
    ConfigurationShared::CheckState relaxed_precision_decorators;
    ConfigurationShared::CheckState optimize_spirv_output;
    ConfigurationShared::CheckState spirv_output_validation;
    ConfigurationShared::CheckState spirv_output_legalization;
    std::unique_ptr<Ui::ConfigureGraphics> ui;
    QColor bg_color;
};
