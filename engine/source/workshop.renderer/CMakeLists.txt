# ================================================================================================
#  workshop
#  Copyright (C) 2021 Tim Leonard
# ================================================================================================

project(workshop.renderer C CXX)

SET(SOURCES
    "common_types.h"

    "renderer.h"
    "renderer.cpp"
    
    "render_options.h"
    "render_options.cpp"
    "render_world_state.h"
    "render_world_state.cpp"
    "render_graph.h"
    "render_graph.cpp"
    "render_effect.h"
    "render_effect.cpp"
    "render_output.h"
    "render_output.cpp"
    "render_pass.h"
    "render_pass.cpp"
    "render_system.h"
    "render_system.cpp"
    "render_world_state.h"
    "render_world_state.cpp"    
    "render_batch_manager.h"
    "render_batch_manager.cpp"    
    "render_resource_cache.h"
    "render_resource_cache.cpp"
    
    "render_effect_manager.h"
    "render_effect_manager.cpp"    
    "render_param_block_manager.h"
    "render_param_block_manager.cpp"
    "render_scene_manager.h"
    "render_scene_manager.cpp"
    "render_visibility_manager.h"
    "render_visibility_manager.cpp"
    "render_imgui_manager.h"
    "render_imgui_manager.cpp"
    
    "render_command_queue.h"
    "render_command_queue.cpp"
    
    "render_object.h"
    "render_object.cpp"
            
    "passes/render_pass_graphics.h"
    "passes/render_pass_graphics.cpp"
    "passes/render_pass_fullscreen.h"
    "passes/render_pass_fullscreen.cpp"
    "passes/render_pass_geometry.h"
    "passes/render_pass_geometry.cpp"
    "passes/render_pass_instanced_model.h"
    "passes/render_pass_instanced_model.cpp"
    "passes/render_pass_callback.h"
    "passes/render_pass_callback.cpp"
    "passes/render_pass_primitives.h"
    "passes/render_pass_primitives.cpp"
    "passes/render_pass_imgui.h"
    "passes/render_pass_imgui.cpp"
    "passes/render_pass_compute.h"
    "passes/render_pass_compute.cpp"
    "passes/render_pass_calculate_mips.h"
    "passes/render_pass_calculate_mips.cpp"
    "passes/render_pass_clear.h"
    "passes/render_pass_clear.cpp"

    "objects/render_view.h"
    "objects/render_view.cpp"
    "objects/render_static_mesh.h"
    "objects/render_static_mesh.cpp"
    "objects/render_light.h"
    "objects/render_light.cpp"
    "objects/render_directional_light.h"
    "objects/render_directional_light.cpp"
    "objects/render_point_light.h"
    "objects/render_point_light.cpp"
    "objects/render_spot_light.h"
    "objects/render_spot_light.cpp"
    "objects/render_light_probe_grid.h"
    "objects/render_light_probe_grid.cpp"
    "objects/render_reflection_probe.h"
    "objects/render_reflection_probe.cpp"
    #"objects/render_skinned_mesh.h"
    #"objects/render_dynamic_mesh.h"

    "systems/render_system_clear.h"
    "systems/render_system_clear.cpp"
    "systems/render_system_resolve_backbuffer.h"
    "systems/render_system_resolve_backbuffer.cpp"    
    "systems/render_system_shadows.h"
    "systems/render_system_shadows.cpp"    
    "systems/render_system_lighting.h"
    "systems/render_system_lighting.cpp"    
    "systems/render_system_imgui.h"
    "systems/render_system_imgui.cpp"
    "systems/render_system_light_probes.h"
    "systems/render_system_light_probes.cpp"
    "systems/render_system_reflection_probes.h"
    "systems/render_system_reflection_probes.cpp"
    "systems/render_system_geometry.h"
    "systems/render_system_geometry.cpp"
    "systems/render_system_transparent_geometry.h"
    "systems/render_system_transparent_geometry.cpp"
    "systems/render_system_debug.h"
    "systems/render_system_debug.cpp"
    "systems/render_system_ssao.h"
    "systems/render_system_ssao.cpp"
    
    "assets/shader/shader.h"
    "assets/shader/shader.cpp"
    "assets/shader/shader_loader.h"
    "assets/shader/shader_loader.cpp"
    
    "assets/model/model.h"
    "assets/model/model.cpp"
    "assets/model/model_loader.h"
    "assets/model/model_loader.cpp"
    
    "assets/material/material.h"
    "assets/material/material.cpp"
    "assets/material/material_loader.h"
    "assets/material/material_loader.cpp"
    
    "assets/texture/texture.h"
    "assets/texture/texture.cpp"
    "assets/texture/texture_loader.h"
    "assets/texture/texture_loader.cpp"
)
 
add_library(${PROJECT_NAME} STATIC ${SOURCES})

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/tier1/rendering")

target_link_libraries(${PROJECT_NAME} 
    workshop.render_interface
    workshop.assets
    imgui
)
