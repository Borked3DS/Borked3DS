// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.utils

import android.content.SharedPreferences
import android.content.res.Configuration
import android.graphics.Color
import androidx.annotation.ColorInt
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.view.WindowInsetsControllerCompat
import androidx.preference.PreferenceManager
import io.github.borked3ds.android.Borked3DSApplication
import io.github.borked3ds.android.R
import io.github.borked3ds.android.features.settings.model.Settings
import io.github.borked3ds.android.ui.main.ThemeProvider
import kotlin.math.roundToInt

@Suppress("DEPRECATION")
object ThemeUtil {
    const val SYSTEM_BAR_ALPHA = 0.9f

    private val preferences: SharedPreferences
        get() =
            PreferenceManager.getDefaultSharedPreferences(Borked3DSApplication.appContext)

    private fun getSelectedStaticThemeColor(): Int {
        val themeIndex = preferences.getInt(Settings.PREF_STATIC_THEME_COLOR, 0)
        val themes = arrayOf(
            R.style.Theme_Borked3DS_Blue,
            R.style.Theme_Borked3DS_Cyan,
            R.style.Theme_Borked3DS_Red,
            R.style.Theme_Borked3DS_Green,
            R.style.Theme_Borked3DS_Lime,
            R.style.Theme_Borked3DS_Yellow,
            R.style.Theme_Borked3DS_Orange,
            R.style.Theme_Borked3DS_Violet,
            R.style.Theme_Borked3DS_Pink,
            R.style.Theme_Borked3DS_Gray
        )
        return themes[themeIndex]
    }

    private fun getThemeMode(activity: AppCompatActivity): Int {
        return PreferenceManager.getDefaultSharedPreferences(activity.applicationContext)
            .getInt(Settings.PREF_THEME_MODE, AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM)
    }

    fun setTheme(activity: AppCompatActivity) {
        setThemeMode(activity)
        if (preferences.getBoolean(Settings.PREF_MATERIAL_YOU, false)) {
            activity.setTheme(R.style.Theme_Borked3DS_Main_MaterialYou)
        } else {
            activity.setTheme(getSelectedStaticThemeColor())
        }

        // Using a specific night mode check because this could apply incorrectly when using the
        // light app mode, dark system mode, and black backgrounds. Launching the settings activity
        // will then show light mode colors/navigation bars but with black backgrounds.
        if (preferences.getBoolean(Settings.PREF_BLACK_BACKGROUNDS, false) &&
            isNightMode(activity)
        ) {
            activity.setTheme(R.style.ThemeOverlay_Borked3DS_Dark)
        }
    }

    fun setThemeMode(activity: AppCompatActivity) {
        val themeMode = getThemeMode(activity)
        activity.delegate.localNightMode = themeMode
        configureSystemBars(activity, themeMode)
    }

    private fun configureSystemBars(activity: AppCompatActivity, themeMode: Int) {
        val nightMode = when (themeMode) {
            AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM -> isNightMode(activity)
            AppCompatDelegate.MODE_NIGHT_YES -> true
            else -> false
        }

        val window = activity.window
        val decorView = window.decorView

        // Use WindowInsetsControllerCompat for all Android versions
        val windowInsetsController = WindowInsetsControllerCompat(window, decorView)

        // Configure both status bar and navigation bar appearances
        windowInsetsController.apply {
            isAppearanceLightStatusBars = !nightMode
            isAppearanceLightNavigationBars = !nightMode
        }
    }

    internal fun isNightMode(activity: AppCompatActivity): Boolean {
        return when (activity.resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK) {
            Configuration.UI_MODE_NIGHT_NO -> false
            Configuration.UI_MODE_NIGHT_YES -> true
            else -> false
        }
    }

    fun setCorrectTheme(activity: AppCompatActivity) {
        val currentTheme = (activity as ThemeProvider).themeId
        setTheme(activity)
        if (currentTheme != (activity as ThemeProvider).themeId) {
            activity.recreate()
        }
    }

    @ColorInt
    fun getColorWithOpacity(@ColorInt color: Int, alphaFactor: Float): Int {
        return Color.argb(
            (alphaFactor * Color.alpha(color)).roundToInt(),
            Color.red(color),
            Color.green(color),
            Color.blue(color)
        )
    }

    // Listener that detects if the theme keys are being changed from the setting menu and recreates the activity
    private var listener: SharedPreferences.OnSharedPreferenceChangeListener? = null

    fun ThemeChangeListener(activity: AppCompatActivity) {
        listener = SharedPreferences.OnSharedPreferenceChangeListener { _, key ->
            val relevantKeys = listOf(
                Settings.PREF_STATIC_THEME_COLOR,
                Settings.PREF_MATERIAL_YOU,
                Settings.PREF_BLACK_BACKGROUNDS
            )
            if (key in relevantKeys) {
                activity.recreate()
            }
        }
        preferences.registerOnSharedPreferenceChangeListener(listener)
    }
}
