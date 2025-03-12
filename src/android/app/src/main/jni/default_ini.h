// Copyright 2014 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Copyright 2025 Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

namespace DefaultINI {

const char* sdl2_config_file = R"(
[Controls]
# The input devices and parameters for each 3DS native input
# It should be in the format of "engine:[engine_name],[param1]:[value1],[param2]:[value2]..."
# Escape characters $0 (for ':'), $1 (for ',') and $2 (for '$') can be used in values

# For button input, the following devices are available:
#  - "keyboard" (default) for keyboard input. Required parameters:
#      - "code": Code of the key to bind
#  - "sdl" for joystick input using SDL. Required parameters:
#      - "joystick": Index of the joystick to bind
#      - "button"(optional): Index of the button to bind
#      - "hat"(optional): Index of the hat to bind as direction buttons
#      - "axis"(optional): Index of the axis to bind
#      - "direction"(only used for hat): Direction name of the hat to bind. Can be "up", "down", "left" or "right"
#      - "threshold"(only used for axis): Float value in (-1.0, 1.0) which the button is
#          triggered if the axis value crosses
#      - "direction"(only used for axis): "+" means the button is triggered when the axis value
#          is greater than the threshold; "-" means the button is triggered when the axis value
#          is smaller than the threshold
button_a=
button_b=
button_x=
button_y=
button_up=
button_down=
button_left=
button_right=
button_l=
button_r=
button_start=
button_select=
button_debug=
button_gpio14=
button_zl=
button_zr=
button_home=

# For analog input, the following devices are available:
#  - "analog_from_button" (default) for emulating analog input from direction buttons. Required parameters:
#      - "up", "down", "left", "right": sub-devices for each direction.
#          Should be in the format as a button input devices using escape characters, for example, "engine$0keyboard$1code$00"
#      - "modifier": Sub-devices as a modifier.
#      - "modifier_scale": Float number representing the applied modifier scale to the analog input.
#          Must be in range of 0.0-1.0. Defaults to 0.5
#  - "sdl" for joystick input using SDL. Required parameters:
#      - "joystick": Index of the joystick to bind
#      - "axis_x": Index of the axis to bind as x-axis (default to 0)
#      - "axis_y": Index of the axis to bind as y-axis (default to 1)
circle_pad=
c_stick=

# For motion input, the following devices are available:
#  - "motion_emu" (default) for emulating motion input from mouse input. Required parameters:
#      - "update_period": Update period in milliseconds (default to 100)
#      - "sensitivity": Coefficient converting mouse movement to tilting angle (default to 0.01)
#      - "tilt_clamp": Max value of the tilt angle in degrees (default to 90)
#  - "cemuhookudp" reads motion input from a udp server that uses cemuhook's udp protocol
motion_device=

# For touch input, the following devices are available:
#  - "emu_window" (default) for emulating touch input from mouse input to the emulation window. No parameters required
#  - "cemuhookudp" reads touch input from a udp server that uses cemuhook's udp protocol
#      - "min_x", "min_y", "max_x", "max_y": defines the udp device's touch screen coordinate system
touch_device= engine:emu_window

# Most desktop OSes do not expose a way to poll the motion state of the controllers
# so as a way around it, cemuhook created a udp client/server protocol to broadcast data directly
# from a controller device to the client program. Borked3DS has a client that can connect and read
# from any cemuhook compatible motion program.

# IPv4 address of the udp input server (Default "127.0.0.1")
udp_input_address=

# Port of the udp input server. (Default 26760)
udp_input_port=

# The pad to request data on. Should be between 0 (Pad 1) and 3 (Pad 4). (Default 0)
udp_pad_index=

# Use Artic Controller when connected to Artic Base Server. (Default 0)
use_artic_base_controller=

[Core]
# May reduce thermal throttling by reducing clock speed slightly.
# 0 (default): Off, 1: On
sustained_performance =

# Whether to use the Just-In-Time (JIT) compiler for CPU emulation
# 0: Interpreter (slow), 1 (default): JIT (fast)
use_cpu_jit =

# Change the Clock Frequency of the emulated 3DS CPU.
# Underclocking can increase the performance at the risk of freezing.
# Overclocking may fix lagging, but at the risk of freezing.
# Range is any positive integer (but we suspect 25 - 400 is a good idea) Default is 100
cpu_clock_percentage =

# Enable Custom CPU ticks
# 0 (default): Off, 1: On
enable_custom_cpu_ticks =

# Set Custom CPU ticks
# Set a custom value of CPU ticks. Higher values can increase performance but if too high,
# game may freeze. Range of 77-21000 is recommended.
enable_custom_cpu_ticks =

# Downcount will be limited to a smaller time slice.
# 0 (default): Off, 1: On
core_downcount_hack =

# Boost low priority starved threads during kernel rescheduling.
# 0 (default): Off, 1: On
priority_boost =

# Whether to hide 3DS camera and screenshot images from being indexed by Android
# 0: Off (default), 1: On
hide_images =

[Renderer]
# Whether to render using OpenGL
# 1: OpenGL ES (default), 2: Vulkan
graphics_api =

# Whether to compile shaders on multiple worker threads (Vulkan only)
# 0: Off, 1: On (default)
async_shader_compilation =

# Whether to emit PICA fragment shader using SPIRV or GLSL (Vulkan only)
# 0: GLSL, 1: SPIR-V (default)
spirv_shader_gen =

# Enable Geometry Shaders. Improved accuracy but extremely expensive on tilers.
# (Vulkan only)
# 0 (default): Off, 1: On
geometry_shader =

# Enables a Vulkan extension that may improve the rendering quality.
# 0 (default): Off, 1: On
use_sample_shading =

# The applied frameskip amount (power of two).
# 0 (default): No frameskip, 1: x2 frameskip, 2: x4 frameskip, 3: x8 frameskip, x16 frameskip.
frame_skip =

# Skips the slow drawing event from PICA core.
# 0 (default): Off, 1: On
skip_slow_draw =

# Skips the texture copy event from rasterizer cache.
# 0 (default): Off, 1: On
skip_texture_copy =

# Skips the CPU write event from rasterizer cache invalidation.
# 0 (default): Off, 1: On
skip_cpu_write =

# Overrides upscaling for dst_params
# 0 (default): Off, 1: On
upscaling_hack =

# Whether to use hardware shaders to emulate 3DS shaders
# 0: Software, 1 (default): Hardware
use_hw_shader =

# Whether to use accurate multiplication in hardware shaders
# 0: Off (Default. Faster, but causes issues in some games) 1: On (Slower, but correct)
shaders_accurate_mul =

# Whether to use the Just-In-Time (JIT) compiler for shader emulation
# 0: Interpreter (slow), 1 (default): JIT (fast)
use_shader_jit =

# Perform presentation on seperate threads. Improves performance on Vulkan in most games.
# 0: Off, 1 (default): On
async_presentation =

# Forces VSync on the display thread. Usually doesn't impact performance, but on some drivers it can
# so only turn this off if you notice a speed difference.
# 0: Off, 1 (default): On
use_vsync_new =

# Overrides the sampling filter used by games. This can be useful in certain cas
es with poorly behaved
# games when upscaling.
# 0 (default): Game Controlled, 2: Nearest Neighbor, 3: Linear
texture_sampling =

# Reduce stuttering by storing and loading generated shaders to disk
# 0: Off, 1 (default. On)
use_disk_shader_cache =

# Resolution scale factor
# 0: Auto (scales resolution to window size), 1: Native 3DS screen resolution, Otherwise a scale
# factor for the 3DS resolution
resolution_factor =

# Texture filter
# 0: None, 1: Anime4K Ultrafast, 2: Bicubic, 3: ScaleForce, 4: xBRZ Freescale, 5: MMPX
texture_filter =

# SPIR-V Shader Generation
# 0: Disabled, 1: Enabled
spirv_shader_gen =

# SPIR-V Optimization
# 0: Disabled, 2: Performance, 3: Size
optimize_spirv_output =

# SPIR-V Validation
# 0: Disabled, 1: Enabled
spirv_output_validation =

# SPIR-V Legalization
# 0: Disabled, 1: Enabled
spirv_output_legalization =

# Whether to enable V-Sync (caps the framerate at 60FPS) or not.
# 0 (default): Off, 1: On
vsync_enabled =

# Turns on the frame limiter, which will limit frames output to the target game speed
# 0: Off, 1: On (default)
use_frame_limit =

# Limits the speed of the game to run no faster than this value as a percentage of target speed
# 1 - 9999: Speed limit as a percentage of target game speed. 100 (default)
frame_limit =

# The clear color for the renderer. What shows up on the sides of the bottom screen.
# Must be in range of 0.0-1.0. Defaults to 0.0 for all.
bg_red =
bg_blue =
bg_green =

# Whether and how Stereoscopic 3D should be rendered
# 0 (default): Off (Monoscopic), 1: Side by Side, 2: Reverse Side by Side, 3: Anaglyph, 4: Interlaced, 5: Reverse Interlaced, 6: Cardboard VR
render_3d =

# Change 3D Intensity
# 0 - 255: Intensity. 0 (default)
factor_3d =

# Change default eye to render when in Monoscopic Mode (i.e. Stereoscopic 3D Mode is set to `Off`).
# 0 (default): Left, 1: Right
mono_render_option =

# Swap Eyes in 3d
# true or false (default)
swap_eyes_3d =

# Name of the post processing shader to apply.
# Loaded from shaders if render_3d is off or side by side.
pp_shader_name =

# Name of the shader to apply when render_3d is anaglyph.
# Loaded from shaders/anaglyph.
# Options (enter as strings):
# rendepth (builtin)
# dubois (builtin)
anaglyph_shader_name =

# Whether to enable linear filtering or not
# This is required for some shaders to work correctly
# 0: Nearest, 1 (default): Linear
filter_mode =

# Delays the game render thread by the specified amount of microseconds
# Set to 0 for no delay, only useful in dynamic-fps games to simulate GPU delay.
delay_game_render_thread_us =

# Disables rendering the right eye image.
# Greatly improves performance in some games, but can cause flickering in others.
# 0: Enable right eye rendering, 1 (default): Disable right eye rendering
disable_right_eye_render =

[Layout]
# Layout for the screen inside the render window, landscape mode
# 0: Original (screens vertically aligned)
# 1: Single Screen Only,
# 2: Large Screen (Default on android)
# 3: Side by Side
# 4: Hybrid
# 5: Custom Layout
layout_option =

# Screen Gap - adds a gap between screens in all two-screen modes
# Measured in pixels relative to the 240px default height of the screens
# Scales with the larger screen (so 24 is 10% of the larger screen height)
# Default value is 0.0
screen_gap =

# Large Screen Proportion - Relative size of large:small in large screen mode
# Default value is 2.25
large_screen_proportion =

# Small Screen Position - where is the small screen relative to the large
# Default value is 0
# 0: Top Right    1: Middle Right    2: Bottom Right
# 3: Top Left     4: Middle left     5: Bottom Left
# 6: Above the large screen          7: Below the large screen
small_screen_position =


# Screen placement when using Custom layout option
# 0x, 0y is the top left corner of the render window.
# suggested aspect ratio for top screen is 5:3
# suggested aspect ratio for bottom screen is 4:3
custom_top_x =
custom_top_y =
custom_top_width =
custom_top_height =
custom_bottom_x =
custom_bottom_y =
custom_bottom_width =
custom_bottom_height =

# Orientation option for the emulator
# 2 (default): Automatic
# 0: Landscape
# 8: Landscape (Flipped)
# 1: Portrait
# 9: Portrait (Flipped)
screen_orientation =

# Position of the performance overlay
# 0: Top Left
# 1: Center Top
# 2: Top Right
# 3: Bottom Left
# 4: Center Bottom
# 5: Bottom Right
perf_overlay_position =

# Layout for the portrait mode
# 0 (default): Top and bottom screens at top, full width
# 1: Custom Layout
portrait_layout_option =

# Screen placement when using Portrait Custom layout option
# 0x, 0y is the top left corner of the render window.
custom_portrait_top_x =
custom_portrait_top_y =
custom_portrait_top_width =
custom_portrait_top_height =
custom_portrait_bottom_x =
custom_portrait_bottom_y =
custom_portrait_bottom_width =
custom_portrait_bottom_height =

# Opacity of second layer when using custom layout option. Useful if positioning on top of the first layer. OpenGL only.
custom_second_layer_opacity =

# Swaps the prominent screen with the other screen.
# For example, if Single Screen is chosen, setting this to 1 will display the bottom screen instead of the top screen.
# 0 (default): Top Screen is prominent, 1: Bottom Screen is prominent
swap_screen =

# Expands the display area to include the cutout (or notch) area
# 0 (default): Off, 1: On
expand_to_cutout_area =

# Screen placement settings when using Cardboard VR (render3d = 4)
# 30 - 100: Screen size as a percentage of the viewport. 85 (default)
cardboard_screen_size =
# -100 - 100: Screen X-Coordinate shift as a percentage of empty space. 0 (default)
cardboard_x_shift =
# -100 - 100: Screen Y-Coordinate shift as a percentage of empty space. 0 (default)
cardboard_y_shift =

[Utility]
# Dumps textures as PNG to dump/textures/[Title ID]/.
# 0 (default): Off, 1: On
dump_textures =

# Reads PNG files from load/textures/[Title ID]/ and replaces textures.
# 0 (default): Off, 1: On
custom_textures =

# Loads all custom textures into memory before booting.
# 0 (default): Off, 1: On
preload_textures =

# Loads custom textures asynchronously with background threads.
# 0: Off, 1 (default): On
async_custom_loading =

[Audio]
# Whether to enable Audio DSP in HLE or LLE mode (Note: LLE mode has a heavy performance impact)
# 0 (default): HLE, 1: LLE, 2: LLE Multithreaded
audio_emulation =

# Whether or not to enable the audio-stretching post-processing effect.
# This adjusts audio speed to match emulation speed and helps prevent audio stutter,
# at the cost of increasing audio latency.
# 0: No, 1 (default): Yes
enable_audio_stretching =

# Scales audio playback speed to account for drops in emulation framerate
# 0 (default): No, 1: Yes
enable_realtime_audio =

# Output volume.
# 1.0 (default): 100%, 0.0; mute
volume =

# Which audio output type to use.
# 0 (default): Auto-select, 1: No audio output, 2: Cubeb (if available), 3: OpenAL (if available), 4: SDL2 (NOT available in Android), 5: Oboe (if available)
output_type =

# Which audio output device to use.
# auto (default): Auto-select
output_device =

# Which audio input type to use.
# 0 (default): Auto-select, 1: No audio input, 2: Static noise, 3: Cubeb (if available), 4: OpenAL (if available), 5: Oboe (if available)
input_type =

# Which audio input device to use.
# auto (default): Auto-select
input_device =

[Data Storage]
# Whether to create a virtual SD card.
# 1 (default): Yes, 0: No
use_virtual_sd =

# Whether to use custom storage locations
# 1: Yes, 0 (default): No
use_custom_storage =

# The path of the virtual SD card directory.
# empty (default) will use the user_path
sdmc_directory =

# The path of NAND directory.
# empty (default) will use the user_path
nand_directory =

[System]
# The system model that Borked3DS will try to emulate
# 0: Old 3DS (default), 1: New 3DS
is_new_3ds =

# Whether to use LLE system applets, if installed
# 0 (default): No, 1: Yes
lle_applets =

# The system region that Borked3DS will use during emulation
# -1: Auto-select (default), 0: Japan, 1: USA, 2: Europe, 3: Australia, 4: China, 5: Korea, 6: Taiwan
region_value =

# The system language that Borked3DS will use during emulation
# 0: Japanese, 1: English (default), 2: French, 3: German, 4: Italian, 5: Spanish,
# 6: Simplified Chinese, 7: Korean, 8: Dutch, 9: Portuguese, 10: Russian, 11: Traditional Chinese
language =

# The clock to use when borked3ds starts
# 0: System clock (default), 1: fixed time
init_clock =

# Time used when init_clock is set to fixed_time in the format %Y-%m-%d %H:%M:%S
# set to fixed time. Default 2000-01-01 00:00:01
# Note: 3DS can only handle times later then Jan 1 2000
init_time =

# The system ticks count to use when Borked3DS starts. Simulates the amount of time the system ran before launching the game.
# This accounts for games that rely on the system tick to seed randomness.
# 0: Random (default), 1: Fixed
init_ticks_type =

# Tick count to use when init_ticks_type is set to Fixed.
# Defaults to 0.
init_ticks_override =

# Number of steps per hour reported by the pedometer. Range from 0 to 65,535.
# Defaults to 0.
steps_per_hour =

# Plugin loader state, if enabled plugins will be loaded from the SD card.
# You can also set if homebrew apps are allowed to enable the plugin loader
plugin_loader =
allow_plugin_loader =

# Whether to create a virtual SD card.
# 1 (default): Yes, 0: No
use_virtual_sd =

[Camera]
# Which camera engine to use for the right outer camera
# blank: a dummy camera that always returns black image
# image: loads a still image from the storage. When the camera is started, you will be prompted
#        to select an image.
# ndk (Default): uses the device camera. You can specify the camera ID to use in the config field.
#                If you don't specify an ID, the default setting will be used. For outer cameras,
#                the back-facing camera will be used. For the inner camera, the front-facing
#                camera will be used. Please note that 'Legacy' cameras are not supported.
camera_outer_right_name =

# A config string for the right outer camera. Its meaning is defined by the camera engine
camera_outer_right_config =

# The image flip to apply
# 0: None (default), 1: Horizontal, 2: Vertical, 3: Reverse
camera_outer_right_flip =

# ... for the left outer camera
camera_outer_left_name =
camera_outer_left_config =
camera_outer_left_flip =

# ... for the inner camera
camera_inner_name =
camera_inner_config =
camera_inner_flip =

[Debugging]
# log_filter is a filter string which removes logs below a certain logging level,
# each of the format `<class>:<level>`.
#
# Examples: *:Debug Kernel.SVC:Info Service.*:Critical
# Default: *:Trace
#
# See src/common/logging/filter.h and src/common/logging/filter.cpp for
# the full list of valid classes and levels.
log_filter = *:Trace

# log_regex_filter is a filter that only displays logs based on the regex
# expression in POSIX format supplied (see log_filter above). Default is "".
log_regex_filter =

# Record frame time data. Saved as a separate .csv file in the log directory.
# 0 (default): Off, 1: On
record_frame_times =

# Whether to enable additional debugging information during emulation
# 0 (default): Off, 1: On
renderer_debug =

# Print Vulkan API calls, parameters and values to an identified output stream.
# 0 (default): Off, 1: On
dump_command_buffers =

# Open port for listening to GDB connections.
use_gdbstub=false
gdbstub_port=24689

# Flush log output on every message
# Immediately commits the debug log to file. Use this if borked3ds crashes and the log output is being cut.
# 0: Off, 1 (default): On
instant_debug_log =

# Delay the start of apps when LLE modules are enabled
# 0: Off, 1 (default): On
delay_start_for_lle_modules =

# Force deterministic async operations
# Only needed for debugging, makes performance worse if enabled
# 0: Off (default), 1: On
deterministic_async_operations =

# To LLE a service module add "LLE\<module name>=true"

[WebService]
# URL for Web API
web_api_url = https://api.borked3ds-emu.org
# Username and token for Borked3DS Web Service
# See https://profile.borked3ds-emu.org/ for more info
borked3ds_username =
borked3ds_token =
)";
}
