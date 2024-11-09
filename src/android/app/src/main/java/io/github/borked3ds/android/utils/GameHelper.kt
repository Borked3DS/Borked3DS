// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.utils

import android.content.SharedPreferences
import android.net.Uri
import androidx.preference.PreferenceManager
import io.github.borked3ds.android.Borked3DSApplication
import io.github.borked3ds.android.NativeLibrary
import io.github.borked3ds.android.model.CheapDocument
import io.github.borked3ds.android.model.Game
import io.github.borked3ds.android.model.GameInfo
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import java.io.IOException

object GameHelper {
    const val KEY_GAME_PATH = "game_path"
    const val KEY_GAMES = "Games"

    private lateinit var preferences: SharedPreferences

    fun getGames(): List<Game> {
        val games = mutableListOf<Game>()
        val context = Borked3DSApplication.appContext
        preferences = PreferenceManager.getDefaultSharedPreferences(context)
        val gamesDir = preferences.getString(KEY_GAME_PATH, "")
        val gamesUri = Uri.parse(gamesDir)

        addGamesRecursive(games, FileUtil.listFiles(gamesUri), 3)
        NativeLibrary.getInstalledGamePaths().forEach {
            games.add(getGame(Uri.parse(it), isInstalled = true, addedToLibrary = true))
        }

        // Cache list of games found on disk
        val serializedGames = mutableSetOf<String>()
        games.forEach {
            serializedGames.add(Json.encodeToString(it))
        }
        preferences.edit()
            .remove(KEY_GAMES)
            .putStringSet(KEY_GAMES, serializedGames)
            .apply()

        return games.toList()
    }

    private fun addGamesRecursive(
        games: MutableList<Game>,
        files: Array<CheapDocument>,
        depth: Int
    ) {
        if (depth <= 0) {
            return
        }

        files.forEach {
            if (it.isDirectory) {
                addGamesRecursive(games, FileUtil.listFiles(it.uri), depth - 1)
            } else {
                if (Game.allExtensions.contains(FileUtil.getExtension(it.uri))) {
                    games.add(getGame(it.uri, isInstalled = false, addedToLibrary = true))
                }
            }
        }
    }

    fun getGame(uri: Uri, isInstalled: Boolean, addedToLibrary: Boolean): Game {
        val filePath = uri.toString()
        val gameInfo: GameInfo? = try {
            GameInfo(filePath)
        } catch (e: IOException) {
            null
        }

        val newGame = Game(
            (gameInfo?.getTitle() ?: FileUtil.getFilename(uri)).replace("[\\t\\n\\r]+".toRegex(), " "),
            filePath.replace("\n", " "),
            filePath,
            NativeLibrary.getTitleId(filePath),
            gameInfo?.getCompany() ?: "",
            gameInfo?.getRegions() ?: "Invalid region",
            isInstalled,
            NativeLibrary.getIsSystemTitle(filePath),
            gameInfo?.getIsVisibleSystemTitle() == true,
            gameInfo?.getIcon(),
            if (FileUtil.isNativePath(filePath)) {
                Borked3DSApplication.documentsTree.getFilename(filePath)
            } else {
                FileUtil.getFilename(Uri.parse(filePath))
            }
        )

        if (addedToLibrary) {
            val addedTime = preferences.getLong(newGame.keyAddedToLibraryTime, 0L)
            if (addedTime == 0L) {
                preferences.edit()
                    .putLong(newGame.keyAddedToLibraryTime, System.currentTimeMillis())
                    .apply()
            }
        }

        return newGame
    }
}
