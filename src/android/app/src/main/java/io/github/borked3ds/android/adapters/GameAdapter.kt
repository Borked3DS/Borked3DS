// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.adapters

import android.content.Context
import android.content.Intent
import android.content.pm.ShortcutInfo
import android.content.pm.ShortcutManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.drawable.BitmapDrawable
import android.graphics.drawable.Icon
import android.net.Uri
import android.os.SystemClock
import android.text.TextUtils
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.PopupMenu
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.ActivityResultLauncher
import androidx.appcompat.app.AppCompatActivity
import androidx.core.net.toUri
import androidx.core.graphics.scale
import androidx.core.content.edit
import androidx.documentfile.provider.DocumentFile
import androidx.lifecycle.ViewModelProvider
import androidx.navigation.findNavController
import androidx.preference.PreferenceManager
import androidx.recyclerview.widget.AsyncDifferConfig
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.bottomsheet.BottomSheetBehavior
import com.google.android.material.bottomsheet.BottomSheetDialog
import com.google.android.material.button.MaterialButton
import com.google.android.material.color.MaterialColors
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import io.github.borked3ds.android.Borked3DSApplication
import io.github.borked3ds.android.HomeNavigationDirections
import io.github.borked3ds.android.R
import io.github.borked3ds.android.adapters.GameAdapter.GameViewHolder
import io.github.borked3ds.android.databinding.CardGameBinding
import io.github.borked3ds.android.databinding.DialogShortcutBinding
import io.github.borked3ds.android.features.cheats.ui.CheatsFragmentDirections
import io.github.borked3ds.android.fragments.IndeterminateProgressDialogFragment
import io.github.borked3ds.android.model.Game
import io.github.borked3ds.android.utils.FileUtil
import io.github.borked3ds.android.utils.GameIconUtils
import io.github.borked3ds.android.utils.PlayTimeTracker
import io.github.borked3ds.android.viewmodel.GamesViewModel
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

sealed class GameListItem {
    data class GameItem(val game: Game) : GameListItem()
    data object Separator : GameListItem()
}

class GameAdapter(
    private val activity: AppCompatActivity,
    private val layoutInflater: LayoutInflater,
    private val openImageLauncher: ActivityResultLauncher<String>?
) : ListAdapter<GameListItem, RecyclerView.ViewHolder>(
    AsyncDifferConfig.Builder(DiffCallback()).build()
),
    View.OnClickListener, View.OnLongClickListener {

    private val gameView = 0
    private val favoriteGameView = 1
    private var lastClickTime = 0L
    private var imagePath: String? = null
    private var dialogShortcutBinding: DialogShortcutBinding? = null

    fun handleShortcutImageResult(uri: Uri?) {
        val path = uri?.toString()
        if (path != null) {
            imagePath = path
            dialogShortcutBinding!!.imageScaleSwitch.isEnabled = imagePath != null
            refreshShortcutDialogIcon()
        }
    }

    override fun getItemViewType(position: Int): Int {
        return when (currentList[position]) {
            is GameListItem.GameItem -> gameView
            is GameListItem.Separator -> favoriteGameView
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): RecyclerView.ViewHolder {
        return when (viewType) {
            gameView -> {
                val binding = CardGameBinding.inflate(layoutInflater, parent, false)
                binding.cardGame.setOnClickListener(this)
                binding.cardGame.setOnLongClickListener(this)
                GameViewHolder(binding)
            }

            favoriteGameView -> {
                val view = layoutInflater.inflate(R.layout.list_item_separator, parent, false)
                SeparatorViewHolder(view)
            }

            else -> throw IllegalArgumentException("Invalid view type")
        }
    }

    override fun onBindViewHolder(holder: RecyclerView.ViewHolder, position: Int) {
        when (holder) {
            is GameViewHolder -> holder.bind((currentList[position] as GameListItem.GameItem).game)
        }
    }

    override fun getItemCount(): Int = currentList.size

    /**
     * Launches the game that was clicked on.
     *
     * @param view The card representing the game the user wants to play.
     */
    override fun onClick(view: View) {
        // Double-click prevention, using threshold of 1000 ms
        if (SystemClock.elapsedRealtime() - lastClickTime < 1000) {
            return
        }
        lastClickTime = SystemClock.elapsedRealtime()

        val holder = view.tag as GameViewHolder
        gameExists(holder)

        val preferences =
            PreferenceManager.getDefaultSharedPreferences(Borked3DSApplication.appContext)
        preferences.edit()
            .putLong(
                holder.game.keyLastPlayedTime,
                System.currentTimeMillis()
            )
            .apply()

        val action = HomeNavigationDirections.actionGlobalEmulationActivity(holder.game)
        view.findNavController().navigate(action)
    }

    /**
     * Opens the about game dialog for the game that was clicked on.
     *
     * @param view The view representing the game the user wants to play.
     */
    override fun onLongClick(view: View): Boolean {
        val context = view.context
        val holder = view.tag as GameViewHolder
        gameExists(holder)

        if (holder.game.titleId == 0L) {
            MaterialAlertDialogBuilder(context)
                .setTitle(R.string.properties)
                .setMessage(R.string.properties_not_loaded)
                .setPositiveButton(android.R.string.ok, null)
                .show()
        } else {
            showAboutGameDialog(context, holder.game, holder, view)
        }
        return true
    }

    // Triggers a library refresh if the user clicks on stale data
    private fun gameExists(holder: GameViewHolder): Boolean {
        if (holder.game.isInstalled) {
            return true
        }

        val gameExists = DocumentFile.fromSingleUri(
            Borked3DSApplication.appContext,
            Uri.parse(holder.game.path)
        )?.exists() == true
        return if (!gameExists) {
            Toast.makeText(
                Borked3DSApplication.appContext,
                R.string.loader_error_file_not_found,
                Toast.LENGTH_LONG
            ).show()

            ViewModelProvider(activity)[GamesViewModel::class.java].reloadGames(true)
            false
        } else {
            true
        }
    }

    inner class GameViewHolder(val binding: CardGameBinding) :
        RecyclerView.ViewHolder(binding.root) {
        lateinit var game: Game

        init {
            binding.cardGame.tag = this
        }

        fun bind(game: Game) {
            this.game = game

            binding.imageGameScreen.scaleType = ImageView.ScaleType.CENTER_CROP
            GameIconUtils.loadGameIcon(activity, game, binding.imageGameScreen)

            binding.textGameTitle.visibility = if (game.title.isEmpty()) {
                View.GONE
            } else {
                View.VISIBLE
            }
            binding.textCompany.visibility = if (game.company.isEmpty()) {
                View.GONE
            } else {
                View.VISIBLE
            }
            binding.textGameId.visibility = if (game.titleId == 0L) {
                View.GONE
            } else {
                View.VISIBLE
            }

            // Add favorite icon
            val preferences = PreferenceManager.getDefaultSharedPreferences(binding.root.context)
            val isFavorite = preferences.getBoolean("favorite_${game.titleId}", false)

            binding.favoriteIcon.setImageResource(R.drawable.ic_star)


            binding.textGameTitle.text = game.title
            binding.favoriteIcon.visibility = if (isFavorite) View.VISIBLE else View.GONE
            binding.textCompany.text = game.company
            binding.textGameRegion.text = game.regions
            binding.textGameId.text = String.format("ID: %016X", game.titleId)
            binding.textFilename.text = game.filename

            val backgroundColorId =
                if (
                    isValidGame(
                        game.filename.substring(game.filename.lastIndexOf(".") + 1).lowercase()
                    )
                ) {
                    com.google.android.material.R.attr.colorSurface
                } else {
                    com.google.android.material.R.attr.colorErrorContainer
                }
            binding.cardContents.setBackgroundColor(
                MaterialColors.getColor(
                    binding.cardContents,
                    backgroundColorId
                )
            )

            binding.textGameTitle.postDelayed(
                {
                    binding.textGameTitle.ellipsize = TextUtils.TruncateAt.MARQUEE
                    binding.textGameTitle.isSelected = true

                    binding.textCompany.ellipsize = TextUtils.TruncateAt.MARQUEE
                    binding.textCompany.isSelected = true

                    binding.textGameRegion.ellipsize = TextUtils.TruncateAt.MARQUEE
                    binding.textGameRegion.isSelected = true
                    binding.textGameId.ellipsize = TextUtils.TruncateAt.MARQUEE
                    binding.textGameId.isSelected = true

                    binding.textFilename.ellipsize = TextUtils.TruncateAt.MARQUEE
                    binding.textFilename.isSelected = true
                },
                3000
            )
        }
    }

    private data class GameDirectories(
        val gameDir: String,
        val saveDir: String,
        val modsDir: String,
        val texturesDir: String,
        val appDir: String,
        val dlcDir: String,
        val updatesDir: String,
        val extraDir: String,
        val shadersDir: String,
        val logsDir: String
    )

    private fun getGameDirectories(game: Game): GameDirectories {
        return GameDirectories(
            gameDir = game.path.substringBeforeLast("/"),
            saveDir = "sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/${
                String.format(
                    "%016x",
                    game.titleId
                ).lowercase().substring(0, 8)
            }/${String.format("%016x", game.titleId).lowercase().substring(8)}/data/00000001",
            modsDir = "load/mods/${String.format("%016X", game.titleId)}",
            texturesDir = "load/textures/${String.format("%016X", game.titleId)}",
            appDir = game.path.substringBeforeLast("/").split("/").filter { it.isNotEmpty() }
                .joinToString("/"),
            dlcDir = "sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/0004008c/${
                String.format(
                    "%016x",
                    game.titleId
                ).lowercase().substring(8)
            }/content",
            updatesDir = "sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/0004000e/${
                String.format(
                    "%016x",
                    game.titleId
                ).lowercase().substring(8)
            }/content",
            extraDir = "sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/extdata/00000000/${
                String.format(
                    "%016X",
                    game.titleId
                ).substring(8, 14).padStart(8, '0')
            }",
            shadersDir = "shaders",
            logsDir = "log"
        )
    }

    private fun showOpenContextMenu(view: View, game: Game) {
        val dirs = getGameDirectories(game)

        val popup = PopupMenu(view.context, view).apply {
            menuInflater.inflate(R.menu.game_context_menu_open, menu)
            listOf(
                R.id.game_context_open_app to dirs.appDir,
                R.id.game_context_open_save_dir to dirs.saveDir,
                R.id.game_context_open_dlc to dirs.dlcDir,
                R.id.game_context_open_updates to dirs.updatesDir,
                R.id.game_context_open_updates to dirs.shadersDir,
                R.id.game_context_open_updates to dirs.logsDir
            ).forEach { (id, dir) ->
                menu.findItem(id)?.isEnabled =
                    Borked3DSApplication.documentsTree.folderUriHelper(dir)?.let {
                        DocumentFile.fromTreeUri(view.context, it)?.exists()
                    } ?: false
            }
            menu.findItem(R.id.game_context_open_extra)?.let { item ->
                if (Borked3DSApplication.documentsTree.folderUriHelper(dirs.extraDir)?.let {
                        DocumentFile.fromTreeUri(view.context, it)?.exists()
                    } != true) {
                    menu.removeItem(item.itemId)
                }
            }
        }

        popup.setOnMenuItemClickListener { menuItem ->
            val intent = Intent(Intent.ACTION_VIEW)
                .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .setType("*/*")

            val uri = when (menuItem.itemId) {
                R.id.game_context_open_app -> Borked3DSApplication.documentsTree.folderUriHelper(
                    dirs.appDir
                )

                R.id.game_context_open_save_dir -> Borked3DSApplication.documentsTree.folderUriHelper(
                    dirs.saveDir
                )

                R.id.game_context_open_dlc -> Borked3DSApplication.documentsTree.folderUriHelper(
                    dirs.dlcDir
                )

                R.id.game_context_open_textures -> Borked3DSApplication.documentsTree.folderUriHelper(
                    dirs.texturesDir,
                    true
                )

                R.id.game_context_open_mods -> Borked3DSApplication.documentsTree.folderUriHelper(
                    dirs.modsDir,
                    true
                )

                R.id.game_context_open_extra -> Borked3DSApplication.documentsTree.folderUriHelper(
                    dirs.extraDir
                )

                R.id.game_context_open_shaders -> Borked3DSApplication.documentsTree.folderUriHelper(
                    dirs.shadersDir
                )

                R.id.game_context_open_logs -> Borked3DSApplication.documentsTree.folderUriHelper(
                    dirs.logsDir
                )

                else -> null
            }

            uri?.let {
                intent.data = it
                view.context.startActivity(intent)
                true
            } ?: false
        }

        popup.show()
    }

    private fun showUninstallContextMenu(
        view: View,
        game: Game,
        bottomSheetDialog: BottomSheetDialog
    ) {
        val dirs = getGameDirectories(game)
        val popup = PopupMenu(view.context, view).apply {
            menuInflater.inflate(R.menu.game_context_menu_uninstall, menu)
            listOf(
                R.id.game_context_uninstall to dirs.gameDir,
                R.id.game_context_uninstall_dlc to dirs.dlcDir,
                R.id.game_context_uninstall_updates to dirs.updatesDir
            ).forEach { (id, dir) ->
                menu.findItem(id)?.isEnabled =
                    Borked3DSApplication.documentsTree.folderUriHelper(dir)?.let {
                        DocumentFile.fromTreeUri(view.context, it)?.exists()
                    } ?: false
            }
        }

        popup.setOnMenuItemClickListener { menuItem ->
            val uninstallAction: () -> Unit = {
                when (menuItem.itemId) {
                    R.id.game_context_uninstall -> Borked3DSApplication.documentsTree.deleteDocument(
                        dirs.gameDir
                    )

                    R.id.game_context_uninstall_dlc -> FileUtil.deleteDocument(
                        Borked3DSApplication.documentsTree.folderUriHelper(dirs.dlcDir)
                            .toString()
                    )

                    R.id.game_context_uninstall_updates -> FileUtil.deleteDocument(
                        Borked3DSApplication.documentsTree.folderUriHelper(dirs.updatesDir)
                            .toString()
                    )
                }
                ViewModelProvider(activity)[GamesViewModel::class.java].reloadGames(true)
                bottomSheetDialog.dismiss()
            }

            if (menuItem.itemId in listOf(
                    R.id.game_context_uninstall,
                    R.id.game_context_uninstall_dlc,
                    R.id.game_context_uninstall_updates
                )
            ) {
                IndeterminateProgressDialogFragment.newInstance(
                    activity,
                    R.string.uninstalling,
                    false,
                    uninstallAction
                )
                    .show(activity.supportFragmentManager, IndeterminateProgressDialogFragment.TAG)
                true
            } else {
                false
            }
        }

        popup.show()
    }

    private fun showAboutGameDialog(
        context: Context,
        game: Game,
        holder: GameViewHolder,
        view: View
    ) {
        val bottomSheetView = layoutInflater.inflate(R.layout.dialog_about_game, null)

        val game_id = String.format("%016X", game.titleId)
        val game_filename = game.filename
        val id_label = context.getString(R.string.id_label)
        val file_label = context.getString(R.string.file_label)

        val bottomSheetDialog = BottomSheetDialog(context)
        bottomSheetDialog.setContentView(bottomSheetView)

        bottomSheetView.findViewById<TextView>(R.id.about_game_title).text = game.title
        bottomSheetView.findViewById<TextView>(R.id.about_game_company).text = game.company
        bottomSheetView.findViewById<TextView>(R.id.about_game_region).text = game.regions
        bottomSheetView.findViewById<TextView>(R.id.about_game_id).text =
            "$id_label: $game_id"
        bottomSheetView.findViewById<TextView>(R.id.about_game_filename).text =
            "$file_label: $game_filename"
        bottomSheetView.findViewById<TextView>(R.id.about_game_playtime).text =
            "Playtime: " + PlayTimeTracker.getPlayTime(game.titleId)
        GameIconUtils.loadGameIcon(activity, game, bottomSheetView.findViewById(R.id.game_icon))

        bottomSheetView.findViewById<MaterialButton>(R.id.about_game_play).setOnClickListener {
            val action = HomeNavigationDirections.actionGlobalEmulationActivity(holder.game)
            view.findNavController().navigate(action)
        }

        bottomSheetView.findViewById<MaterialButton>(R.id.game_shortcut).setOnClickListener {
            val preferences = PreferenceManager.getDefaultSharedPreferences(context)

            // Default to false for zoomed in shortcut icons
            preferences.edit() {
                putBoolean(
                    "shouldStretchIcon",
                    false
                )
            }

            dialogShortcutBinding = DialogShortcutBinding.inflate(activity.layoutInflater)

            dialogShortcutBinding!!.shortcutNameInput.setText(game.title)
            GameIconUtils.loadGameIcon(activity, game, dialogShortcutBinding!!.shortcutIcon)

            dialogShortcutBinding!!.shortcutIcon.setOnClickListener {
                openImageLauncher?.launch("image/*")
            }

            dialogShortcutBinding!!.imageScaleSwitch.setOnCheckedChangeListener { _, isChecked ->
                preferences.edit {
                    putBoolean(
                        "shouldStretchIcon",
                        isChecked
                    )
                }
                refreshShortcutDialogIcon()
            }

            MaterialAlertDialogBuilder(context)
                .setTitle(R.string.create_shortcut)
                .setView(dialogShortcutBinding!!.root)
                .setPositiveButton(android.R.string.ok) { _, _ ->
                    val shortcutName = dialogShortcutBinding!!.shortcutNameInput.text.toString()
                    if (shortcutName.isEmpty()) {
                        Toast.makeText(context, R.string.shortcut_name_empty, Toast.LENGTH_LONG).show()
                        return@setPositiveButton
                    }
                    val iconBitmap = (dialogShortcutBinding!!.shortcutIcon.drawable as BitmapDrawable).bitmap
                    val shortcutManager = activity.getSystemService(ShortcutManager::class.java)

                    CoroutineScope(Dispatchers.IO).launch {
                        val icon = Icon.createWithBitmap(iconBitmap)
                        val shortcut = ShortcutInfo.Builder(context, shortcutName)
                            .setShortLabel(shortcutName)
                            .setIcon(icon)
                            .setIntent(game.launchIntent.apply {
                                putExtra("launchedFromShortcut", true)
                            })
                            .build()

                        shortcutManager?.requestPinShortcut(shortcut, null)
                        imagePath = null
                    }
                }
                .setNegativeButton(android.R.string.cancel) { _, _ ->
                    imagePath = null
                }
                .show()

            bottomSheetDialog.dismiss()
        }

        bottomSheetView.findViewById<MaterialButton>(R.id.cheats).setOnClickListener {
            val action = CheatsFragmentDirections.actionGlobalCheatsFragment(holder.game.titleId)
            view.findNavController().navigate(action)
            bottomSheetDialog.dismiss()
        }

        bottomSheetView.findViewById<MaterialButton>(R.id.favorite_game).apply {
            val preferences = PreferenceManager.getDefaultSharedPreferences(context)
            val isFavorite = preferences.getBoolean("favorite_${game.titleId}", false)

            setIconResource(if (isFavorite) R.drawable.ic_star else R.drawable.ic_star_frame)

            setOnClickListener {
                val newFavoriteState = !isFavorite
                preferences.edit()
                    .putBoolean("favorite_${game.titleId}", newFavoriteState)
                    .apply()
                setIconResource(if (newFavoriteState) R.drawable.ic_star else R.drawable.ic_star_frame)

                val position = currentList.indexOf(GameListItem.GameItem(game))
                if (position != -1) {
                    notifyItemChanged(position)
                }

                val sortedGames =
                    currentList.filterIsInstance<GameListItem.GameItem>().map { it.game }
                submitGameList(sortedGames)
                bottomSheetDialog.dismiss()
            }
        }

        bottomSheetView.findViewById<MaterialButton>(R.id.menu_button_open).setOnClickListener {
            showOpenContextMenu(it, game)
        }

        bottomSheetView.findViewById<MaterialButton>(R.id.menu_button_uninstall)
            .setOnClickListener {
                showUninstallContextMenu(it, game, bottomSheetDialog)
            }

        val bottomSheetBehavior = bottomSheetDialog.getBehavior()
        bottomSheetBehavior.skipCollapsed = true
        bottomSheetBehavior.state = BottomSheetBehavior.STATE_EXPANDED

        bottomSheetDialog.show()
    }

    private fun refreshShortcutDialogIcon() {
        if (imagePath != null) {
            val originalBitmap = BitmapFactory.decodeStream(
                Borked3DSApplication.appContext.contentResolver.openInputStream(
                    imagePath!!.toUri()
                )
            )
            val scaledBitmap = {
                val preferences =
                    PreferenceManager.getDefaultSharedPreferences(Borked3DSApplication.appContext)
                if (preferences.getBoolean("shouldStretchIcon", true)) {
                    // stretch to fit
                    originalBitmap.scale(108, 108)
                } else {
                    // Zoom in to fit the bitmap while keeping the aspect ratio
                    val width = originalBitmap.width
                    val height = originalBitmap.height
                    val targetSize = 108

                    if (width > height) {
                        // Landscape orientation
                        val scaleFactor = targetSize.toFloat() / height
                        val scaledWidth = (width * scaleFactor).toInt()
                        val scaledBmp = originalBitmap.scale(scaledWidth, targetSize)

                        val startX = (scaledWidth - targetSize) / 2
                        Bitmap.createBitmap(scaledBmp, startX, 0, targetSize, targetSize)
                    } else {
                        val scaleFactor = targetSize.toFloat() / width
                        val scaledHeight = (height * scaleFactor).toInt()
                        val scaledBmp = originalBitmap.scale(targetSize, scaledHeight)

                        val startY = (scaledHeight - targetSize) / 2
                        Bitmap.createBitmap(scaledBmp, 0, startY, targetSize, targetSize)
                    }
                }
            }()
                dialogShortcutBinding!!.shortcutIcon.setImageBitmap(scaledBitmap)
            }
        }

    private fun isValidGame(extension: String): Boolean {
        return Game.badExtensions.stream()
            .noneMatch { extension == it.lowercase() }
    }

    private class DiffCallback : DiffUtil.ItemCallback<GameListItem>() {
        override fun areItemsTheSame(oldItem: GameListItem, newItem: GameListItem): Boolean {
            if (oldItem::class != newItem::class) return false
            return when (oldItem) {
                is GameListItem.GameItem -> {
                    val newGame = (newItem as GameListItem.GameItem)
                    oldItem.game.titleId == newGame.game.titleId
                }

                GameListItem.Separator -> true
            }
        }

        override fun areContentsTheSame(oldItem: GameListItem, newItem: GameListItem): Boolean {
            return oldItem == newItem
        }
    }

    private val gameComparator = compareBy<Game> { game ->
        val preferences =
            PreferenceManager.getDefaultSharedPreferences(Borked3DSApplication.appContext)
        !preferences.getBoolean("favorite_${game.titleId}", false) // Favorites first
    }.thenBy { it.title.lowercase() } // Then alphabetically

    fun submitGameList(games: List<Game>) {
        val preferences =
            PreferenceManager.getDefaultSharedPreferences(Borked3DSApplication.appContext)
        val sortedGames = games.sortedWith(gameComparator)

        val items = mutableListOf<GameListItem>()
        var hasFavorites = false
        var hasNonFavorites = false

        sortedGames.forEach { game ->
            val isFavorite = preferences.getBoolean("favorite_${game.titleId}", false)
            if (isFavorite) hasFavorites = true
            if (!isFavorite) hasNonFavorites = true
            items.add(GameListItem.GameItem(game))
        }

        if (hasFavorites && hasNonFavorites) {
            val separatorIndex = items.indexOfFirst {
                it is GameListItem.GameItem &&
                        !preferences.getBoolean("favorite_${it.game.titleId}", false)
            }
            items.add(separatorIndex, GameListItem.Separator)
        }

        submitList(items)
    }

    inner class SeparatorViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView)

}
