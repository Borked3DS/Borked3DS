// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.features.settings.ui

import android.content.Context
import android.content.SharedPreferences
import android.content.res.Resources
import android.hardware.camera2.CameraAccessException
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.os.Build
import android.text.TextUtils
import androidx.preference.PreferenceManager
import io.github.borked3ds.android.Borked3DSApplication
import io.github.borked3ds.android.R
import io.github.borked3ds.android.features.settings.model.AbstractBooleanSetting
import io.github.borked3ds.android.features.settings.model.AbstractIntSetting
import io.github.borked3ds.android.features.settings.model.AbstractSetting
import io.github.borked3ds.android.features.settings.model.AbstractShortSetting
import io.github.borked3ds.android.features.settings.model.AbstractStringSetting
import io.github.borked3ds.android.features.settings.model.BooleanSetting
import io.github.borked3ds.android.features.settings.model.FloatSetting
import io.github.borked3ds.android.features.settings.model.IntSetting
import io.github.borked3ds.android.features.settings.model.ScaledFloatSetting
import io.github.borked3ds.android.features.settings.model.Settings
import io.github.borked3ds.android.features.settings.model.StringSetting
import io.github.borked3ds.android.features.settings.model.view.DateTimeSetting
import io.github.borked3ds.android.features.settings.model.view.HeaderSetting
import io.github.borked3ds.android.features.settings.model.view.InputBindingSetting
import io.github.borked3ds.android.features.settings.model.view.RunnableSetting
import io.github.borked3ds.android.features.settings.model.view.SettingsItem
import io.github.borked3ds.android.features.settings.model.view.SingleChoiceSetting
import io.github.borked3ds.android.features.settings.model.view.SliderSetting
import io.github.borked3ds.android.features.settings.model.view.StringInputSetting
import io.github.borked3ds.android.features.settings.model.view.StringSingleChoiceSetting
import io.github.borked3ds.android.features.settings.model.view.SubmenuSetting
import io.github.borked3ds.android.features.settings.model.view.SwitchSetting
import io.github.borked3ds.android.features.settings.utils.SettingsFile
import io.github.borked3ds.android.fragments.ResetSettingsDialogFragment
import io.github.borked3ds.android.utils.BirthdayMonth
import io.github.borked3ds.android.utils.EmulationMenuSettings
import io.github.borked3ds.android.utils.GpuDriverHelper
import io.github.borked3ds.android.utils.Log
import io.github.borked3ds.android.utils.SystemSaveGame
import io.github.borked3ds.android.utils.ThemeUtil

class SettingsFragmentPresenter(private val fragmentView: SettingsFragmentView) {
    private var menuTag: String? = null
    private lateinit var gameId: String
    private var settingsList: ArrayList<SettingsItem>? = null

    private val settingsActivity get() = fragmentView.activityView as SettingsActivity
    private val settings get() = fragmentView.activityView!!.settings
    private lateinit var settingsAdapter: SettingsAdapter

    private lateinit var preferences: SharedPreferences

    fun onCreate(menuTag: String, gameId: String) {
        this.gameId = gameId
        this.menuTag = menuTag
    }

    fun onViewCreated(settingsAdapter: SettingsAdapter) {
        this.settingsAdapter = settingsAdapter
        preferences = PreferenceManager.getDefaultSharedPreferences(Borked3DSApplication.appContext)
        loadSettingsList()
    }

    fun putSetting(setting: AbstractSetting) {
        if (setting.section == null || setting.key == null) {
            return
        }

        val section = settings.getSection(setting.section!!)!!
        if (section.getSetting(setting.key!!) == null) {
            section.putSetting(setting)
        }
    }

    fun loadSettingsList() {
        if (!TextUtils.isEmpty(gameId)) {
            settingsActivity.setToolbarTitle("Game Settings: $gameId")
        }
        val sl = ArrayList<SettingsItem>()
        if (menuTag == null) {
            return
        }
        when (menuTag) {
            SettingsFile.FILE_NAME_CONFIG -> addConfigSettings(sl)
            Settings.SECTION_CORE -> addGeneralSettings(sl)
            Settings.SECTION_SYSTEM -> addSystemSettings(sl)
            Settings.SECTION_CAMERA -> addCameraSettings(sl)
            Settings.SECTION_CONTROLS -> addControlsSettings(sl)
            Settings.SECTION_RENDERER -> addGraphicsSettings(sl)
            Settings.SECTION_LAYOUT -> addLayoutSettings(sl)
            Settings.SECTION_AUDIO -> addAudioSettings(sl)
            Settings.SECTION_DEBUG -> addDebugSettings(sl)
            Settings.SECTION_THEME -> addThemeSettings(sl)
            Settings.SECTION_STATS_OVERLAY -> addStatsOverlaySettings(sl)
            Settings.SECTION_CUSTOM_LANDSCAPE -> addCustomLandscapeSettings(sl)
            Settings.SECTION_CUSTOM_PORTRAIT -> addCustomPortraitSettings(sl)
            else -> {
                fragmentView.showToastMessage("Unimplemented menu", false)
                return
            }
        }
        settingsList = sl
        fragmentView.showSettingsList(settingsList!!)
    }

    /** Returns the portrait mode width */
    private fun getWidth(): Int {
        val dm = Resources.getSystem().displayMetrics
        return if (dm.widthPixels < dm.heightPixels)
            dm.widthPixels
        else
            dm.heightPixels
    }

    private fun getHeight(): Int {
        val dm = Resources.getSystem().displayMetrics
        return if (dm.widthPixels < dm.heightPixels)
            dm.heightPixels
        else
            dm.widthPixels
    }

    private fun addConfigSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.preferences_settings))
        sl.apply {
            add(
                SubmenuSetting(
                    R.string.preferences_general,
                    R.string.preferences_general_description,
                    R.drawable.ic_general_settings,
                    Settings.SECTION_CORE
                )
            )
            add(
                SubmenuSetting(
                    R.string.preferences_system,
                    R.string.preferences_system_description,
                    R.drawable.ic_system_settings,
                    Settings.SECTION_SYSTEM
                )
            )
            add(
                SubmenuSetting(
                    R.string.preferences_camera,
                    R.string.preferences_camera_description,
                    R.drawable.ic_camera_settings,
                    Settings.SECTION_CAMERA
                )
            )
            add(
                SubmenuSetting(
                    R.string.preferences_controls,
                    R.string.preferences_controls_description,
                    R.drawable.ic_controls_settings,
                    Settings.SECTION_CONTROLS
                )
            )
            add(
                SubmenuSetting(
                    R.string.preferences_graphics,
                    R.string.preferences_graphics_description,
                    R.drawable.ic_graphics,
                    Settings.SECTION_RENDERER
                )
            )
            add(
                SubmenuSetting(
                    R.string.preferences_layout,
                    R.string.preferences_layout_description,
                    R.drawable.ic_fit_screen,
                    Settings.SECTION_LAYOUT
                )
            )
            add(
                SubmenuSetting(
                    R.string.preferences_audio,
                    R.string.preferences_audio_description,
                    R.drawable.ic_audio,
                    Settings.SECTION_AUDIO
                )
            )
            add(
                SubmenuSetting(
                    R.string.preferences_debug,
                    R.string.preferences_debug_description,
                    R.drawable.ic_code,
                    Settings.SECTION_DEBUG
                )
            )

            add(
                RunnableSetting(
                    R.string.reset_to_default,
                    R.string.reset_to_default_description,
                    false,
                    R.drawable.ic_restore,
                    {
                        ResetSettingsDialogFragment().show(
                            settingsActivity.supportFragmentManager,
                            ResetSettingsDialogFragment.TAG
                        )
                    }
                )
            )
        }
    }

    private fun addGeneralSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.preferences_general))
        sl.apply {
            add(
                SwitchSetting(
                    BooleanSetting.EXPAND_TO_CUTOUT_AREA,
                    R.string.expand_to_cutout_area,
                    R.string.expand_to_cutout_area_description,
                    BooleanSetting.EXPAND_TO_CUTOUT_AREA.key,
                    BooleanSetting.EXPAND_TO_CUTOUT_AREA.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.SUSTAINED_PERFORMANCE,
                    R.string.sustained_performance,
                    R.string.sustained_performance_description,
                    BooleanSetting.SUSTAINED_PERFORMANCE.key,
                    BooleanSetting.SUSTAINED_PERFORMANCE.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.USE_FRAME_LIMIT,
                    R.string.frame_limit_enable,
                    R.string.frame_limit_enable_description,
                    BooleanSetting.USE_FRAME_LIMIT.key,
                    BooleanSetting.USE_FRAME_LIMIT.defaultValue
                )
            )
            add(
                SliderSetting(
                    IntSetting.FRAME_LIMIT,
                    R.string.frame_limit_slider,
                    R.string.frame_limit_slider_description,
                    1,
                    200,
                    "%",
                    IntSetting.FRAME_LIMIT.key,
                    IntSetting.FRAME_LIMIT.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.TURBO_SPEED,
                    R.string.turbo_speed,
                    R.string.turbo_speed_description,
                    100,
                    400,
                    "%",
                    IntSetting.TURBO_SPEED.key,
                    IntSetting.TURBO_SPEED.defaultValue.toFloat()
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.HIDE_IMAGES,
                    R.string.hide_images,
                    R.string.hide_images_description,
                    BooleanSetting.HIDE_IMAGES.key,
                    BooleanSetting.HIDE_IMAGES.defaultValue
                )
            )
        }
    }

    @OptIn(ExperimentalStdlibApi::class)
    private fun addSystemSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.preferences_system))
        sl.apply {
            add(
                SwitchSetting(
                    BooleanSetting.NEW_3DS,
                    R.string.new_3ds,
                    R.string.new_3ds_description,
                    BooleanSetting.NEW_3DS.key,
                    BooleanSetting.NEW_3DS.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.LLE_APPLETS,
                    R.string.lle_applets,
                    R.string.lle_applets_description,
                    BooleanSetting.LLE_APPLETS.key,
                    BooleanSetting.LLE_APPLETS.defaultValue
                )
            )
            val usernameSetting = object : AbstractStringSetting {
                override var string: String
                    get() = SystemSaveGame.getUsername()
                    set(value) = SystemSaveGame.setUsername(value)
                override val key = null
                override val section = null
                override val isRuntimeEditable = false
                override val valueAsString get() = string
                override val defaultValue = "BORKED3DS"
            }
            add(
                StringInputSetting(
                    usernameSetting,
                    R.string.username,
                    0,
                    "BORKED3DS",
                    10
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.EMULATED_REGION,
                    R.string.emulated_region,
                    0,
                    R.array.regionNames,
                    R.array.regionValues,
                    IntSetting.EMULATED_REGION.key,
                    IntSetting.EMULATED_REGION.defaultValue
                )
            )

            val systemLanguageSetting = object : AbstractIntSetting {
                override var int: Int
                    get() = SystemSaveGame.getSystemLanguage()
                    set(value) = SystemSaveGame.setSystemLanguage(value)
                override val key = null
                override val section = null
                override val isRuntimeEditable = false
                override val valueAsString get() = int.toString()
                override val defaultValue = 1
            }
            add(
                SingleChoiceSetting(
                    systemLanguageSetting,
                    R.string.emulated_language,
                    0,
                    R.array.languageNames,
                    R.array.languageValues
                )
            )

            val systemCountrySetting = object : AbstractShortSetting {
                override var short: Short
                    get() = SystemSaveGame.getCountryCode()
                    set(value) = SystemSaveGame.setCountryCode(value)
                override val key = null
                override val section = null
                override val isRuntimeEditable = false
                override val valueAsString = short.toString()
                override val defaultValue: Short = 49
            }
            var index = -1
            val countries = settingsActivity.resources.getStringArray(R.array.countries)
                .mapNotNull {
                    index++
                    if (it.isNotEmpty()) it to index.toString() else null
                }
            add(
                StringSingleChoiceSetting(
                    systemCountrySetting,
                    R.string.country,
                    0,
                    countries.map { it.first }.toTypedArray(),
                    countries.map { it.second }.toTypedArray()
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.INIT_TICKS_TYPE,
                    R.string.init_ticks_type,
                    R.string.init_ticks_type_description,
                    R.array.initTicksNames,
                    R.array.initTicksValues,
                    IntSetting.INIT_TICKS_TYPE.key,
                    IntSetting.INIT_TICKS_TYPE.defaultValue
                )
            )
            add(
                SliderSetting(
                    IntSetting.INIT_TICKS_OVERRIDE,
                    R.string.init_ticks_override,
                    R.string.init_ticks_override_description,
                    0,
                    65535,
                    " ticks",
                    IntSetting.INIT_TICKS_OVERRIDE.key,
                    IntSetting.INIT_TICKS_OVERRIDE.defaultValue.toFloat()
                )
            )
            val playCoinSettings = object : AbstractIntSetting {
                override var int: Int
                    get() = SystemSaveGame.getPlayCoins()
                    set(value) = SystemSaveGame.setPlayCoins(value)
                override val key = null
                override val section = null
                override val isRuntimeEditable = false
                override val valueAsString = int.toString()
                override val defaultValue = 42
            }
            add(
                SliderSetting(
                    playCoinSettings,
                    R.string.play_coins,
                    0,
                    0,
                    300,
                    ""
                )
            )
            add(
                SliderSetting(
                    IntSetting.STEPS_PER_HOUR,
                    R.string.steps_per_hour,
                    R.string.steps_per_hour_description,
                    0,
                    65535,
                    " steps",
                    IntSetting.STEPS_PER_HOUR.key,
                    IntSetting.STEPS_PER_HOUR.defaultValue.toFloat()
                )
            )
            add(
                RunnableSetting(
                    R.string.console_id,
                    0,
                    false,
                    0,
                    { settingsAdapter.onClickRegenerateConsoleId() },
                    { "0x${SystemSaveGame.getConsoleId().toHexString().uppercase()}" }
                )
            )
            add(HeaderSetting(R.string.birthday))
            val systemBirthdayMonthSetting = object : AbstractShortSetting {
                override var short: Short
                    get() = SystemSaveGame.getBirthday()[0]
                    set(value) {
                        val birthdayDay = SystemSaveGame.getBirthday()[1]
                        val daysInNewMonth = BirthdayMonth.getMonthFromCode(value)?.days ?: 31
                        if (daysInNewMonth < birthdayDay) {
                            SystemSaveGame.setBirthday(value, 1)
                            settingsAdapter.notifyDataSetChanged()
                        } else {
                            SystemSaveGame.setBirthday(value, birthdayDay)
                        }
                    }
                override val key = null
                override val section = null
                override val isRuntimeEditable = false
                override val valueAsString get() = short.toString()
                override val defaultValue: Short = 3
            }
            add(
                SingleChoiceSetting(
                    systemBirthdayMonthSetting,
                    R.string.birthday_month,
                    0,
                    R.array.months,
                    R.array.monthValues
                )
            )

            val systemBirthdayDaySetting = object : AbstractShortSetting {
                override var short: Short
                    get() = SystemSaveGame.getBirthday()[1]
                    set(value) {
                        val birthdayMonth = SystemSaveGame.getBirthday()[0]
                        val daysInNewMonth =
                            BirthdayMonth.getMonthFromCode(birthdayMonth)?.days ?: 31
                        if (value > daysInNewMonth) {
                            SystemSaveGame.setBirthday(birthdayMonth, 1)
                        } else {
                            SystemSaveGame.setBirthday(birthdayMonth, value)
                        }
                    }
                override val key = null
                override val section = null
                override val isRuntimeEditable = false
                override val valueAsString get() = short.toString()
                override val defaultValue: Short = 25
            }
            val birthdayMonth = SystemSaveGame.getBirthday()[0]
            val daysInMonth = BirthdayMonth.getMonthFromCode(birthdayMonth)?.days ?: 31
            val dayArray = Array(daysInMonth) { "${it + 1}" }
            add(
                StringSingleChoiceSetting(
                    systemBirthdayDaySetting,
                    R.string.birthday_day,
                    0,
                    dayArray,
                    dayArray
                )
            )

            add(HeaderSetting(R.string.clock))
            add(
                SingleChoiceSetting(
                    IntSetting.INIT_CLOCK,
                    R.string.init_clock,
                    R.string.init_clock_description,
                    R.array.systemClockNames,
                    R.array.systemClockValues,
                    IntSetting.INIT_CLOCK.key,
                    IntSetting.INIT_CLOCK.defaultValue
                )
            )
            add(
                DateTimeSetting(
                    StringSetting.INIT_TIME,
                    R.string.init_time,
                    R.string.init_time_description,
                    StringSetting.INIT_TIME.key,
                    StringSetting.INIT_TIME.defaultValue
                )
            )

            add(HeaderSetting(R.string.plugin_loader))
            add(
                SwitchSetting(
                    BooleanSetting.PLUGIN_LOADER,
                    R.string.plugin_loader,
                    R.string.plugin_loader_description,
                    BooleanSetting.PLUGIN_LOADER.key,
                    BooleanSetting.PLUGIN_LOADER.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.ALLOW_PLUGIN_LOADER,
                    R.string.allow_plugin_loader,
                    R.string.allow_plugin_loader_description,
                    BooleanSetting.ALLOW_PLUGIN_LOADER.key,
                    BooleanSetting.ALLOW_PLUGIN_LOADER.defaultValue
                )
            )
            add(HeaderSetting(R.string.storage))
            add(
                SwitchSetting(
                    BooleanSetting.USE_VIRTUAL_SD,
                    R.string.use_virtual_sd,
                    R.string.use_virtual_sd_description,
                    BooleanSetting.USE_VIRTUAL_SD.key,
                    BooleanSetting.USE_VIRTUAL_SD.defaultValue
                )
            )

        }
    }

    private fun addCameraSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.camera))

        // Get the camera IDs
        val cameraManager =
            settingsActivity.getSystemService(Context.CAMERA_SERVICE) as CameraManager?
        val supportedCameraNameList = ArrayList<String>()
        val supportedCameraIdList = ArrayList<String>()
        if (cameraManager != null) {
            try {
                for (id in cameraManager.cameraIdList) {
                    val characteristics = cameraManager.getCameraCharacteristics(id)
                    if (characteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL) ==
                        CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY
                    ) {
                        continue  // Legacy cameras cannot be used with the NDK
                    }
                    supportedCameraIdList.add(id)
                    val facing = characteristics.get(CameraCharacteristics.LENS_FACING)
                    var stringId: Int = R.string.camera_facing_external
                    when (facing) {
                        CameraCharacteristics.LENS_FACING_FRONT -> stringId =
                            R.string.camera_facing_front

                        CameraCharacteristics.LENS_FACING_BACK -> stringId =
                            R.string.camera_facing_back

                        CameraCharacteristics.LENS_FACING_EXTERNAL -> stringId =
                            R.string.camera_facing_external
                    }
                    supportedCameraNameList.add(
                        String.format("%1\$s (%2\$s)", id, settingsActivity.getString(stringId))
                    )
                }
            } catch (e: CameraAccessException) {
                Log.error("Couldn't retrieve camera list")
                e.printStackTrace()
            }
        }

        // Create the names and values for display
        val cameraDeviceNameList =
            settingsActivity.resources.getStringArray(R.array.cameraDeviceNames).toMutableList()
        cameraDeviceNameList.addAll(supportedCameraNameList)
        val cameraDeviceValueList =
            settingsActivity.resources.getStringArray(R.array.cameraDeviceValues).toMutableList()
        cameraDeviceValueList.addAll(supportedCameraIdList)

        val haveCameraDevices = supportedCameraIdList.isNotEmpty()

        val imageSourceNames =
            settingsActivity.resources.getStringArray(R.array.cameraImageSourceNames)
        val imageSourceValues =
            settingsActivity.resources.getStringArray(R.array.cameraImageSourceValues)
        if (!haveCameraDevices) {
            // Remove the last entry (ndk / Device Camera)
            imageSourceNames.copyOfRange(0, imageSourceNames.size - 1)
            imageSourceValues.copyOfRange(0, imageSourceValues.size - 1)
        }

        sl.apply {
            add(HeaderSetting(R.string.inner_camera))
            add(
                StringSingleChoiceSetting(
                    StringSetting.CAMERA_INNER_NAME,
                    R.string.image_source,
                    R.string.image_source_description,
                    imageSourceNames,
                    imageSourceValues,
                    StringSetting.CAMERA_INNER_NAME.key,
                    StringSetting.CAMERA_INNER_NAME.defaultValue
                )
            )
            if (haveCameraDevices) {
                add(
                    StringSingleChoiceSetting(
                        StringSetting.CAMERA_INNER_CONFIG,
                        R.string.camera_device,
                        R.string.camera_device_description,
                        cameraDeviceNameList.toTypedArray(),
                        cameraDeviceValueList.toTypedArray(),
                        StringSetting.CAMERA_INNER_CONFIG.key,
                        StringSetting.CAMERA_INNER_CONFIG.defaultValue
                    )
                )
            }
            add(
                SingleChoiceSetting(
                    IntSetting.CAMERA_INNER_FLIP,
                    R.string.image_flip,
                    R.string.image_flip_description,
                    R.array.cameraFlipNames,
                    R.array.cameraDeviceValues,
                    IntSetting.CAMERA_INNER_FLIP.key,
                    IntSetting.CAMERA_INNER_FLIP.defaultValue
                )
            )

            add(HeaderSetting(R.string.outer_left_camera))
            add(
                StringSingleChoiceSetting(
                    StringSetting.CAMERA_OUTER_LEFT_NAME,
                    R.string.image_source,
                    R.string.image_source_description,
                    imageSourceNames,
                    imageSourceValues,
                    StringSetting.CAMERA_OUTER_LEFT_NAME.key,
                    StringSetting.CAMERA_OUTER_LEFT_NAME.defaultValue
                )
            )
            if (haveCameraDevices) {
                add(
                    StringSingleChoiceSetting(
                        StringSetting.CAMERA_OUTER_LEFT_CONFIG,
                        R.string.camera_device,
                        R.string.camera_device_description,
                        cameraDeviceNameList.toTypedArray(),
                        cameraDeviceValueList.toTypedArray(),
                        StringSetting.CAMERA_OUTER_LEFT_CONFIG.key,
                        StringSetting.CAMERA_OUTER_LEFT_CONFIG.defaultValue
                    )
                )
            }
            add(
                SingleChoiceSetting(
                    IntSetting.CAMERA_OUTER_LEFT_FLIP,
                    R.string.image_flip,
                    R.string.image_flip_description,
                    R.array.cameraFlipNames,
                    R.array.cameraDeviceValues,
                    IntSetting.CAMERA_OUTER_LEFT_FLIP.key,
                    IntSetting.CAMERA_OUTER_LEFT_FLIP.defaultValue
                )
            )

            add(HeaderSetting(R.string.outer_right_camera))
            add(
                StringSingleChoiceSetting(
                    StringSetting.CAMERA_OUTER_RIGHT_NAME,
                    R.string.image_source,
                    R.string.image_source_description,
                    imageSourceNames,
                    imageSourceValues,
                    StringSetting.CAMERA_OUTER_RIGHT_NAME.key,
                    StringSetting.CAMERA_OUTER_RIGHT_NAME.defaultValue
                )
            )
            if (haveCameraDevices) {
                add(
                    StringSingleChoiceSetting(
                        StringSetting.CAMERA_OUTER_RIGHT_CONFIG,
                        R.string.camera_device,
                        R.string.camera_device_description,
                        cameraDeviceNameList.toTypedArray(),
                        cameraDeviceValueList.toTypedArray(),
                        StringSetting.CAMERA_OUTER_RIGHT_CONFIG.key,
                        StringSetting.CAMERA_OUTER_RIGHT_CONFIG.defaultValue
                    )
                )
            }
            add(
                SingleChoiceSetting(
                    IntSetting.CAMERA_OUTER_RIGHT_FLIP,
                    R.string.image_flip,
                    R.string.image_flip_description,
                    R.array.cameraFlipNames,
                    R.array.cameraDeviceValues,
                    IntSetting.CAMERA_OUTER_RIGHT_FLIP.key,
                    IntSetting.CAMERA_OUTER_RIGHT_FLIP.defaultValue
                )
            )
        }
    }

    private fun addControlsSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.preferences_controls))
        sl.apply {
            add(
                RunnableSetting(
                    R.string.controller_quick_config,
                    R.string.controller_quick_config_description,
                    false,
                    0,
                    { settingsAdapter.onClickControllerQuickConfig() }
                )
            )

            add(HeaderSetting(R.string.controller_buttons))
            Settings.buttonKeys.forEachIndexed { i: Int, key: String ->
                val button = InputBindingSetting.getInputObject(key, preferences)
                add(InputBindingSetting(button, Settings.buttonTitles[i]))
            }

            add(HeaderSetting(R.string.controller_circlepad))
            Settings.circlePadKeys.forEachIndexed { i: Int, key: String ->
                val button = InputBindingSetting.getInputObject(key, preferences)
                add(InputBindingSetting(button, Settings.axisTitles[i]))
            }

            add(HeaderSetting(R.string.controller_c))
            Settings.cStickKeys.forEachIndexed { i: Int, key: String ->
                val button = InputBindingSetting.getInputObject(key, preferences)
                add(InputBindingSetting(button, Settings.axisTitles[i]))
            }

            add(
                HeaderSetting(
                    R.string.controller_dpad_axis,
                    R.string.controller_dpad_axis_description
                )
            )
            Settings.dPadAxisKeys.forEachIndexed { i: Int, key: String ->
                val button = InputBindingSetting.getInputObject(key, preferences)
                add(InputBindingSetting(button, Settings.axisTitles[i]))
            }
            add(
                HeaderSetting(
                    R.string.controller_dpad_button,
                    R.string.controller_dpad_button_description
                )
            )
            Settings.dPadButtonKeys.forEachIndexed { i: Int, key: String ->
                val button = InputBindingSetting.getInputObject(key, preferences)
                add(InputBindingSetting(button, Settings.dPadTitles[i]))
            }

            add(HeaderSetting(R.string.controller_triggers))
            Settings.triggerKeys.forEachIndexed { i: Int, key: String ->
                val button = InputBindingSetting.getInputObject(key, preferences)
                add(InputBindingSetting(button, Settings.triggerTitles[i]))
            }

            add(HeaderSetting(R.string.controller_hotkeys))
            Settings.hotKeys.forEachIndexed { i: Int, key: String ->
                val button = InputBindingSetting.getInputObject(key, preferences)
                add(InputBindingSetting(button, Settings.hotkeyTitles[i]))
            }
            add(HeaderSetting(R.string.miscellaneous))
            add(
                SwitchSetting(
                    BooleanSetting.USE_ARTIC_BASE_CONTROLLER,
                    R.string.use_artic_base_controller,
                    R.string.use_artic_base_controller_desc,
                    BooleanSetting.USE_ARTIC_BASE_CONTROLLER.key,
                    BooleanSetting.USE_ARTIC_BASE_CONTROLLER.defaultValue
                )
            )
        }
    }

    private fun addGraphicsSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.preferences_graphics))
        sl.apply {
            add(
                SingleChoiceSetting(
                    IntSetting.GRAPHICS_API,
                    R.string.graphics_api,
                    0,
                    R.array.graphicsApiNames,
                    R.array.graphicsApiValues,
                    IntSetting.GRAPHICS_API.key,
                    IntSetting.GRAPHICS_API.defaultValue
                )
            )
            add(HeaderSetting(R.string.renderer))
            add(
                SingleChoiceSetting(
                    IntSetting.RESOLUTION_FACTOR,
                    R.string.internal_resolution,
                    R.string.internal_resolution_description,
                    R.array.resolutionFactorNames,
                    R.array.resolutionFactorValues,
                    IntSetting.RESOLUTION_FACTOR.key,
                    IntSetting.RESOLUTION_FACTOR.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.LINEAR_FILTERING,
                    R.string.linear_filtering,
                    R.string.linear_filtering_description,
                    BooleanSetting.LINEAR_FILTERING.key,
                    BooleanSetting.LINEAR_FILTERING.defaultValue
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.TEXTURE_FILTER,
                    R.string.texture_filter_name,
                    R.string.texture_filter_description,
                    R.array.textureFilterNames,
                    R.array.textureFilterValues,
                    IntSetting.TEXTURE_FILTER.key,
                    IntSetting.TEXTURE_FILTER.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.HW_SHADER,
                    R.string.hw_shaders,
                    R.string.hw_shaders_description,
                    BooleanSetting.HW_SHADER.key,
                    BooleanSetting.HW_SHADER.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.SHADERS_ACCURATE_MUL,
                    R.string.shaders_accurate_mul,
                    R.string.shaders_accurate_mul_description,
                    BooleanSetting.SHADERS_ACCURATE_MUL.key,
                    BooleanSetting.SHADERS_ACCURATE_MUL.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.SHADER_JIT,
                    R.string.use_shader_jit,
                    R.string.use_shader_jit_description,
                    BooleanSetting.SHADER_JIT.key,
                    BooleanSetting.SHADER_JIT.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.ASYNC_SHADERS,
                    R.string.async_shaders,
                    R.string.async_shaders_description,
                    BooleanSetting.ASYNC_SHADERS.key,
                    BooleanSetting.ASYNC_SHADERS.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.ASYNC_PRESENTATION,
                    R.string.async_presentation,
                    R.string.async_presentation_description,
                    BooleanSetting.ASYNC_PRESENTATION.key,
                    BooleanSetting.ASYNC_PRESENTATION.defaultValue
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.FRAME_SKIP,
                    R.string.frame_skip_name,
                    R.string.frame_skip_description,
                    R.array.frameSkipNames,
                    R.array.frameSkipValues,
                    IntSetting.FRAME_SKIP.key,
                    IntSetting.FRAME_SKIP.defaultValue
                )
            )
            add(HeaderSetting(R.string.vulkan_options))
            add(
                SwitchSetting(
                    BooleanSetting.SPIRV_SHADER_GEN,
                    R.string.spirv_shader_gen,
                    R.string.spirv_shader_gen_description,
                    BooleanSetting.SPIRV_SHADER_GEN.key,
                    BooleanSetting.SPIRV_SHADER_GEN.defaultValue,
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.OPTIMIZE_SPIRV,
                    R.string.optimize_spirv,
                    R.string.optimize_spirv_description,
                    R.array.optimizeSpirvNames,
                    R.array.optimizeSpirvValues,
                    IntSetting.OPTIMIZE_SPIRV.key,
                    IntSetting.OPTIMIZE_SPIRV.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.SPIRV_OUTPUT_VALIDATION,
                    R.string.spirv_output_validation,
                    R.string.spirv_output_validation_description,
                    BooleanSetting.SPIRV_OUTPUT_VALIDATION.key,
                    BooleanSetting.SPIRV_OUTPUT_VALIDATION.defaultValue,
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.SPIRV_OUTPUT_LEGALIZATION,
                    R.string.spirv_output_legalization,
                    R.string.spirv_output_legalization_description,
                    BooleanSetting.SPIRV_OUTPUT_LEGALIZATION.key,
                    BooleanSetting.SPIRV_OUTPUT_LEGALIZATION.defaultValue,
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.GEOMETRY_SHADER,
                    R.string.geometry_shader,
                    R.string.geometry_shader_desc,
                    BooleanSetting.GEOMETRY_SHADER.key,
                    BooleanSetting.GEOMETRY_SHADER.defaultValue,
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.RELAXED_PRECISION_DECORATORS,
                    R.string.relaxed_precision_decorators,
                    R.string.relaxed_precision_decorators_desc,
                    BooleanSetting.RELAXED_PRECISION_DECORATORS.key,
                    BooleanSetting.RELAXED_PRECISION_DECORATORS.defaultValue,
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.USE_SAMPLE_SHADING,
                    R.string.use_sample_shading,
                    R.string.use_sample_shading_description,
                    BooleanSetting.USE_SAMPLE_SHADING.key,
                    BooleanSetting.USE_SAMPLE_SHADING.defaultValue,
                )
            )
            add(HeaderSetting(R.string.graphics_hacks, R.string.hacks_description))
            add(
                SwitchSetting(
                    BooleanSetting.SKIP_SLOW_DRAW,
                    R.string.skip_slow_draw,
                    R.string.skip_slow_draw_description,
                    BooleanSetting.SKIP_SLOW_DRAW.key,
                    BooleanSetting.SKIP_SLOW_DRAW.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.SKIP_TEXTURE_COPY,
                    R.string.skip_texture_copy,
                    R.string.skip_texture_copy_description,
                    BooleanSetting.SKIP_TEXTURE_COPY.key,
                    BooleanSetting.SKIP_TEXTURE_COPY.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.SKIP_CPU_WRITE,
                    R.string.skip_cpu_write,
                    R.string.skip_cpu_write_description,
                    BooleanSetting.SKIP_CPU_WRITE.key,
                    BooleanSetting.SKIP_CPU_WRITE.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.UPSCALING_HACK,
                    R.string.upscaling_hack,
                    R.string.upscaling_hack_description,
                    BooleanSetting.UPSCALING_HACK.key,
                    BooleanSetting.UPSCALING_HACK.defaultValue
                )
            )
            add(HeaderSetting(R.string.stereoscopy))
            add(
                SingleChoiceSetting(
                    IntSetting.STEREOSCOPIC_3D_MODE,
                    R.string.render3d,
                    0,
                    R.array.render3dModes,
                    R.array.render3dValues,
                    IntSetting.STEREOSCOPIC_3D_MODE.key,
                    IntSetting.STEREOSCOPIC_3D_MODE.defaultValue
                )
            )
            add(
                SliderSetting(
                    IntSetting.STEREOSCOPIC_3D_DEPTH,
                    R.string.factor3d,
                    R.string.factor3d_description,
                    0,
                    255,
                    "%",
                    IntSetting.STEREOSCOPIC_3D_DEPTH.key,
                    IntSetting.STEREOSCOPIC_3D_DEPTH.defaultValue.toFloat()
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.MONO_RENDER_OPTION,
                    R.string.mono_render_option,
                    R.string.mono_render_option_description,
                    R.array.monoRenderOptionModes,
                    R.array.monoRenderOptionValues,
                    IntSetting.MONO_RENDER_OPTION.key,
                    IntSetting.MONO_RENDER_OPTION.defaultValue
                )
            )

            val anaglyphShaderModes =
                settingsActivity.resources.getStringArray(R.array.anaglyphShaderNameModes)
            val anaglyphShaderValues =
                settingsActivity.resources.getStringArray(R.array.anaglyphShaderNameValues)

            add(
                StringSingleChoiceSetting(
                    StringSetting.ANAGLYPH_SHADER_NAME,
                    R.string.anaglyph_shader_name,
                    R.string.anaglyph_shader_name_description,
                    anaglyphShaderModes,
                    anaglyphShaderValues,
                    StringSetting.ANAGLYPH_SHADER_NAME.key,
                    StringSetting.ANAGLYPH_SHADER_NAME.defaultValue
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.SWAP_EYES_3D,
                    R.string.swap_eyes_3d,
                    R.string.swap_eyes_3d_description,
                    BooleanSetting.SWAP_EYES_3D.key,
                    BooleanSetting.SWAP_EYES_3D.defaultValue
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.DISABLE_RIGHT_EYE_RENDER,
                    R.string.disable_right_eye_render,
                    R.string.disable_right_eye_render_desc,
                    BooleanSetting.DISABLE_RIGHT_EYE_RENDER.key,
                    BooleanSetting.DISABLE_RIGHT_EYE_RENDER.defaultValue
                )
            )

            add(HeaderSetting(R.string.cardboard_vr))
            add(
                SliderSetting(
                    IntSetting.CARDBOARD_SCREEN_SIZE,
                    R.string.cardboard_screen_size,
                    R.string.cardboard_screen_size_description,
                    30,
                    100,
                    "%",
                    IntSetting.CARDBOARD_SCREEN_SIZE.key,
                    IntSetting.CARDBOARD_SCREEN_SIZE.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.CARDBOARD_X_SHIFT,
                    R.string.cardboard_x_shift,
                    R.string.cardboard_x_shift_description,
                    -100,
                    100,
                    "%",
                    IntSetting.CARDBOARD_X_SHIFT.key,
                    IntSetting.CARDBOARD_X_SHIFT.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.CARDBOARD_Y_SHIFT,
                    R.string.cardboard_y_shift,
                    R.string.cardboard_y_shift_description,
                    -100,
                    100,
                    "%",
                    IntSetting.CARDBOARD_Y_SHIFT.key,
                    IntSetting.CARDBOARD_Y_SHIFT.defaultValue.toFloat()
                )
            )

            add(HeaderSetting(R.string.utility))
            add(
                SwitchSetting(
                    BooleanSetting.CUSTOM_TEXTURES,
                    R.string.custom_textures,
                    R.string.custom_textures_description,
                    BooleanSetting.CUSTOM_TEXTURES.key,
                    BooleanSetting.CUSTOM_TEXTURES.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.DUMP_TEXTURES,
                    R.string.dump_textures,
                    R.string.dump_textures_description,
                    BooleanSetting.DUMP_TEXTURES.key,
                    BooleanSetting.DUMP_TEXTURES.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.PRELOAD_TEXTURES,
                    R.string.preload_textures,
                    R.string.preload_textures_description,
                    BooleanSetting.PRELOAD_TEXTURES.key,
                    BooleanSetting.PRELOAD_TEXTURES.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.ASYNC_CUSTOM_LOADING,
                    R.string.async_custom_loading,
                    R.string.async_custom_loading_description,
                    BooleanSetting.ASYNC_CUSTOM_LOADING.key,
                    BooleanSetting.ASYNC_CUSTOM_LOADING.defaultValue
                )
            )
            add(HeaderSetting(R.string.advanced))
            if (GpuDriverHelper.supportsCustomDriverLoading()) {
                add(
                    SwitchSetting(
                        BooleanSetting.ADRENO_GPU_BOOST,
                        R.string.adreno_gpu_boost,
                        R.string.adreno_gpu_boost_description,
                        BooleanSetting.ADRENO_GPU_BOOST.key,
                        BooleanSetting.ADRENO_GPU_BOOST.defaultValue
                    )
                )
            }
            add(
                SingleChoiceSetting(
                    IntSetting.TEXTURE_SAMPLING,
                    R.string.texture_sampling_name,
                    R.string.texture_sampling_description,
                    R.array.textureSamplingNames,
                    R.array.textureSamplingValues,
                    IntSetting.TEXTURE_SAMPLING.key,
                    IntSetting.TEXTURE_SAMPLING.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.DISK_SHADER_CACHE,
                    R.string.use_disk_shader_cache,
                    R.string.use_disk_shader_cache_description,
                    BooleanSetting.DISK_SHADER_CACHE.key,
                    BooleanSetting.DISK_SHADER_CACHE.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.VSYNC,
                    R.string.vsync,
                    R.string.vsync_description,
                    BooleanSetting.VSYNC.key,
                    BooleanSetting.VSYNC.defaultValue
                )
            )
            add(
                SliderSetting(
                    IntSetting.DELAY_RENDER_THREAD_US,
                    R.string.delay_render_thread,
                    R.string.delay_render_thread_description,
                    0,
                    16000,
                    " μs",
                    IntSetting.DELAY_RENDER_THREAD_US.key,
                    IntSetting.DELAY_RENDER_THREAD_US.defaultValue.toFloat()
                )
            )
        }
    }

    private fun addLayoutSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle("Layout")
        sl.apply {
            add(HeaderSetting(R.string.layout_screens))
            add(
                SingleChoiceSetting(
                    IntSetting.ORIENTATION_OPTION,
                    R.string.layout_screen_orientation,
                    0,
                    R.array.screenOrientations,
                    R.array.screenOrientationValues,
                    IntSetting.ORIENTATION_OPTION.key,
                    IntSetting.ORIENTATION_OPTION.defaultValue
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.SCREEN_LAYOUT,
                    R.string.emulation_switch_screen_layout,
                    0,
                    R.array.landscapeLayouts,
                    R.array.landscapeLayoutValues,
                    IntSetting.SCREEN_LAYOUT.key,
                    IntSetting.SCREEN_LAYOUT.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.UPRIGHT_SCREEN,
                    R.string.emulation_rotate_upright,
                    R.string.emulation_rotate_upright_desc,
                    BooleanSetting.UPRIGHT_SCREEN.key,
                    BooleanSetting.UPRIGHT_SCREEN.defaultValue
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.PORTRAIT_SCREEN_LAYOUT,
                    R.string.emulation_switch_portrait_layout,
                    0,
                    R.array.portraitLayouts,
                    R.array.portraitLayoutValues,
                    IntSetting.PORTRAIT_SCREEN_LAYOUT.key,
                    IntSetting.PORTRAIT_SCREEN_LAYOUT.defaultValue
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.SMALL_SCREEN_POSITION,
                    R.string.emulation_small_screen_position,
                    R.string.small_screen_position_description,
                    R.array.smallScreenPositions,
                    R.array.smallScreenPositionValues,
                    IntSetting.SMALL_SCREEN_POSITION.key,
                    IntSetting.SMALL_SCREEN_POSITION.defaultValue
                )
            )
            add(
                SliderSetting(
                    IntSetting.SCREEN_GAP,
                    R.string.screen_gap,
                    R.string.screen_gap_description,
                    0,
                    480,
                    "px",
                    IntSetting.SCREEN_GAP.key,
                    IntSetting.SCREEN_GAP.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    FloatSetting.LARGE_SCREEN_PROPORTION,
                    R.string.large_screen_proportion,
                    R.string.large_screen_proportion_description,
                    1,
                    5,
                    "",
                    FloatSetting.LARGE_SCREEN_PROPORTION.key,
                    FloatSetting.LARGE_SCREEN_PROPORTION.defaultValue
                )
            )
            add(HeaderSetting(R.string.custom_layout))
            add(
                SubmenuSetting(
                    R.string.stats_overlay_options,
                    R.string.stats_overlay_options_description,
                    R.drawable.ic_frames,
                    Settings.SECTION_STATS_OVERLAY
                )
            )
            add(
                SubmenuSetting(
                    R.string.emulation_landscape_custom_layout,
                    R.string.emulation_landscape_custom_layout_description,
                    R.drawable.ic_fit_screen,
                    Settings.SECTION_CUSTOM_LANDSCAPE
                )
            )
            add(
                SubmenuSetting(
                    R.string.emulation_portrait_custom_layout,
                    R.string.emulation_portrait_custom_layout_description,
                    R.drawable.ic_portrait_fit_screen,
                    Settings.SECTION_CUSTOM_PORTRAIT
                )
            )
            add(
                SliderSetting(
                    FloatSetting.SECOND_SCREEN_OPACITY,
                    R.string.second_screen_opacity,
                    R.string.second_screen_opacity_description,
                    0,
                    100,
                    "%",
                    FloatSetting.SECOND_SCREEN_OPACITY.key,
                    FloatSetting.SECOND_SCREEN_OPACITY.defaultValue
                )
            )
            add(HeaderSetting(R.string.bg_color, R.string.bg_color_description))
            add(
                SliderSetting(
                    FloatSetting.BG_RED,
                    R.string.bg_red,
                    R.string.bg_description,
                    0,
                    1,
                    "x",
                    FloatSetting.BG_RED.key,
                    FloatSetting.BG_RED.defaultValue
                )
            )
            add(
                SliderSetting(
                    FloatSetting.BG_BLUE,
                    R.string.bg_blue,
                    R.string.bg_description,
                    0,
                    1,
                    "x",
                    FloatSetting.BG_BLUE.key,
                    FloatSetting.BG_BLUE.defaultValue
                )
            )
            add(
                SliderSetting(
                    FloatSetting.BG_GREEN,
                    R.string.bg_green,
                    R.string.bg_description,
                    0,
                    1,
                    "x",
                    FloatSetting.BG_GREEN.key,
                    FloatSetting.BG_GREEN.defaultValue
                )
            )
        }
    }

    private fun addCustomLandscapeSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.emulation_landscape_custom_layout))
        sl.apply {
            add(HeaderSetting(R.string.emulation_top_screen))
            add(
                SliderSetting(
                    IntSetting.LANDSCAPE_TOP_X,
                    R.string.emulation_custom_layout_x,
                    0,
                    0,
                    getHeight(),
                    "px",
                    IntSetting.LANDSCAPE_TOP_X.key,
                    IntSetting.LANDSCAPE_TOP_X.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.LANDSCAPE_TOP_Y,
                    R.string.emulation_custom_layout_y,
                    0,
                    0,
                    getWidth(),
                    "px",
                    IntSetting.LANDSCAPE_TOP_Y.key,
                    IntSetting.LANDSCAPE_TOP_Y.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.LANDSCAPE_TOP_WIDTH,
                    R.string.emulation_custom_layout_width,
                    0,
                    0,
                    getHeight(),
                    "px",
                    IntSetting.LANDSCAPE_TOP_WIDTH.key,
                    IntSetting.LANDSCAPE_TOP_WIDTH.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.LANDSCAPE_TOP_HEIGHT,
                    R.string.emulation_custom_layout_height,
                    0,
                    0,
                    getWidth(),
                    "px",
                    IntSetting.LANDSCAPE_TOP_HEIGHT.key,
                    IntSetting.LANDSCAPE_TOP_HEIGHT.defaultValue.toFloat()
                )
            )
            add(HeaderSetting(R.string.emulation_bottom_screen))
            add(
                SliderSetting(
                    IntSetting.LANDSCAPE_BOTTOM_X,
                    R.string.emulation_custom_layout_x,
                    0,
                    0,
                    getHeight(),
                    "px",
                    IntSetting.LANDSCAPE_BOTTOM_X.key,
                    IntSetting.LANDSCAPE_BOTTOM_X.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.LANDSCAPE_BOTTOM_Y,
                    R.string.emulation_custom_layout_y,
                    0,
                    0,
                    getWidth(),
                    "px",
                    IntSetting.LANDSCAPE_BOTTOM_Y.key,
                    IntSetting.LANDSCAPE_BOTTOM_Y.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.LANDSCAPE_BOTTOM_WIDTH,
                    R.string.emulation_custom_layout_width,
                    0,
                    0,
                    getHeight(),
                    "px",
                    IntSetting.LANDSCAPE_BOTTOM_WIDTH.key,
                    IntSetting.LANDSCAPE_BOTTOM_WIDTH.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.LANDSCAPE_BOTTOM_HEIGHT,
                    R.string.emulation_custom_layout_height,
                    0,
                    0,
                    getWidth(),
                    "px",
                    IntSetting.LANDSCAPE_BOTTOM_HEIGHT.key,
                    IntSetting.LANDSCAPE_BOTTOM_HEIGHT.defaultValue.toFloat()
                )
            )
        }

    }

    private fun addCustomPortraitSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.emulation_portrait_custom_layout))
        sl.apply {
            add(HeaderSetting(R.string.emulation_top_screen))
            add(
                SliderSetting(
                    IntSetting.PORTRAIT_TOP_X,
                    R.string.emulation_custom_layout_x,
                    0,
                    0,
                    getWidth(),
                    "px",
                    IntSetting.PORTRAIT_TOP_X.key,
                    IntSetting.PORTRAIT_TOP_X.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.PORTRAIT_TOP_Y,
                    R.string.emulation_custom_layout_y,
                    0,
                    0,
                    getHeight(),
                    "px",
                    IntSetting.PORTRAIT_TOP_Y.key,
                    IntSetting.PORTRAIT_TOP_Y.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.PORTRAIT_TOP_WIDTH,
                    R.string.emulation_custom_layout_width,
                    0,
                    0,
                    getWidth(),
                    "px",
                    IntSetting.PORTRAIT_TOP_WIDTH.key,
                    IntSetting.PORTRAIT_TOP_WIDTH.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.PORTRAIT_TOP_HEIGHT,
                    R.string.emulation_custom_layout_height,
                    0,
                    0,
                    getHeight(),
                    "px",
                    IntSetting.PORTRAIT_TOP_HEIGHT.key,
                    IntSetting.PORTRAIT_TOP_HEIGHT.defaultValue.toFloat()
                )
            )
            add(HeaderSetting(R.string.emulation_bottom_screen))
            add(
                SliderSetting(
                    IntSetting.PORTRAIT_BOTTOM_X,
                    R.string.emulation_custom_layout_x,
                    0,
                    0,
                    getWidth(),
                    "px",
                    IntSetting.PORTRAIT_BOTTOM_X.key,
                    IntSetting.PORTRAIT_BOTTOM_X.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.PORTRAIT_BOTTOM_Y,
                    R.string.emulation_custom_layout_y,
                    0,
                    0,
                    getHeight(),
                    "px",
                    IntSetting.PORTRAIT_BOTTOM_Y.key,
                    IntSetting.PORTRAIT_BOTTOM_Y.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.PORTRAIT_BOTTOM_WIDTH,
                    R.string.emulation_custom_layout_width,
                    0,
                    0,
                    getWidth(),
                    "px",
                    IntSetting.PORTRAIT_BOTTOM_WIDTH.key,
                    IntSetting.PORTRAIT_BOTTOM_WIDTH.defaultValue.toFloat()
                )
            )
            add(
                SliderSetting(
                    IntSetting.PORTRAIT_BOTTOM_HEIGHT,
                    R.string.emulation_custom_layout_height,
                    0,
                    0,
                    getHeight(),
                    "px",
                    IntSetting.PORTRAIT_BOTTOM_HEIGHT.key,
                    IntSetting.PORTRAIT_BOTTOM_HEIGHT.defaultValue.toFloat()
                )
            )
        }

    }

    private fun addAudioSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.preferences_audio))
        sl.apply {
            add(HeaderSetting(R.string.audio_output))
            add(
                SliderSetting(
                    ScaledFloatSetting.AUDIO_VOLUME,
                    R.string.audio_volume,
                    0,
                    0,
                    100,
                    "%",
                    ScaledFloatSetting.AUDIO_VOLUME.key,
                    ScaledFloatSetting.AUDIO_VOLUME.defaultValue
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.AUDIO_EMULATION,
                    R.string.audio_emulation,
                    R.string.audio_emulation_description,
                    R.array.audioEmulationNames,
                    R.array.audioEmulationValues,
                    IntSetting.AUDIO_EMULATION.key,
                    IntSetting.AUDIO_EMULATION.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.ENABLE_AUDIO_STRETCHING,
                    R.string.audio_stretch,
                    R.string.audio_stretch_description,
                    BooleanSetting.ENABLE_AUDIO_STRETCHING.key,
                    BooleanSetting.ENABLE_AUDIO_STRETCHING.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.ENABLE_REALTIME_AUDIO,
                    R.string.realtime_audio,
                    R.string.realtime_audio_description,
                    BooleanSetting.ENABLE_REALTIME_AUDIO.key,
                    BooleanSetting.ENABLE_REALTIME_AUDIO.defaultValue
                )
            )
            add(
                SingleChoiceSetting(
                    IntSetting.AUDIO_OUTPUT_TYPE,
                    R.string.audio_output_type,
                    R.string.audio_output_description,
                    R.array.audioOutputTypeNames,
                    R.array.audioOutputTypeValues,
                    IntSetting.AUDIO_OUTPUT_TYPE.key,
                    IntSetting.AUDIO_OUTPUT_TYPE.defaultValue
                )
            )

            val soundOutputModeSetting = object : AbstractIntSetting {
                override var int: Int
                    get() = SystemSaveGame.getSoundOutputMode()
                    set(value) = SystemSaveGame.setSoundOutputMode(value)
                override val key = null
                override val section = null
                override val isRuntimeEditable = false
                override val valueAsString = int.toString()
                override val defaultValue = 1
            }
            add(
                SingleChoiceSetting(
                    soundOutputModeSetting,
                    R.string.sound_output_mode,
                    0,
                    R.array.soundOutputModes,
                    R.array.soundOutputModeValues
                )
            )
            add(HeaderSetting(R.string.audio_input))
            add(
                SingleChoiceSetting(
                    IntSetting.AUDIO_INPUT_TYPE,
                    R.string.audio_input_type,
                    R.string.audio_input_description,
                    R.array.audioInputTypeNames,
                    R.array.audioInputTypeValues,
                    IntSetting.AUDIO_INPUT_TYPE.key,
                    IntSetting.AUDIO_INPUT_TYPE.defaultValue
                )
            )
        }
    }

    private fun addStatsOverlaySettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.stats_overlay_options))
        sl.apply {

            add(HeaderSetting(R.string.stats_overlay_customization))

            add(
                SwitchSetting(
                    object : AbstractBooleanSetting {
                        override val key = "EmulationMenuSettings_showPerfStatsOverlay"
                        override val section = Settings.SECTION_LAYOUT
                        override val defaultValue = false
                        override var boolean: Boolean
                            get() = EmulationMenuSettings.showPerfStatsOverlay
                            set(value) {
                                EmulationMenuSettings.showPerfStatsOverlay = value
                            }
                        override val isRuntimeEditable = true
                        override val valueAsString: String get() = boolean.toString()
                    },
                    R.string.enable_stats_overlay_,
                    0,
                    "EmulationMenuSettings_showPerfStatsOverlay",
                    false
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.OVERLAY_BACKGROUND,
                    R.string.overlay_background,
                    R.string.overlay_background_description,
                    "overlay_background",
                    false
                )
            )

            add(
                SingleChoiceSetting(
                    IntSetting.PERF_OVERLAY_POSITION,
                    R.string.overlay_position,
                    R.string.overlay_position_description,
                    R.array.statsPosition,
                    R.array.statsPositionValues,
                )
            )


            add(HeaderSetting(R.string.stats_overlay_items))

            add(
                SwitchSetting(
                    BooleanSetting.SHOW_FPS,
                    R.string.show_fps,
                    R.string.show_fps_description,
                    "show_fps",
                    true
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.SHOW_FRAMETIME,
                    R.string.show_frametime,
                    R.string.show_frametime_description,
                    "show_frametime",
                    false
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.SHOW_SPEED,
                    R.string.show_speed,
                    R.string.show_speed_description,
                    "show_speed",
                    false
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.SHOW_APP_RAM_USAGE,
                    R.string.show_app_ram_usage,
                    R.string.show_app_ram_usage_description,
                    "show_app_ram_usage",
                    false
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.SHOW_SYSTEM_RAM_USAGE,
                    R.string.show_system_ram_usage,
                    R.string.show_system_ram_usage_description,
                    "show_system_ram_usage",
                    false
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.SHOW_BAT_TEMPERATURE,
                    R.string.show_bat_temperature,
                    R.string.show_bat_temperature_description,
                    "show_bat_temperature",
                    false
                )
            )
        }
    }

    private fun addDebugSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.preferences_debug))
        sl.apply {
            add(HeaderSetting(R.string.debug_warning))
            add(HeaderSetting(R.string.cpu_options))
            add(
                SliderSetting(
                    IntSetting.CPU_CLOCK_SPEED,
                    R.string.cpu_clock_speed,
                    R.string.cpu_clock_speed_description,
                    25,
                    400,
                    "%",
                    IntSetting.CPU_CLOCK_SPEED.key,
                    IntSetting.CPU_CLOCK_SPEED.defaultValue.toFloat()
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.CPU_JIT,
                    R.string.cpu_jit,
                    R.string.cpu_jit_description,
                    BooleanSetting.CPU_JIT.key,
                    BooleanSetting.CPU_JIT.defaultValue
                )
            )
            add(HeaderSetting(R.string.cpu_hacks, R.string.hacks_description))
            add(
                SwitchSetting(
                    BooleanSetting.ENABLE_CUSTOM_CPU_TICKS,
                    R.string.enable_custom_cpu_ticks,
                    R.string.enable_custom_cpu_ticks_description,
                    BooleanSetting.ENABLE_CUSTOM_CPU_TICKS.key,
                    BooleanSetting.ENABLE_CUSTOM_CPU_TICKS.defaultValue
                )
            )
            add(
                SliderSetting(
                    IntSetting.CUSTOM_CPU_TICKS,
                    R.string.custom_cpu_ticks,
                    0,
                    77,
                    65535,
                    "",
                    IntSetting.CUSTOM_CPU_TICKS.key,
                    IntSetting.CUSTOM_CPU_TICKS.defaultValue.toFloat()
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.CORE_DOWNCOUNT_HACK,
                    R.string.core_downcount_hack,
                    R.string.core_downcount_hack_description,
                    BooleanSetting.CORE_DOWNCOUNT_HACK.key,
                    BooleanSetting.CORE_DOWNCOUNT_HACK.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.PRIORITY_BOOST,
                    R.string.priority_boost,
                    R.string.priority_boost_description,
                    BooleanSetting.PRIORITY_BOOST.key,
                    BooleanSetting.PRIORITY_BOOST.defaultValue
                )
            )
            add(HeaderSetting(R.string.gdb))
            add(
                SwitchSetting(
                    BooleanSetting.GDB_STUB,
                    R.string.gdb_stub,
                    R.string.gdb_stub_description,
                    BooleanSetting.GDB_STUB.key,
                    BooleanSetting.GDB_STUB.defaultValue
                )
            )
            add(
                SliderSetting(
                    IntSetting.GDB_PORT,
                    R.string.gdb_port,
                    R.string.gdb_port_description,
                    0,
                    65535,
                    "",
                    IntSetting.GDB_PORT.key,
                    IntSetting.GDB_PORT.defaultValue.toFloat()
                )
            )
            add(HeaderSetting(R.string.logging))

            val logFilterModes =
                settingsActivity.resources.getStringArray(R.array.logFilterNameModes)
            val logFilterValues =
                settingsActivity.resources.getStringArray(R.array.logFilterNameValues)
            add(
                StringSingleChoiceSetting(
                    StringSetting.LOG_FILTER,
                    R.string.log_filter_name,
                    R.string.log_filter_description,
                    logFilterModes,
                    logFilterValues,
                    StringSetting.LOG_FILTER.key,
                    StringSetting.LOG_FILTER.defaultValue
                )
            )

            add(
                StringInputSetting(
                    StringSetting.LOG_REGEX_FILTER,
                    R.string.log_regex_filter_name,
                    R.string.log_regex_filter_description,
                    StringSetting.LOG_REGEX_FILTER.defaultValue,
                    255
                )
            )

            add(
                SwitchSetting(
                    BooleanSetting.DEBUG_RENDERER,
                    R.string.renderer_debug,
                    R.string.renderer_debug_description,
                    BooleanSetting.DEBUG_RENDERER.key,
                    BooleanSetting.DEBUG_RENDERER.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.INSTANT_DEBUG_LOG,
                    R.string.instant_debug_log,
                    R.string.instant_debug_log_desc,
                    BooleanSetting.INSTANT_DEBUG_LOG.key,
                    BooleanSetting.INSTANT_DEBUG_LOG.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.RECORD_FRAME_TIMES,
                    R.string.record_frame_times,
                    R.string.record_frame_times_description,
                    BooleanSetting.RECORD_FRAME_TIMES.key,
                    BooleanSetting.RECORD_FRAME_TIMES.defaultValue
                )
            )
            add(
                SwitchSetting(
                    BooleanSetting.DUMP_COMMAND_BUFFERS,
                    R.string.dump_command_buffers,
                    R.string.dump_command_buffers_description,
                    BooleanSetting.DUMP_COMMAND_BUFFERS.key,
                    BooleanSetting.DUMP_COMMAND_BUFFERS.defaultValue
                )
            )
        }
    }

    private fun addThemeSettings(sl: ArrayList<SettingsItem>) {
        settingsActivity.setToolbarTitle(settingsActivity.getString(R.string.preferences_theme))
        sl.apply {
            val theme: AbstractBooleanSetting = object : AbstractBooleanSetting {
                override var boolean: Boolean
                    get() = preferences.getBoolean(Settings.PREF_MATERIAL_YOU, false)
                    set(value) {
                        preferences.edit()
                            .putBoolean(Settings.PREF_MATERIAL_YOU, value)
                            .apply()
                        settingsActivity.recreate()
                    }
                override val key: String? = null
                override val section: String? = null
                override val isRuntimeEditable: Boolean = false
                override val valueAsString: String
                    get() = preferences.getBoolean(Settings.PREF_MATERIAL_YOU, false).toString()
                override val defaultValue = false
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                add(
                    SwitchSetting(
                        theme,
                        R.string.material_you,
                        R.string.material_you_description
                    )
                )
            }

            val staticThemeColor: AbstractIntSetting = object : AbstractIntSetting {
                override var int: Int
                    get() = preferences.getInt(Settings.PREF_STATIC_THEME_COLOR, 0)
                    set(value) {
                        preferences.edit()
                            .putInt(Settings.PREF_STATIC_THEME_COLOR, value)
                            .apply()
                        settingsActivity.recreate()
                    }
                override val key: String? = null
                override val section: String? = null
                override val isRuntimeEditable: Boolean = false
                override val valueAsString: String
                    get() = preferences.getInt(Settings.PREF_STATIC_THEME_COLOR, 0).toString()
                override val defaultValue: Any = 0
            }

            add(
                SingleChoiceSetting(
                    staticThemeColor,
                    R.string.static_theme_color,
                    R.string.static_theme_color_description,
                    R.array.staticThemeNames,
                    R.array.staticThemeValues
                )
            )

            val themeMode: AbstractIntSetting = object : AbstractIntSetting {
                override var int: Int
                    get() = preferences.getInt(Settings.PREF_THEME_MODE, -1)
                    set(value) {
                        preferences.edit()
                            .putInt(Settings.PREF_THEME_MODE, value)
                            .apply()
                        ThemeUtil.setThemeMode(settingsActivity)
                        settingsActivity.recreate()
                    }
                override val key: String? = null
                override val section: String? = null
                override val isRuntimeEditable: Boolean = false
                override val valueAsString: String
                    get() = preferences.getInt(Settings.PREF_THEME_MODE, -1).toString()
                override val defaultValue: Any = -1
            }

            add(
                SingleChoiceSetting(
                    themeMode,
                    R.string.change_theme_mode,
                    0,
                    R.array.themeModeEntries,
                    R.array.themeModeValues
                )
            )

            val blackBackgrounds: AbstractBooleanSetting = object : AbstractBooleanSetting {
                override var boolean: Boolean
                    get() = preferences.getBoolean(Settings.PREF_BLACK_BACKGROUNDS, false)
                    set(value) {
                        preferences.edit()
                            .putBoolean(Settings.PREF_BLACK_BACKGROUNDS, value)
                            .apply()
                        settingsActivity.recreate()
                    }
                override val key: String? = null
                override val section: String? = null
                override val isRuntimeEditable: Boolean = false
                override val valueAsString: String
                    get() = preferences.getBoolean(Settings.PREF_BLACK_BACKGROUNDS, false)
                        .toString()
                override val defaultValue: Any = false
            }

            add(
                SwitchSetting(
                    blackBackgrounds,
                    R.string.use_black_backgrounds,
                    R.string.use_black_backgrounds_description
                )
            )
        }
    }
}
