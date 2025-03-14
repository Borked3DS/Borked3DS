// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.display

import android.app.Activity
import android.content.Context
import android.view.Surface
import android.view.WindowManager
import io.github.borked3ds.android.NativeLibrary
import io.github.borked3ds.android.R
import io.github.borked3ds.android.features.settings.model.BooleanSetting
import io.github.borked3ds.android.features.settings.model.IntSetting
import io.github.borked3ds.android.features.settings.model.Settings
import io.github.borked3ds.android.features.settings.utils.SettingsFile
import io.github.borked3ds.android.utils.EmulationMenuSettings

class ScreenAdjustmentUtil(
    private val context: Context,
    private val windowManager: WindowManager,
    private val settings: Settings,
) {
    fun WindowManager.getDisplayRotation(context: Context): Int {
        return if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.R) {
            context.display?.rotation ?: Surface.ROTATION_0
        } else {
            @Suppress("DEPRECATION")
            defaultDisplay?.rotation ?: Surface.ROTATION_0
        }
    }

    fun swapScreen() {
        val isEnabled = !EmulationMenuSettings.swapScreens
        EmulationMenuSettings.swapScreens = isEnabled

        NativeLibrary.swapScreens(isEnabled, windowManager.getDisplayRotation(context))
        BooleanSetting.SWAP_SCREEN.boolean = isEnabled
        settings.saveSetting(BooleanSetting.SWAP_SCREEN, SettingsFile.FILE_NAME_CONFIG)
    }

    fun cycleLayouts() {
        val landscapeValues = context.resources?.getIntArray(R.array.landscapeValues) ?: return
        val portraitValues = context.resources?.getIntArray(R.array.portraitValues) ?: return

        if (NativeLibrary.isPortraitMode) {
            val currentLayout = IntSetting.PORTRAIT_SCREEN_LAYOUT.int ?: return
            val pos = portraitValues.indexOf(currentLayout)
            if (pos != -1) {
                val layoutOption = portraitValues[(pos + 1) % portraitValues.size]
                changePortraitOrientation(layoutOption)
            }
        } else {
            val currentLayout = IntSetting.SCREEN_LAYOUT.int ?: return
            val pos = landscapeValues.indexOf(currentLayout)
            if (pos != -1) {
                val layoutOption = landscapeValues[(pos + 1) % landscapeValues.size]
                changeScreenOrientation(layoutOption)
            }
        }
    }

    fun changePortraitOrientation(layoutOption: Int) {
        IntSetting.PORTRAIT_SCREEN_LAYOUT.int = layoutOption
        settings.saveSetting(IntSetting.PORTRAIT_SCREEN_LAYOUT, SettingsFile.FILE_NAME_CONFIG)
        NativeLibrary.reloadSettings()
        NativeLibrary.updateFramebuffer(NativeLibrary.isPortraitMode)
    }

    fun changeScreenOrientation(layoutOption: Int) {
        IntSetting.SCREEN_LAYOUT.int = layoutOption
        settings.saveSetting(IntSetting.SCREEN_LAYOUT, SettingsFile.FILE_NAME_CONFIG)
        NativeLibrary.reloadSettings()
        NativeLibrary.updateFramebuffer(NativeLibrary.isPortraitMode)
    }

    fun changeActivityOrientation(orientationOption: Int) {
        val activity = context as? Activity ?: return
        IntSetting.ORIENTATION_OPTION.int = orientationOption
        settings.saveSetting(IntSetting.ORIENTATION_OPTION, SettingsFile.FILE_NAME_CONFIG)
        activity.requestedOrientation = orientationOption
    }

    fun toggleScreenUpright() {
        val uprightBoolean = BooleanSetting.UPRIGHT_SCREEN.boolean
        BooleanSetting.UPRIGHT_SCREEN.boolean = !uprightBoolean
        settings.saveSetting(BooleanSetting.UPRIGHT_SCREEN, SettingsFile.FILE_NAME_CONFIG)
        NativeLibrary.reloadSettings()
        NativeLibrary.updateFramebuffer(NativeLibrary.isPortraitMode)
    }

    companion object {
        private const val TAG = "ScreenAdjustmentUtil"
    }
}
