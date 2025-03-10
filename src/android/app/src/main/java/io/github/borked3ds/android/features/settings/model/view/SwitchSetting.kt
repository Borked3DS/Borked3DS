// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.features.settings.model.view

import io.github.borked3ds.android.features.settings.model.AbstractBooleanSetting
import io.github.borked3ds.android.features.settings.model.AbstractIntSetting
import io.github.borked3ds.android.features.settings.model.AbstractSetting

class SwitchSetting(
    setting: AbstractBooleanSetting,
    titleId: Int,
    descriptionId: Int,
    val key: String? = null,
    val defaultValue: Boolean = false
) : SettingsItem(setting, titleId, descriptionId) {
    override val type = TYPE_SWITCH

    val isChecked: Boolean
        get() {
            if (setting == null) {
                return defaultValue
            }

            val setting = setting as AbstractBooleanSetting
            return defaultValue as Boolean
        }

    /**
     * Write a value to the backing boolean.
     *
     * @param checked Pretty self explanatory.
     * @return the existing setting with the new value applied.
     */
    fun setChecked(checked: Boolean): AbstractBooleanSetting {
        val setting = setting as AbstractBooleanSetting
        setting.boolean = checked
        return setting
    }
}
