// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.fragments

import android.annotation.SuppressLint
import android.app.Activity
import android.app.ActivityManager
import android.content.Context
import android.content.DialogInterface
import android.content.Intent
import android.content.IntentFilter
import android.content.SharedPreferences
import android.content.pm.ActivityInfo
import android.net.Uri
import android.os.BatteryManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.text.Editable
import android.text.TextWatcher
import android.view.Choreographer
import android.view.Gravity
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.PopupMenu
import android.widget.TextView
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.coordinatorlayout.widget.CoordinatorLayout
import androidx.core.content.res.ResourcesCompat
import androidx.drawerlayout.widget.DrawerLayout
import androidx.drawerlayout.widget.DrawerLayout.DrawerListener
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.fragment.app.viewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.navigation.findNavController
import androidx.navigation.fragment.navArgs
import androidx.preference.PreferenceManager
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.slider.Slider
import io.github.borked3ds.android.Borked3DSApplication
import io.github.borked3ds.android.EmulationNavigationDirections
import io.github.borked3ds.android.NativeLibrary
import io.github.borked3ds.android.R
import io.github.borked3ds.android.activities.EmulationActivity
import io.github.borked3ds.android.databinding.DialogCheckboxBinding
import io.github.borked3ds.android.databinding.DialogSliderBinding
import io.github.borked3ds.android.databinding.FragmentEmulationBinding
import io.github.borked3ds.android.display.PortraitScreenLayout
import io.github.borked3ds.android.display.ScreenAdjustmentUtil
import io.github.borked3ds.android.display.ScreenLayout
import io.github.borked3ds.android.features.settings.model.BooleanSetting
import io.github.borked3ds.android.features.settings.model.IntSetting
import io.github.borked3ds.android.features.settings.model.SettingsViewModel
import io.github.borked3ds.android.features.settings.ui.SettingsActivity
import io.github.borked3ds.android.features.settings.utils.SettingsFile
import io.github.borked3ds.android.model.Game
import io.github.borked3ds.android.utils.DirectoryInitialization
import io.github.borked3ds.android.utils.DirectoryInitialization.DirectoryInitializationState
import io.github.borked3ds.android.utils.EmulationLifecycleUtil
import io.github.borked3ds.android.utils.EmulationMenuSettings
import io.github.borked3ds.android.utils.FileUtil
import io.github.borked3ds.android.utils.GameHelper
import io.github.borked3ds.android.utils.GameIconUtils
import io.github.borked3ds.android.utils.Log
import io.github.borked3ds.android.utils.ViewUtils
import io.github.borked3ds.android.viewmodel.EmulationState
import io.github.borked3ds.android.viewmodel.EmulationViewModel
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import java.io.File
import java.util.Locale

class EmulationFragment : Fragment(), SurfaceHolder.Callback, Choreographer.FrameCallback {
    private val preferences: SharedPreferences
        get() = PreferenceManager.getDefaultSharedPreferences(Borked3DSApplication.appContext)

    private lateinit var emulationState: EmulationState
    private var perfStatsUpdater: Runnable? = null

    private lateinit var emulationActivity: EmulationActivity

    private var _binding: FragmentEmulationBinding? = null
    private val binding get() = _binding!!

    private val args by navArgs<EmulationFragmentArgs>()

    private lateinit var game: Game
    private lateinit var screenAdjustmentUtil: ScreenAdjustmentUtil

    private val emulationViewModel: EmulationViewModel by activityViewModels()
    private val settingsViewModel: SettingsViewModel by viewModels()

    private var currentOrientationIndex = 0
    private val orientations = arrayOf(
        ActivityInfo.SCREEN_ORIENTATION_PORTRAIT,
        ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE,
        ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT,
        ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE
    )

    override fun onAttach(context: Context) {
        super.onAttach(context)
        if (context is EmulationActivity) {
            emulationActivity = context
            NativeLibrary.setEmulationActivity(context)
        } else {
            throw IllegalStateException("EmulationFragment must have EmulationActivity parent")
        }
    }

    private fun getCurrentOrientationIndex(): Int {
        val currentOrientation = resources.configuration.orientation
        return orientations.indexOf(currentOrientation).takeIf { it >= 0 } ?: 0
    }

    /**
     * Initialize anything that doesn't depend on the layout / views in here.
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        currentOrientationIndex = getCurrentOrientationIndex()

        val intent = requireActivity().intent
        val intentUri: Uri? = intent.data
        val oldIntentInfo = Pair(
            intent.getStringExtra("SelectedGame"),
            intent.getStringExtra("SelectedTitle")
        )
        var intentGame: Game? = null
        if (intentUri != null) {
            intentGame = if (Game.extensions.contains(FileUtil.getExtension(intentUri))) {
                GameHelper.getGame(intentUri, isInstalled = false, addedToLibrary = false)
            } else {
                null
            }
        } else if (oldIntentInfo.first != null) {
            val gameUri = Uri.parse(oldIntentInfo.first)
            intentGame = if (Game.extensions.contains(FileUtil.getExtension(gameUri))) {
                GameHelper.getGame(gameUri, isInstalled = false, addedToLibrary = false)
            } else {
                null
            }
        }

        game = args.game ?: intentGame ?: run {
            activity?.applicationContext?.let { appContext ->
                Toast.makeText(
                    appContext,
                    R.string.no_game_present,
                    Toast.LENGTH_SHORT
                ).show()
            }
            requireActivity().finish()
            return
        }

        emulationViewModel.initializeEmulationState(game.path)
        emulationState = emulationViewModel.getEmulationState()
            ?: throw IllegalStateException("EmulationState not initialized")
        emulationActivity = requireActivity() as EmulationActivity
        screenAdjustmentUtil = ScreenAdjustmentUtil(
            emulationActivity,
            emulationActivity.windowManager,
            settingsViewModel.settings
        )
        EmulationLifecycleUtil.addShutdownHook(hook = { emulationState.stop() })
        EmulationLifecycleUtil.addPauseResumeHook(hook = { togglePause() })
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentEmulationBinding.inflate(inflater)
        return binding.root
    }

    // This is using the correct scope, lint is just acting up
    @SuppressLint("UnsafeRepeatOnLifecycleDetector")
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        if (requireActivity().isFinishing) {
            return
        }
        binding.surfaceEmulation.holder.addCallback(this)
        binding.doneControlConfig.setOnClickListener {
            binding.doneControlConfig.visibility = View.GONE
            binding.surfaceInputOverlay.setIsInEditMode(false)
        }

        // Show/hide the "Stats" overlay
        updateShowPerfStatsOverlay()

        val position = IntSetting.PERF_OVERLAY_POSITION.int
        updatePerfStatsPosition(position)

        binding.drawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED)
        binding.drawerLayout.addDrawerListener(object : DrawerListener {
            override fun onDrawerSlide(drawerView: View, slideOffset: Float) {
                binding.surfaceInputOverlay.dispatchTouchEvent(
                    MotionEvent.obtain(
                        SystemClock.uptimeMillis(),
                        SystemClock.uptimeMillis() + 100,
                        MotionEvent.ACTION_UP,
                        0f,
                        0f,
                        0
                    )
                )
            }

            override fun onDrawerOpened(drawerView: View) {
                binding.drawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED)
                binding.surfaceInputOverlay.isClickable = false
                binding.surfaceInputOverlay.isFocusable = false
                binding.surfaceInputOverlay.isFocusableInTouchMode = false
            }

            override fun onDrawerClosed(drawerView: View) {
                binding.drawerLayout.setDrawerLockMode(EmulationMenuSettings.drawerLockMode)
                binding.surfaceInputOverlay.isClickable = true
                binding.surfaceInputOverlay.isFocusable = true
                binding.surfaceInputOverlay.isFocusableInTouchMode = true
            }

            override fun onDrawerStateChanged(newState: Int) {
                // No op
            }
        })

        binding.inGameMenu.menu.findItem(R.id.menu_lock_drawer).apply {
            val titleId =
                if (EmulationMenuSettings.drawerLockMode == DrawerLayout.LOCK_MODE_LOCKED_CLOSED) {
                    R.string.unlock_drawer
                } else {
                    R.string.lock_drawer
                }
            val iconId =
                if (EmulationMenuSettings.drawerLockMode == DrawerLayout.LOCK_MODE_UNLOCKED) {
                    R.drawable.ic_unlocked
                } else {
                    R.drawable.ic_lock
                }

            title = getString(titleId)
            context?.let { safeContext ->
                icon = ResourcesCompat.getDrawable(
                    resources,
                    iconId,
                    safeContext.theme
                )
            }
        }

        binding.inGameMenu.getHeaderView(0)?.apply {
            val titleView = findViewById<TextView>(R.id.text_game_title)
            val iconView = findViewById<ImageView>(R.id.game_icon)

            titleView?.text = game.title

            context?.let { safeContext ->
                GameIconUtils.loadGameIcon(requireActivity(), game, iconView)
            }
            true
        }

        binding.inGameMenu.setNavigationItemSelectedListener {
            when (it.itemId) {
                R.id.menu_emulation_pause -> {
                    if (emulationState.isPaused) {
                        emulationState.unpause()
                        it.title = resources.getString(R.string.pause_emulation)
                        context?.let { safeContext ->
                            it.icon = ResourcesCompat.getDrawable(
                                resources,
                                R.drawable.ic_pause,
                                safeContext.theme
                            )
                        }
                    } else {
                        emulationState.pause()
                        it.title = resources.getString(R.string.resume_emulation)
                        context?.let { safeContext ->
                            it.icon = ResourcesCompat.getDrawable(
                                resources,
                                R.drawable.ic_play,
                                safeContext.theme
                            )
                        }
                    }
                    true
                }

                R.id.menu_emulation_savestates -> {
                    showSavestateMenu()
                    true
                }

                R.id.menu_overlay_options -> {
                    showOverlayMenu()
                    true
                }

                R.id.menu_emulation_rotate_screen -> {
                    rotateScreen()
                    true
                }

                R.id.menu_amiibo -> {
                    showAmiiboMenu()
                    true
                }

                R.id.menu_landscape_screen_layout -> {
                    showLandscapeScreenLayoutMenu()
                    true
                }

                R.id.menu_portrait_screen_layout -> {
                    showPortraitScreenLayoutMenu()
                    true
                }

                R.id.menu_swap_screens -> {
                    screenAdjustmentUtil.swapScreen()
                    true
                }


                R.id.menu_rotate_upright -> {
                    screenAdjustmentUtil.toggleScreenUpright()
                    true
                }

                R.id.menu_lock_drawer -> {
                    when (EmulationMenuSettings.drawerLockMode) {
                        DrawerLayout.LOCK_MODE_UNLOCKED -> {
                            EmulationMenuSettings.drawerLockMode =
                                DrawerLayout.LOCK_MODE_LOCKED_CLOSED
                            it.title = resources.getString(R.string.unlock_drawer)
                            context?.let { safeContext ->
                                it.icon = ResourcesCompat.getDrawable(
                                    resources,
                                    R.drawable.ic_lock,
                                    safeContext.theme
                                )
                            }
                        }

                        DrawerLayout.LOCK_MODE_LOCKED_CLOSED -> {
                            EmulationMenuSettings.drawerLockMode = DrawerLayout.LOCK_MODE_UNLOCKED
                            it.title = resources.getString(R.string.lock_drawer)
                            context?.let { safeContext ->
                                it.icon = ResourcesCompat.getDrawable(
                                    resources,
                                    R.drawable.ic_unlocked,
                                    safeContext.theme
                                )
                            }
                        }
                    }
                    true
                }

                R.id.menu_cheats -> {
                    val action = EmulationNavigationDirections
                        .actionGlobalCheatsActivity(NativeLibrary.getRunningTitleId())
                    binding.root.findNavController().navigate(action)
                    true
                }

                R.id.menu_tweaks -> {
                    emulationActivity.displayTweaks()
                    true
                }

                R.id.menu_settings -> {
                    SettingsActivity.launch(
                        requireActivity(),
                        SettingsFile.FILE_NAME_CONFIG,
                        ""
                    )
                    true
                }

                R.id.menu_multiplayer -> {
                    emulationActivity.displayMultiplayerDialog()
                    true
                }

                R.id.menu_exit -> {
                    NativeLibrary.pauseEmulation()
                    context?.let { safeContext ->
                        MaterialAlertDialogBuilder(safeContext)
                            .setTitle(R.string.emulation_close_game)
                            .setMessage(R.string.emulation_close_game_message)
                            .setPositiveButton(android.R.string.ok) { _, _ ->
                                EmulationLifecycleUtil.closeGame()
                            }
                            .setNegativeButton(android.R.string.cancel) { _, _ ->
                                NativeLibrary.unPauseEmulation()
                            }
                            .setOnCancelListener {
                                NativeLibrary.unPauseEmulation()
                            }
                            .show()
                        true  // Explicit return for non-null context
                    } ?: false  // Return false if context is null
                }

                else -> false
            }
        }

        requireActivity().onBackPressedDispatcher.addCallback(
            viewLifecycleOwner,
            object : OnBackPressedCallback(true) {
                override fun handleOnBackPressed() {
                    if (!emulationViewModel.emulationStarted.value) {
                        return
                    }

                    if (binding.drawerLayout.isOpen) {
                        binding.drawerLayout.close()
                    } else {
                        binding.drawerLayout.open()
                    }
                }
            }
        )

        GameIconUtils.loadGameIcon(requireActivity(), game, binding.loadingImage)
        binding.loadingTitle.text = game.title

        viewLifecycleOwner.lifecycleScope.apply {
            launch {
                repeatOnLifecycle(Lifecycle.State.CREATED) {
                    emulationViewModel.shaderProgress.collectLatest {
                        if (it > 0 && it != emulationViewModel.totalShaders.value) {
                            binding.loadingProgressIndicator.isIndeterminate = false
                            binding.loadingProgressText.visibility = View.VISIBLE
                            binding.loadingProgressText.text = String.format(
                                Locale.ROOT,
                                "%d/%d",
                                emulationViewModel.shaderProgress.value,
                                emulationViewModel.totalShaders.value
                            )

                            if (it < binding.loadingProgressIndicator.max) {
                                binding.loadingProgressIndicator.progress = it
                            }
                        }

                        if (it == emulationViewModel.totalShaders.value) {
                            binding.loadingText.setText(R.string.loading)
                            binding.loadingProgressIndicator.isIndeterminate = true
                            binding.loadingProgressText.visibility = View.GONE
                        }
                    }
                }
            }
            launch {
                repeatOnLifecycle(Lifecycle.State.CREATED) {
                    emulationViewModel.totalShaders.collectLatest {
                        binding.loadingProgressIndicator.max = it
                    }
                }
            }
            launch {
                repeatOnLifecycle(Lifecycle.State.CREATED) {
                    emulationViewModel.shaderMessage.collectLatest {
                        if (it != "") {
                            binding.loadingText.text = it
                        }
                    }
                }
            }
            launch {
                repeatOnLifecycle(Lifecycle.State.CREATED) {
                    emulationViewModel.emulationStarted.collectLatest { started ->
                        if (started) {
                            ViewUtils.hideView(binding.loadingIndicator)
                            ViewUtils.showView(binding.surfaceInputOverlay)
                            binding.inGameMenu.menu.findItem(R.id.menu_emulation_savestates).isVisible =
                                NativeLibrary.getSavestateInfo() != null
                            binding.drawerLayout.setDrawerLockMode(EmulationMenuSettings.drawerLockMode)
                        }
                    }
                }
            }
        }
    }

    private fun rotateScreen() {
        val activity = context as? Activity ?: return
        val currentOrientation = activity.requestedOrientation
        val currentIndex = orientations.indexOf(currentOrientation).takeIf { it != -1 } ?: 0
        val newIndex = (currentIndex + 1) % orientations.size
        activity.requestedOrientation = orientations[newIndex]
        IntSetting.ORIENTATION_OPTION.int = orientations[newIndex]
        settingsViewModel.settings.saveSetting(
            IntSetting.ORIENTATION_OPTION,
            SettingsFile.FILE_NAME_CONFIG
        )
        currentOrientationIndex = newIndex // Keep the index in sync for any other uses
    }

    fun isDrawerOpen(): Boolean {
        return binding.drawerLayout.isOpen
    }

    private fun togglePause() {
        if (emulationState.isPaused) {
            emulationState.unpause()
        } else {
            emulationState.pause()
        }
    }

    override fun onResume() {
        super.onResume()
        Choreographer.getInstance().postFrameCallback(this)
        if (NativeLibrary.isRunning()) {
            NativeLibrary.unPauseEmulation()
            binding.inGameMenu.menu.findItem(R.id.menu_emulation_pause)?.let { menuItem ->
                menuItem.title = resources.getString(R.string.pause_emulation)
                context?.let { safeContext ->
                    menuItem.icon = ResourcesCompat.getDrawable(
                        resources,
                        R.drawable.ic_pause,
                        safeContext.theme
                    )
                }
            }

            val position = IntSetting.PERF_OVERLAY_POSITION.int
            updatePerfStatsPosition(position)

            return
        }

        if (DirectoryInitialization.areBorked3DSDirectoriesReady()) {
            emulationState.run(emulationActivity.isActivityRecreated ?: false)
        } else {
            setupBorked3DSDirectoriesThenStartEmulation()
        }
    }

    override fun onPause() {
        if (NativeLibrary.isRunning()) {
            emulationState.pause()
        }
        Choreographer.getInstance().removeFrameCallback(this)
        super.onPause()
    }

    override fun onDetach() {
        NativeLibrary.clearEmulationActivity()
        super.onDetach()
    }

    private fun setupBorked3DSDirectoriesThenStartEmulation() {
        val directoryInitializationState = DirectoryInitialization.start()
        if (!isAdded) return // Check if fragment is attached

        activity?.applicationContext?.let { appContext ->
            when (directoryInitializationState) {
                DirectoryInitializationState.BORKED3DS_DIRECTORIES_INITIALIZED -> {
                    emulationState.run(emulationActivity.isActivityRecreated)
                }

                DirectoryInitializationState.EXTERNAL_STORAGE_PERMISSION_NEEDED -> {
                    Toast.makeText(appContext, R.string.write_permission_needed, Toast.LENGTH_SHORT)
                        .show()
                }

                DirectoryInitializationState.CANT_FIND_EXTERNAL_STORAGE -> {
                    Toast.makeText(
                        appContext,
                        R.string.external_storage_not_mounted,
                        Toast.LENGTH_SHORT
                    )
                        .show()
                }

                else -> {
                    // Handle any other states if necessary
                    Toast.makeText(appContext, R.string.unknown_error, Toast.LENGTH_SHORT).show()
                }
            }
        }
    }

    private fun showSavestateMenu() {
        context?.let { safeContext ->
            val popupMenu = PopupMenu(
                safeContext,
                binding.inGameMenu.findViewById(R.id.menu_emulation_savestates)
            )

            popupMenu.menuInflater.inflate(R.menu.menu_savestates, popupMenu.menu)

            popupMenu.setOnMenuItemClickListener {
                when (it.itemId) {
                    R.id.menu_emulation_save_state -> {
                        showStateSubmenu(true)
                        true
                    }

                    R.id.menu_emulation_load_state -> {
                        showStateSubmenu(false)
                        true
                    }

                    else -> true
                }
            }

            popupMenu.show()
        }
    }

    private fun showStateSubmenu(isSaving: Boolean) {
        val savestates = NativeLibrary.getSavestateInfo()

        context?.let { safeContext ->
            val popupMenu = PopupMenu(
                safeContext,
                binding.inGameMenu.findViewById(R.id.menu_emulation_savestates)
            )

            popupMenu.menu.apply {
                for (i in 0 until NativeLibrary.SAVESTATE_SLOT_COUNT) {
                    val slot = i
                    var enableClick = isSaving
                    val text = if (slot == NativeLibrary.QUICKSAVE_SLOT) {
                        getString(R.string.emulation_quicksave_slot)
                    } else {
                        getString(R.string.emulation_empty_state_slot, slot)
                    }

                    add(text).setEnabled(enableClick).setOnMenuItemClickListener {
                        if (isSaving) {
                            NativeLibrary.saveState(slot)
                            activity?.applicationContext?.let { appContext ->
                                Toast.makeText(
                                    appContext,
                                    getString(R.string.quicksave_saving),
                                    Toast.LENGTH_SHORT
                                ).show()
                            }
                        } else {
                            NativeLibrary.loadState(slot)
                            binding.drawerLayout.close()
                            activity?.applicationContext?.let { appContext ->
                                Toast.makeText(
                                    appContext,
                                    getString(R.string.quickload_loading),
                                    Toast.LENGTH_SHORT
                                ).show()
                            }
                        }
                        true
                    }
                }
            }

            savestates?.forEach {
                val text = if (it.slot == NativeLibrary.QUICKSAVE_SLOT) {
                    getString(R.string.emulation_occupied_quicksave_slot, it.time)
                } else {
                    getString(R.string.emulation_occupied_state_slot, it.slot, it.time)
                }
                popupMenu.menu.getItem(it.slot).setTitle(text).isEnabled = true
            }

            popupMenu.show()
        }
    }

    private fun showLoadStateSubmenu() {
        val savestates = NativeLibrary.getSavestateInfo()

        context?.let { safeContext ->
            val popupMenu = PopupMenu(
                safeContext,
                binding.inGameMenu.findViewById(R.id.menu_emulation_savestates)
            )

            popupMenu.menu.apply {
                for (i in 0 until NativeLibrary.SAVESTATE_SLOT_COUNT) {
                    val slot = i + 1
                    val text = getString(R.string.emulation_empty_state_slot, slot)
                    add(text).setEnabled(false).setOnMenuItemClickListener {
                        NativeLibrary.loadState(slot)
                        true
                    }
                }
            }

            savestates?.forEach {
                val text = getString(R.string.emulation_occupied_state_slot, it.slot, it.time)
                popupMenu.menu.getItem(it.slot - 1).setTitle(text).isEnabled = true
            }

            popupMenu.show()
        }
    }

    private fun displaySavestateWarning() {
        if (preferences.getBoolean("savestateWarningShown", false)) {
            return
        }

        val dialogCheckboxBinding = DialogCheckboxBinding.inflate(layoutInflater)
        context?.let { safeContext ->
            MaterialAlertDialogBuilder(safeContext)
                .setTitle(R.string.savestates)
                .setMessage(R.string.savestate_warning_message)
                .setView(dialogCheckboxBinding.root)
                .setPositiveButton(android.R.string.ok) { _: DialogInterface?, _: Int ->
                    preferences.edit()
                        .putBoolean(
                            "savestateWarningShown",
                            dialogCheckboxBinding.checkBox.isChecked
                        )
                        .apply()
                }
                .show()
        }
    }

    private fun showOverlayMenu() {
        context?.let { safeContext ->
            val popupMenu = PopupMenu(
                safeContext,
                binding.inGameMenu.findViewById(R.id.menu_overlay_options)
            )

            popupMenu.menuInflater.inflate(R.menu.menu_overlay_options, popupMenu.menu)

            popupMenu.menu.apply {
                findItem(R.id.menu_show_overlay).isChecked = EmulationMenuSettings.showOverlay
                findItem(R.id.menu_show_perf_overlay).isChecked =
                    EmulationMenuSettings.showPerfStatsOverlay
                findItem(R.id.menu_haptic_feedback).isChecked = EmulationMenuSettings.hapticFeedback
                findItem(R.id.menu_emulation_joystick_rel_center).isChecked =
                    EmulationMenuSettings.joystickRelCenter
                findItem(R.id.menu_emulation_dpad_slide_enable).isChecked =
                    EmulationMenuSettings.dpadSlide
            }

            popupMenu.setOnMenuItemClickListener {
                when (it.itemId) {
                    R.id.menu_show_overlay -> {
                        EmulationMenuSettings.showOverlay = !EmulationMenuSettings.showOverlay
                        binding.surfaceInputOverlay.refreshControls()
                        true
                    }

                    R.id.menu_show_perf_overlay -> {
                        EmulationMenuSettings.showPerfStatsOverlay =
                            !EmulationMenuSettings.showPerfStatsOverlay
                        updateShowPerfStatsOverlay()
                        true
                    }

                    R.id.menu_haptic_feedback -> {
                        EmulationMenuSettings.hapticFeedback = !EmulationMenuSettings.hapticFeedback
                        updateShowPerfStatsOverlay()
                        true
                    }

                    R.id.menu_emulation_edit_layout -> {
                        editControlsPlacement()
                        binding.drawerLayout.close()
                        true
                    }

                    R.id.menu_emulation_toggle_controls -> {
                        showToggleControlsDialog()
                        true
                    }

                    R.id.menu_emulation_adjust_scale_reset_all -> {
                        resetAllScales()
                        true
                    }

                    R.id.menu_emulation_adjust_scale -> {
                        showAdjustScaleDialog("controlScale")
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_a -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_A)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_b -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_B)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_x -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_X)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_y -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_Y)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_l -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.TRIGGER_L)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_r -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.TRIGGER_R)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_zl -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_ZL)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_zr -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_ZR)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_start -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_START)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_select -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_SELECT)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_controller_dpad -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.DPAD)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_controller_circlepad -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.STICK_LEFT)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_controller_c -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.STICK_C)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_home -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_HOME)
                        true
                    }

                    R.id.menu_emulation_adjust_scale_button_swap -> {
                        showAdjustScaleDialog("controlScale-" + NativeLibrary.ButtonType.BUTTON_SWAP)
                        true
                    }

                    R.id.menu_emulation_adjust_opacity -> {
                        showAdjustOpacityDialog()
                        true
                    }

                    R.id.menu_emulation_joystick_rel_center -> {
                        EmulationMenuSettings.joystickRelCenter =
                            !EmulationMenuSettings.joystickRelCenter
                        true
                    }

                    R.id.menu_emulation_dpad_slide_enable -> {
                        EmulationMenuSettings.dpadSlide = !EmulationMenuSettings.dpadSlide
                        true
                    }

                    R.id.menu_emulation_reset_overlay -> {
                        showResetOverlayDialog()
                        true
                    }

                    else -> true
                }
            }

            popupMenu.show()
        }
    }

    private fun showAmiiboMenu() {
        context?.let { safeContext ->
            val popupMenu = PopupMenu(
                safeContext,
                binding.inGameMenu.findViewById(R.id.menu_amiibo)
            )

            popupMenu.menuInflater.inflate(R.menu.menu_amiibo_options, popupMenu.menu)

            popupMenu.setOnMenuItemClickListener {
                when (it.itemId) {
                    R.id.menu_emulation_amiibo_load -> {
                        emulationActivity.openFileLauncher.launch(false)
                        true
                    }

                    R.id.menu_emulation_amiibo_remove -> {
                        NativeLibrary.removeAmiibo()
                        true
                    }

                    else -> true
                }
            }

            popupMenu.show()
        }
    }

    private fun showLandscapeScreenLayoutMenu() {
        context?.let { safeContext ->
            val popupMenu = PopupMenu(
                safeContext,
                binding.inGameMenu.findViewById(R.id.menu_landscape_screen_layout)
            )

            popupMenu.menuInflater.inflate(R.menu.menu_landscape_screen_layout, popupMenu.menu)

            val layoutOptionMenuItem = when (IntSetting.SCREEN_LAYOUT.int) {
                ScreenLayout.ORIGINAL.int -> R.id.menu_screen_layout_original
                ScreenLayout.SINGLE_SCREEN.int -> R.id.menu_screen_layout_single
                ScreenLayout.SIDE_SCREEN.int -> R.id.menu_screen_layout_sidebyside
                ScreenLayout.HYBRID_SCREEN.int -> R.id.menu_screen_layout_hybrid
                ScreenLayout.CUSTOM_LAYOUT.int -> R.id.menu_screen_layout_custom
                else -> R.id.menu_screen_layout_largescreen
            }
            popupMenu.menu.findItem(layoutOptionMenuItem).isChecked = true

            popupMenu.setOnMenuItemClickListener {
                when (it.itemId) {
                    R.id.menu_screen_layout_largescreen -> {
                        screenAdjustmentUtil.changeScreenOrientation(ScreenLayout.LARGE_SCREEN.int)
                        true
                    }

                    R.id.menu_screen_layout_single -> {
                        screenAdjustmentUtil.changeScreenOrientation(ScreenLayout.SINGLE_SCREEN.int)
                        true
                    }

                    R.id.menu_screen_layout_sidebyside -> {
                        screenAdjustmentUtil.changeScreenOrientation(ScreenLayout.SIDE_SCREEN.int)
                        true
                    }

                    R.id.menu_screen_layout_hybrid -> {
                        screenAdjustmentUtil.changeScreenOrientation(ScreenLayout.HYBRID_SCREEN.int)
                        true
                    }

                    R.id.menu_screen_layout_original -> {
                        screenAdjustmentUtil.changeScreenOrientation(ScreenLayout.ORIGINAL.int)
                        true
                    }

                    R.id.menu_screen_layout_custom -> {
                        activity?.applicationContext?.let { appContext ->
                            Toast.makeText(
                                appContext,
                                R.string.emulation_adjust_custom_layout,
                                Toast.LENGTH_LONG
                            ).show()
                        }
                        screenAdjustmentUtil.changeScreenOrientation(ScreenLayout.CUSTOM_LAYOUT.int)
                        true
                    }

                    else -> true
                }
            }

            popupMenu.show()
        }
    }

    private fun showPortraitScreenLayoutMenu() {
        context?.let { safeContext ->
            val popupMenu = PopupMenu(
                safeContext,
                binding.inGameMenu.findViewById(R.id.menu_portrait_screen_layout)
            )

            popupMenu.menuInflater.inflate(R.menu.menu_portrait_screen_layout, popupMenu.menu)

            val layoutOptionMenuItem = when (IntSetting.PORTRAIT_SCREEN_LAYOUT.int) {
                PortraitScreenLayout.TOP_FULL_WIDTH.int -> R.id.menu_portrait_layout_top_full
                PortraitScreenLayout.CUSTOM_PORTRAIT_LAYOUT.int -> R.id.menu_portrait_layout_custom
                PortraitScreenLayout.ORIGINAL.int -> R.id.menu_portrait_layout_original
                else -> R.id.menu_portrait_layout_top_full
            }

            popupMenu.menu.findItem(layoutOptionMenuItem).isChecked = true

            popupMenu.setOnMenuItemClickListener {
                when (it.itemId) {
                    R.id.menu_portrait_layout_top_full -> {
                        screenAdjustmentUtil.changePortraitOrientation(PortraitScreenLayout.TOP_FULL_WIDTH.int)
                        true
                    }

                    R.id.menu_portrait_layout_custom -> {
                        activity?.applicationContext?.let { appContext ->
                            Toast.makeText(
                                appContext,
                                R.string.emulation_adjust_custom_layout,
                                Toast.LENGTH_LONG
                            ).show()
                        }
                        screenAdjustmentUtil.changePortraitOrientation(PortraitScreenLayout.CUSTOM_PORTRAIT_LAYOUT.int)
                        true
                    }

                    R.id.menu_portrait_layout_original -> {
                        screenAdjustmentUtil.changePortraitOrientation(PortraitScreenLayout.ORIGINAL.int)
                        true
                    }

                    else -> true
                }
            }

            popupMenu.show()
        }
    }

    private fun editControlsPlacement() {
        if (binding.surfaceInputOverlay.isInEditMode) {
            binding.doneControlConfig.visibility = View.GONE
            binding.surfaceInputOverlay.setIsInEditMode(false)
        } else {
            binding.doneControlConfig.visibility = View.VISIBLE
            binding.surfaceInputOverlay.setIsInEditMode(true)
        }
    }

    private fun showToggleControlsDialog() {
        val editor = preferences.edit()
        val enabledButtons = BooleanArray(15)
        enabledButtons.forEachIndexed { i: Int, _: Boolean ->
            // Buttons that are disabled by default
            var defaultValue = true
            when (i) {
                6, 7, 12, 13, 14 -> defaultValue = false
            }
            enabledButtons[i] = preferences.getBoolean("buttonToggle$i", defaultValue)
        }

        context?.let { safeContext ->
            MaterialAlertDialogBuilder(safeContext)
                .setTitle(R.string.emulation_toggle_controls)
                .setMultiChoiceItems(
                    R.array.n3dsButtons, enabledButtons
                ) { _: DialogInterface?, indexSelected: Int, isChecked: Boolean ->
                    editor.putBoolean("buttonToggle$indexSelected", isChecked)
                }
                .setPositiveButton(android.R.string.ok) { _: DialogInterface?, _: Int ->
                    editor.apply()
                    binding.surfaceInputOverlay.refreshControls()
                }
                .show()
        }
    }

    private fun showAdjustScaleDialog(target: String) {
        val sliderBinding = DialogSliderBinding.inflate(layoutInflater)

        sliderBinding.apply {
            slider.valueTo = 150f
            slider.valueFrom = 0f
            slider.value = preferences.getInt(target, 50).toFloat()
            textValue.setText(String.format(Locale.ROOT, "%d", (slider.value + 50).toInt()))
            textValue.addTextChangedListener(object : TextWatcher {
                override fun afterTextChanged(s: Editable) {
                    val value = s.toString().toIntOrNull()
                    if (value == null || value < 50 || value > 150) {
                        textInput.error = "Inappropriate Value"
                    } else {
                        textInput.error = null
                        slider.value = value.toFloat() - 50
                    }
                }

                override fun beforeTextChanged(p0: CharSequence?, p1: Int, p2: Int, p3: Int) {}
                override fun onTextChanged(p0: CharSequence?, p1: Int, p2: Int, p3: Int) {}
            })
            slider.addOnChangeListener(
                Slider.OnChangeListener { slider: Slider, progress: Float, _: Boolean ->
                    if (textValue.text.toString() != (slider.value + 50).toInt().toString()) {
                        textValue.setText(
                            String.format(
                                Locale.ROOT,
                                "%d",
                                (slider.value + 50).toInt()
                            )
                        )
                        textValue.setSelection(textValue.length())
                        setControlScale(slider.value.toInt(), target)
                    }
                })
            textInput.suffixText = "%"
        }
        val previousProgress = sliderBinding.slider.value.toInt()

        context?.let { safeContext ->
            MaterialAlertDialogBuilder(safeContext)
                .setTitle(R.string.emulation_control_scale)
                .setView(sliderBinding.root)
                .setNegativeButton(android.R.string.cancel) { _: DialogInterface?, _: Int ->
                    setControlScale(previousProgress, target)
                }
                .setPositiveButton(android.R.string.ok) { _: DialogInterface?, _: Int ->
                    setControlScale(sliderBinding.slider.value.toInt(), target)
                }
                .setNeutralButton(R.string.slider_default) { _: DialogInterface?, _: Int ->
                    setControlScale(50, target)
                }
                .show()
        }
    }

    private fun showAdjustOpacityDialog() {
        val sliderBinding = DialogSliderBinding.inflate(layoutInflater)

        sliderBinding.apply {
            slider.valueFrom = 0f
            slider.valueTo = 100f
            slider.value = preferences.getInt("controlOpacity", 50).toFloat()
            textValue.setText(String.format(Locale.ROOT, "%d", (slider.value + 50).toInt()))

            textValue.addTextChangedListener(object : TextWatcher {
                override fun afterTextChanged(s: Editable) {
                    val value = s.toString().toIntOrNull()
                    if (value == null || value < slider.valueFrom || value > slider.valueTo) {
                        textInput.error = "Inappropriate Value"
                    } else {
                        textInput.error = null
                        slider.value = value.toFloat()
                    }
                }

                override fun beforeTextChanged(p0: CharSequence?, p1: Int, p2: Int, p3: Int) {}
                override fun onTextChanged(p0: CharSequence?, p1: Int, p2: Int, p3: Int) {}
            })

            slider.addOnChangeListener { _: Slider, value: Float, _: Boolean ->
                if (textValue.text.toString() != slider.value.toInt().toString()) {
                    textValue.setText(String.format(Locale.ROOT, "%d", slider.value.toInt()))
                    textValue.setSelection(textValue.length())
                    setControlOpacity(slider.value.toInt())
                }
            }

            textInput.suffixText = "%"
        }
        val previousProgress = sliderBinding.slider.value.toInt()

        context?.let { safeContext ->
            MaterialAlertDialogBuilder(safeContext)
                .setTitle(R.string.emulation_control_opacity)
                .setView(sliderBinding.root)
                .setNegativeButton(android.R.string.cancel) { _: DialogInterface?, _: Int ->
                    setControlOpacity(previousProgress)
                }
                .setPositiveButton(android.R.string.ok) { _: DialogInterface?, _: Int ->
                    setControlOpacity(sliderBinding.slider.value.toInt())
                }
                .setNeutralButton(R.string.slider_default) { _: DialogInterface?, _: Int ->
                    setControlOpacity(50)
                }
                .show()
        }
    }

    private fun setControlScale(scale: Int, target: String) {
        preferences.edit()
            .putInt(target, scale)
            .apply()
        binding.surfaceInputOverlay.refreshControls()
    }

    private fun resetScale(target: String) {
        preferences.edit().putInt(
            target,
            100
        ).apply()
    }

    private fun resetAllScales() {
        resetScale("controlScale")
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_A)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_B)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_X)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_Y)
        resetScale("controlScale-" + NativeLibrary.ButtonType.TRIGGER_L)
        resetScale("controlScale-" + NativeLibrary.ButtonType.TRIGGER_R)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_ZL)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_ZR)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_START)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_SELECT)
        resetScale("controlScale-" + NativeLibrary.ButtonType.DPAD)
        resetScale("controlScale-" + NativeLibrary.ButtonType.STICK_LEFT)
        resetScale("controlScale-" + NativeLibrary.ButtonType.STICK_C)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_HOME)
        resetScale("controlScale-" + NativeLibrary.ButtonType.BUTTON_SWAP)
        binding.surfaceInputOverlay.refreshControls()
    }

    private fun setControlOpacity(opacity: Int) {
        preferences.edit()
            .putInt("controlOpacity", opacity)
            .apply()
        binding.surfaceInputOverlay.refreshControls()
    }

    private fun showResetOverlayDialog() {
        context?.let { safeContext ->
            MaterialAlertDialogBuilder(safeContext)
                .setTitle(getString(R.string.emulation_touch_overlay_reset))
                .setPositiveButton(android.R.string.ok) { _: DialogInterface?, _: Int ->
                    resetInputOverlay()
                }
                .setNegativeButton(android.R.string.cancel, null)
                .show()
        }
    }

    private fun resetInputOverlay() {
        resetAllScales()
        preferences.edit()
            .putInt("controlOpacity", 50)
            .apply()

        val editor = preferences.edit()
        for (i in 0 until 15) {
            var defaultValue = true
            when (i) {
                6, 7, 12, 13, 14 -> defaultValue = false
            }
            editor.putBoolean("buttonToggle$i", defaultValue)
        }
        editor.apply()

        binding.surfaceInputOverlay.resetButtonPlacement()
    }

    private fun updateShowPerfStatsOverlay() {
        if (EmulationMenuSettings.showPerfStatsOverlay) {
            val FPS = 1
            val FRAMETIME = 2
            val SPEED = 3
            perfStatsUpdater = Runnable {
                val sb = StringBuilder()
                val perfStats = NativeLibrary.getPerfStats()
                if (perfStats[FPS] > 0) {
                    if (BooleanSetting.SHOW_FPS.boolean) {
                        sb.append(
                            String.format(
                                "FPS: %d",
                                (perfStats[FPS] + 0.5).toInt()
                            )
                        )
                    }

                    if (BooleanSetting.SHOW_FRAMETIME.boolean) {
                        if (sb.isNotEmpty()) sb.append(" | ")
                        sb.append(
                            String.format(
                                "FT: %.2fms",
                                (perfStats[FRAMETIME] * 1000.0f).toFloat()
                            )
                        )
                    }

                    if (BooleanSetting.SHOW_SPEED.boolean) {
                        if (sb.isNotEmpty()) sb.append(" | ")
                        sb.append(
                            String.format(
                                "Speed: %d%%",
                                (perfStats[SPEED] * 100.0 + 0.5).toInt()
                            )
                        )
                    }

                    if (BooleanSetting.SHOW_APP_RAM_USAGE.boolean) {
                        if (sb.isNotEmpty()) sb.append(" | ")
                        val appRamUsage =
                            File("/proc/self/statm").readLines()[0].split(' ')[1].toLong() * 4096 / 1000000
                        sb.append("Process RAM: $appRamUsage MB")
                    }

                    if (BooleanSetting.SHOW_SYSTEM_RAM_USAGE.boolean) {
                        if (sb.isNotEmpty()) sb.append(" | ")
                        context?.let { ctx ->
                            val activityManager =
                                ctx.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
                            val memInfo = ActivityManager.MemoryInfo()
                            activityManager.getMemoryInfo(memInfo)
                            val usedRamMB = (memInfo.totalMem - memInfo.availMem) / 1048576L
                            sb.append("RAM: $usedRamMB MB")
                        }
                    }

                    if (BooleanSetting.SHOW_BAT_TEMPERATURE.boolean) {
                        if (sb.isNotEmpty()) sb.append(" | ")
                        val batteryTemp = getBatteryTemperature()
                        val tempF = celsiusToFahrenheit(batteryTemp)
                        sb.append(String.format("%.1f°C/%.1f°F", batteryTemp, tempF))
                    }

                    if (isAdded && BooleanSetting.OVERLAY_BACKGROUND.boolean) {
                        binding.showPerfOverlayText.setBackgroundResource(R.color.borked3ds_transparent_black_50)
                    } else {
                        binding.showPerfOverlayText.setBackgroundResource(0)
                    }

                    if (isAdded) {
                        binding.showPerfOverlayText.text = sb.toString()
                    }
                }
                perfStatsUpdateHandler.postDelayed(perfStatsUpdater!!, 3000)
            }
            perfStatsUpdateHandler.post(perfStatsUpdater!!)

            binding.showPerfOverlayText.visibility = View.VISIBLE
        } else {
            if (perfStatsUpdater != null) {
                perfStatsUpdateHandler.removeCallbacks(perfStatsUpdater!!)
            }
            binding.showPerfOverlayText.visibility = View.GONE
        }
    }

    private fun updatePerfStatsPosition(position: Int) {
        val params = binding.showPerfOverlayText.layoutParams as CoordinatorLayout.LayoutParams
        when (position) {
            0 -> {
                params.gravity = (Gravity.TOP or Gravity.START)
                params.setMargins(resources.getDimensionPixelSize(R.dimen.spacing_large), 0, 0, 0)
            }

            1 -> {
                params.gravity = (Gravity.TOP or Gravity.CENTER_HORIZONTAL)
            }

            2 -> {
                params.gravity = (Gravity.TOP or Gravity.END)
                params.setMargins(0, 0, resources.getDimensionPixelSize(R.dimen.spacing_large), 0)
            }

            3 -> {
                params.gravity = (Gravity.BOTTOM or Gravity.START)
                params.setMargins(resources.getDimensionPixelSize(R.dimen.spacing_large), 0, 0, 0)
            }

            4 -> {
                params.gravity = (Gravity.BOTTOM or Gravity.CENTER_HORIZONTAL)
            }

            5 -> {
                params.gravity = (Gravity.BOTTOM or Gravity.END)
                params.setMargins(0, 0, resources.getDimensionPixelSize(R.dimen.spacing_large), 0)
            }
        }

        binding.showPerfOverlayText.layoutParams = params
    }

    private fun getBatteryTemperature(): Float {
        return try {
            val context = context ?: return 0f
            val batteryIntent =
                context.registerReceiver(null, IntentFilter(Intent.ACTION_BATTERY_CHANGED))
            // Temperature in tenths of a degree Celsius
            val temperature = batteryIntent?.getIntExtra(BatteryManager.EXTRA_TEMPERATURE, 0) ?: 0
            // Convert to degrees Celsius
            temperature / 10.0f
        } catch (e: Exception) {
            Log.error("[EmulationFragment] Failed to get battery temperature: ${e.message}")
            0.0f
        }
    }

    private fun celsiusToFahrenheit(celsius: Float): Float {
        return (celsius * 9 / 5) + 32
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        emulationViewModel.onSurfaceChanged(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.debug("[EmulationFragment] Surface changed. Resolution: $width x $height")
        emulationViewModel.onSurfaceChanged(holder.surface)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        emulationState?.clearSurface()
    }

    override fun doFrame(frameTimeNanos: Long) {
        Choreographer.getInstance().postFrameCallback(this)
        NativeLibrary.doFrame()
    }

    override fun onDestroyView() {
        super.onDestroyView()
        perfStatsUpdater?.let {
            // Cleanup perfStatsUpdater
            perfStatsUpdateHandler.removeCallbacks(it)
        }
        _binding = null
    }

    companion object {
        private val perfStatsUpdateHandler = Handler(Looper.myLooper()!!)
    }
}
