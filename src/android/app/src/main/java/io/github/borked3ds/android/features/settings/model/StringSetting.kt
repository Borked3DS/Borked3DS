// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.features.settings.model

enum class StringSetting(
    override val key: String,
    override val section: String,
    override val defaultValue: String
) : AbstractStringSetting {
    ANAGLYPH_SHADER_NAME("anaglyph_shader_name", Settings.SECTION_RENDERER, "rendepth (builtin)"),
    INIT_TIME("init_time", Settings.SECTION_SYSTEM, "946731601"),
    CAMERA_INNER_NAME("camera_inner_name", Settings.SECTION_CAMERA, "ndk"),
    CAMERA_INNER_CONFIG("camera_inner_config", Settings.SECTION_CAMERA, "_front"),
    CAMERA_OUTER_LEFT_NAME("camera_outer_left_name", Settings.SECTION_CAMERA, "ndk"),
    CAMERA_OUTER_LEFT_CONFIG("camera_outer_left_config", Settings.SECTION_CAMERA, "_back"),
    CAMERA_OUTER_RIGHT_NAME("camera_outer_right_name", Settings.SECTION_CAMERA, "ndk"),
    CAMERA_OUTER_RIGHT_CONFIG("camera_outer_right_config", Settings.SECTION_CAMERA, "_back"),
    LOG_FILTER("log_filter", Settings.SECTION_DEBUG, "*:Trace"),
    LOG_REGEX_FILTER("log_regex_filter", Settings.SECTION_DEBUG, "");

    override var string: String = defaultValue

    override val valueAsString: String
        get() = string

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
            INIT_TIME,
            CAMERA_INNER_NAME,
            CAMERA_INNER_CONFIG,
            CAMERA_OUTER_LEFT_NAME,
            CAMERA_OUTER_LEFT_CONFIG,
            CAMERA_OUTER_RIGHT_NAME,
            CAMERA_OUTER_RIGHT_CONFIG
        )

        fun from(key: String): StringSetting? = StringSetting.entries.firstOrNull { it.key == key }

        fun clear() = StringSetting.entries.forEach { it.string = it.defaultValue }
    }
}
