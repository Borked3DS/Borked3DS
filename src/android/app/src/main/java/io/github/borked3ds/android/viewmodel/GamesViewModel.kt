// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.viewmodel

import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import androidx.preference.PreferenceManager
import io.github.borked3ds.android.Borked3DSApplication
import io.github.borked3ds.android.model.Game
import io.github.borked3ds.android.utils.GameHelper
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import java.util.Locale

class GamesViewModel : ViewModel() {
    val games get() = _games.asStateFlow()
    private val _games = MutableStateFlow(emptyList<Game>())

    val searchedGames get() = _searchedGames.asStateFlow()
    private val _searchedGames = MutableStateFlow(emptyList<Game>())

    val isReloading get() = _isReloading.asStateFlow()
    private val _isReloading = MutableStateFlow(false)

    val shouldSwapData get() = _shouldSwapData.asStateFlow()
    private val _shouldSwapData = MutableStateFlow(false)

    val shouldScrollToTop get() = _shouldScrollToTop.asStateFlow()
    private val _shouldScrollToTop = MutableStateFlow(false)

    val searchFocused get() = _searchFocused.asStateFlow()
    private val _searchFocused = MutableStateFlow(false)

    init {
        // Retrieve list of cached games
        val storedGames =
            PreferenceManager.getDefaultSharedPreferences(Borked3DSApplication.appContext)
                .getStringSet(GameHelper.KEY_GAMES, emptySet())
        if (storedGames?.isNotEmpty() == true) {
            val deserializedGames = mutableSetOf<Game>()
            storedGames.forEach {
                val game: Game? = try {
                    Json.decodeFromString<Game>(it)
                } catch (ignored: Exception) {
                    null
                }

                game?.let { g ->
                    val gameExists = DocumentFile.fromSingleUri(
                        Borked3DSApplication.appContext,
                        Uri.parse(g.path)
                    )?.exists()

                    if (gameExists == true || g.isInstalled) {
                        deserializedGames.add(g)
                    }
                }
            }
            setGames(deserializedGames.toList())
        }
        reloadGames(false)
    }

    fun setGames(games: List<Game>) {
        val sortedList = games.sortedWith(
            compareBy(
                { it.title.lowercase(Locale.getDefault()) },
                { it.path }
            )
        )
        val filteredList = sortedList.filter {
            if (it.isSystemTitle) {
                it.isVisibleSystemTitle
            }
            true
        }

        _games.value = filteredList
    }

    fun setSearchedGames(games: List<Game>) {
        _searchedGames.value = games
    }

    fun setShouldSwapData(shouldSwap: Boolean) {
        _shouldSwapData.value = shouldSwap
    }

    fun setShouldScrollToTop(shouldScroll: Boolean) {
        _shouldScrollToTop.value = shouldScroll
    }

    fun setSearchFocused(searchFocused: Boolean) {
        _searchFocused.value = searchFocused
    }

    fun reloadGames(directoryChanged: Boolean) {
        if (isReloading.value) {
            return
        }
        _isReloading.value = true

        viewModelScope.launch {
            withContext(Dispatchers.IO) {
                setGames(GameHelper.getGames())
                _isReloading.value = false

                if (directoryChanged) {
                    setShouldSwapData(true)
                }
            }
        }
    }
}