// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.renderer/passes/render_pass_compute.h"
#include "workshop.renderer/render_world_state.h"
#include "workshop.renderer/render_param_block_manager.h"
#include "workshop.renderer/renderer.h"
#include "workshop.renderer/common_types.h"
#include "workshop.renderer/objects/render_view.h"
#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface/ri_command_queue.h"
#include "workshop.render_interface/ri_command_list.h"
#include "workshop.render_interface/ri_buffer.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

void render_pass_compute::generate(renderer& renderer, generated_state& state_output, render_view& view)
{
    size_t group_size_x;
    size_t group_size_y;
    size_t group_size_z;

    if (!technique->get_define<size_t>("DISPATCH_SIZE_X", group_size_x) ||
        !technique->get_define<size_t>("DISPATCH_SIZE_Y", group_size_y) ||
        !technique->get_define<size_t>("DISPATCH_SIZE_Z", group_size_z))
    {
        db_warning(renderer, "Failed to run '%s', shader is missing group size defines - DISPATCH_SIZE_X, DISPATCH_SIZE_Y, DISPATCH_SIZE_Z.", technique->name.c_str());
        return;
    }

    // Create the command list.
    ri_command_list& list = renderer.get_render_interface().get_graphics_queue().alloc_command_list();
    list.open();
    {
        profile_gpu_marker(list, profile_colors::gpu_compute, "%s", name.c_str());

        // Resolve all the param blocks we are going to use.
        std::vector<ri_param_block*> blocks = bind_param_blocks(view.get_resource_cache());

        // Put together param block list to use.
        list.set_pipeline(*technique->pipeline);
        list.set_param_blocks(blocks);
        list.dispatch(group_size_x, group_size_y, group_size_z);
    }
    list.close();

    state_output.graphics_command_lists.push_back(&list);
}

}; // namespace ws