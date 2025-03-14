// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.features.settings.model

enum class BooleanSetting(
    override val key: String,
    override val section: String,
    override val defaultValue: Boolean
) : AbstractBooleanSetting {
    EXPAND_TO_CUTOUT_AREA("expand_to_cutout_area", Settings.SECTION_LAYOUT, false),
    SUSTAINED_PERFORMANCE("sustained_performance", Settings.SECTION_CORE, false),
    USE_FRAME_LIMIT("use_frame_limit", Settings.SECTION_RENDERER, true),
    HIDE_IMAGES("hide_images", Settings.SECTION_CORE, false),
    NEW_3DS("is_new_3ds", Settings.SECTION_SYSTEM, true),
    LLE_APPLETS("lle_applets", Settings.SECTION_SYSTEM, false),
    PLUGIN_LOADER("plugin_loader", Settings.SECTION_SYSTEM, false),
    ALLOW_PLUGIN_LOADER("allow_plugin_loader", Settings.SECTION_SYSTEM, true),
    USE_VIRTUAL_SD("use_virtual_sd", Settings.SECTION_SYSTEM, true),
    USE_ARTIC_BASE_CONTROLLER("use_artic_base_controller", Settings.SECTION_CONTROLS, false),
    LINEAR_FILTERING("filter_mode", Settings.SECTION_RENDERER, true),
    HW_SHADER("use_hw_shader", Settings.SECTION_RENDERER, true),
    SHADERS_ACCURATE_MUL("shaders_accurate_mul", Settings.SECTION_RENDERER, false),
    SHADER_JIT("use_shader_jit", Settings.SECTION_RENDERER, true),
    ASYNC_SHADERS("async_shader_compilation", Settings.SECTION_RENDERER, false),
    ASYNC_PRESENTATION("async_presentation", Settings.SECTION_RENDERER, true),
    SPIRV_SHADER_GEN("spirv_shader_gen", Settings.SECTION_RENDERER, true),
    SPIRV_OUTPUT_VALIDATION("spirv_output_validation", Settings.SECTION_RENDERER, false),
    SPIRV_OUTPUT_LEGALIZATION("spirv_output_legalization", Settings.SECTION_RENDERER, false),
    GEOMETRY_SHADER("geometry_shader", Settings.SECTION_RENDERER, false),
    RELAXED_PRECISION_DECORATORS("relaxed_precision_decorators", Settings.SECTION_RENDERER, false),
    USE_SAMPLE_SHADING("use_sample_shading", Settings.SECTION_RENDERER, false),
    SKIP_SLOW_DRAW("skip_slow_draw", Settings.SECTION_RENDERER, false),
    SKIP_TEXTURE_COPY("skip_texture_copy", Settings.SECTION_RENDERER, false),
    SKIP_CPU_WRITE("skip_cpu_write", Settings.SECTION_RENDERER, false),
    UPSCALING_HACK("upscaling_hack", Settings.SECTION_RENDERER, false),
    SWAP_EYES_3D("swap_eyes_3d", Settings.SECTION_RENDERER, false),
    ENABLE_CUSTOM_CPU_TICKS("enable_custom_cpu_ticks", Settings.SECTION_CORE, false),
    DISABLE_RIGHT_EYE_RENDER("disable_right_eye_render", Settings.SECTION_RENDERER, true),
    CUSTOM_TEXTURES("custom_textures", Settings.SECTION_UTILITY, false),
    DUMP_TEXTURES("dump_textures", Settings.SECTION_UTILITY, false),
    PRELOAD_TEXTURES("preload_textures", Settings.SECTION_RENDERER, false),
    ASYNC_CUSTOM_LOADING("async_custom_loading", Settings.SECTION_UTILITY, true),
    ADRENO_GPU_BOOST("adreno_gpu_boost", Settings.SECTION_RENDERER, false),
    DISK_SHADER_CACHE("use_disk_shader_cache", Settings.SECTION_RENDERER, true),
    VSYNC("use_vsync_new", Settings.SECTION_RENDERER, true),
    ENABLE_AUDIO_STRETCHING("enable_audio_stretching", Settings.SECTION_AUDIO, true),
    ENABLE_REALTIME_AUDIO("enable_realtime_audio", Settings.SECTION_AUDIO, false),
    CPU_JIT("use_cpu_jit", Settings.SECTION_CORE, true),
    CORE_DOWNCOUNT_HACK("core_downcount_hack", Settings.SECTION_RENDERER, false),
    PRIORITY_BOOST("priority_boost", Settings.SECTION_CORE, false),
    GDB_STUB("use_gdbstub", Settings.SECTION_DEBUG, false),
    DEBUG_RENDERER("renderer_debug", Settings.SECTION_DEBUG, false),
    INSTANT_DEBUG_LOG("instant_debug_log", Settings.SECTION_DEBUG, true),
    UPRIGHT_SCREEN("upright_screen", Settings.SECTION_LAYOUT, false),
    RECORD_FRAME_TIMES("record_frame_times", Settings.SECTION_DEBUG, false),
    DUMP_COMMAND_BUFFERS("dump_command_buffers", Settings.SECTION_DEBUG, false),
    SWAP_SCREEN("swap_screen", Settings.SECTION_LAYOUT, false),
    SHOW_FPS("show_fps", Settings.SECTION_LAYOUT, true),
    SHOW_FRAMETIME("show_frametime", Settings.SECTION_LAYOUT, false),
    SHOW_SPEED("show_speed", Settings.SECTION_LAYOUT, false),
    SHOW_APP_RAM_USAGE("show_app_ram_usage", Settings.SECTION_LAYOUT, false),
    SHOW_SYSTEM_RAM_USAGE("show_system_ram_usage", Settings.SECTION_LAYOUT, false),
    SHOW_BAT_TEMPERATURE("show_bat_temperature", Settings.SECTION_LAYOUT, false),
    OVERLAY_BACKGROUND("overlay_background", Settings.SECTION_LAYOUT, false),
    CUSTOM_LAYOUT("custom_layout", Settings.SECTION_LAYOUT, false);

    override var boolean: Boolean = defaultValue

    override val valueAsString: String
        get() = boolean.toString()

    override val isRuntimeEditable: Boolean
        get() {
            for (setting in NOT_RUNTIME_EDITABLE) {
                if (setting == this) {
                    return false
                }
            }
            return true
        }

    companion object {
        private val NOT_RUNTIME_EDITABLE = listOf(
            SUSTAINED_PERFORMANCE,
            NEW_3DS,
            LLE_APPLETS,
            PLUGIN_LOADER,
            ALLOW_PLUGIN_LOADER,
            USE_ARTIC_BASE_CONTROLLER,
            ASYNC_SHADERS,
            RELAXED_PRECISION_DECORATORS,
            ASYNC_CUSTOM_LOADING,
            ADRENO_GPU_BOOST,
            VSYNC,
            CPU_JIT,
            CORE_DOWNCOUNT_HACK,
            ASYNC_CUSTOM_LOADING,
            SHADERS_ACCURATE_MUL,
            DEBUG_RENDERER
        )

        fun from(key: String): BooleanSetting? =
            BooleanSetting.entries.firstOrNull { it.key == key }

        fun clear() = BooleanSetting.entries.forEach { it.boolean = it.defaultValue }
    }
}
