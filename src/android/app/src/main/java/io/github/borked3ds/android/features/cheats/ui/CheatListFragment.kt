// Copyright 2023 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

package io.github.borked3ds.android.features.cheats.ui

import android.annotation.SuppressLint
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.ViewGroup.MarginLayoutParams
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updatePadding
import androidx.fragment.app.Fragment
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.navigation.findNavController
import androidx.recyclerview.widget.LinearLayoutManager
import com.google.android.material.divider.MaterialDividerItemDecoration
import io.github.borked3ds.android.R
import io.github.borked3ds.android.databinding.FragmentCheatListBinding
import io.github.borked3ds.android.features.cheats.model.CheatsViewModel
import io.github.borked3ds.android.ui.main.MainActivity
import kotlinx.coroutines.launch

class CheatListFragment : Fragment() {
    private var _binding: FragmentCheatListBinding? = null
    private val binding get() = _binding!!

    private val cheatsViewModel: CheatsViewModel by activityViewModels()

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View {
        _binding = FragmentCheatListBinding.inflate(layoutInflater)
        return binding.root
    }

    // This is using the correct scope, lint is just acting up
    @SuppressLint("UnsafeRepeatOnLifecycleDetector")
    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.cheatList.adapter = CheatsAdapter(requireActivity(), cheatsViewModel)
        binding.cheatList.layoutManager = LinearLayoutManager(requireContext())
        binding.cheatList.addItemDecoration(
            MaterialDividerItemDecoration(
                requireContext(),
                MaterialDividerItemDecoration.VERTICAL
            )
        )

        viewLifecycleOwner.lifecycleScope.apply {
            launch {
                repeatOnLifecycle(Lifecycle.State.CREATED) {
                    cheatsViewModel.cheatAddedEvent.collect { position: Int? ->
                        position?.let {
                            binding.cheatList.apply {
                                post { (adapter as? CheatsAdapter)?.notifyItemInserted(it) }
                            }
                        }
                    }
                }
            }
            launch {
                repeatOnLifecycle(Lifecycle.State.CREATED) {
                    cheatsViewModel.cheatChangedEvent.collect { position: Int? ->
                        position?.let {
                            binding.cheatList.apply {
                                post { (adapter as? CheatsAdapter)?.notifyItemChanged(it) }
                            }
                        }
                    }
                }
            }
            launch {
                repeatOnLifecycle(Lifecycle.State.CREATED) {
                    cheatsViewModel.cheatDeletedEvent.collect { position: Int? ->
                        position?.let {
                            binding.cheatList.apply {
                                post { (adapter as? CheatsAdapter)?.notifyItemRemoved(it) }
                            }
                        }
                    }
                }
            }
        }

        binding.fab.setOnClickListener {
            cheatsViewModel.startAddingCheat()
            cheatsViewModel.openDetailsView()
        }

        binding.toolbarCheatList.setNavigationOnClickListener {
            if (requireActivity() is MainActivity) {
                view.findNavController().popBackStack()
            } else {
                requireActivity().finish()
            }
        }

        setInsets()
    }

    private fun setInsets() {
        ViewCompat.setOnApplyWindowInsetsListener(
            binding.root
        ) { _: View, windowInsets: WindowInsetsCompat ->
            val barInsets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars())
            val cutoutInsets = windowInsets.getInsets(WindowInsetsCompat.Type.displayCutout())

            val leftInsets = barInsets.left + cutoutInsets.left
            val rightInsets = barInsets.right + cutoutInsets.right

            val mlpAppBar = binding.toolbarCheatList.layoutParams as? MarginLayoutParams
            mlpAppBar?.let {
                it.leftMargin = leftInsets
                it.rightMargin = rightInsets
                binding.toolbarCheatList.layoutParams = it
            }

            binding.cheatList.updatePadding(
                left = leftInsets,
                right = rightInsets,
                bottom = barInsets.bottom +
                        resources.getDimensionPixelSize(R.dimen.spacing_fab_list)
            )

            val mlpFab = binding.fab.layoutParams as? MarginLayoutParams
            mlpFab?.let {
                val fabPadding = resources.getDimensionPixelSize(R.dimen.spacing_large)
                it.leftMargin = leftInsets + fabPadding
                it.bottomMargin = barInsets.bottom + fabPadding
                it.rightMargin = rightInsets + fabPadding
                binding.fab.layoutParams = it
            }
            windowInsets
        }
    }
}