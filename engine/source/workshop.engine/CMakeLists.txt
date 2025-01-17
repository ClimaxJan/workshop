# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

# The engine library contains the base game engine code, this contains and binds together the
# the basic building blocks of the engine into something capable of running and executing.
#
# High level gameplay constructs such as specialized objects, ai, etc. Should all exist in 
# a framework library, not in here.

project(workshop.engine C CXX)

SET(SOURCES
    "app/engine_app.cpp"
    "app/engine_app.h"

    "engine/engine.cpp"
    "engine/engine.h"
    "engine/world.cpp"
    "engine/world.h"

    "ecs/component.cpp"
    "ecs/component.h"
    "ecs/component_filter.cpp"
    "ecs/component_filter.h"
    "ecs/object.cpp"
    "ecs/object.h"
    "ecs/ecs_manager.cpp"
    "ecs/ecs_manager.h"
    
    "presentation/presenter.cpp"
    "presentation/presenter.h"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

set(NON_RELEASE_LIBS "")

# Include pix in windows/non-release configurations.
if (WIN32)
    set(NON_RELEASE_LIBS ${NON_RELEASE_LIBS} pix)
endif()

target_link_libraries(${PROJECT_NAME} 
    workshop.renderer
    workshop.window_interface
    workshop.input_interface
    workshop.platform_interface
    workshop.debug_menu
    "$<$<CONFIG:Debug>:${NON_RELEASE_LIBS}>"
    "$<$<CONFIG:Profile>:${NON_RELEASE_LIBS}>"
    yamlcpp
)

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier2/engine")
