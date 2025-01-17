# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================

include_directories("${ENV_ROOT_PATH}/engine/source")

# ================================================================================================
#  Tier 0 - Containers/Utilities/etc
# ================================================================================================
add_subdirectory(workshop.core)
if (WIN32)
    add_subdirectory(workshop.core.win32)
endif()

# ================================================================================================
#  Tier 1 - Interfaces used by the engine for rendering/loading/audio/etc
# ================================================================================================
add_subdirectory(workshop.assets)
add_subdirectory(workshop.render_interface)
if (WIN32)
    add_subdirectory(workshop.render_interface.dx12)
endif()
add_subdirectory(workshop.renderer)

add_subdirectory(workshop.platform_interface)
add_subdirectory(workshop.platform_interface.sdl)

add_subdirectory(workshop.window_interface)
add_subdirectory(workshop.window_interface.sdl)

add_subdirectory(workshop.input_interface)
add_subdirectory(workshop.input_interface.sdl)

add_subdirectory(workshop.debug_menu)

# ================================================================================================
#  Tier 2 - Engine functionality
# ================================================================================================
add_subdirectory(workshop.engine)

# ================================================================================================
#  Misc third party stuff
# ================================================================================================
add_subdirectory(thirdparty)


