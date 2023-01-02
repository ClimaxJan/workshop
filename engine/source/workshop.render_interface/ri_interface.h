// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"

#include "workshop.render_interface/ri_pipeline.h"
#include "workshop.render_interface/ri_param_block_archetype.h"
#include "workshop.render_interface/ri_texture.h"
#include "workshop.render_interface/ri_sampler.h"
#include "workshop.render_interface/ri_buffer.h"

namespace ws {

class window;
class ri_swapchain;
class ri_fence;
class ri_command_queue;
class ri_command_list;
class ri_param_block_archetype;
class ri_shader_compiler;
class ri_buffer;
class ri_layout_factory;

// ================================================================================================
//  Types of renderer implementations available. Make sure to update if you add new ones.
// ================================================================================================
enum class ri_interface_type
{
#ifdef WS_WINDOWS
    dx12
#endif
};

// ================================================================================================
//  Engine interface for all rendering functionality.
// ================================================================================================
class ri_interface
{
public:

    // Registers all the steps required to initialize the rendering system.
    // Interacting with this class without successfully running these steps is undefined.
    virtual void register_init(init_list& list) = 0;

    // Informs the renderer that a new frame is starting to be rendered. The 
    // render can use this notification to update per-frame allocations and do
    // any general bookkeeping required.
    virtual void begin_frame() = 0;

    // Informs the renderer that the frame has finished rendering.
    virtual void end_frame() = 0;

    // Creates a swapchain for rendering to the given window.
    virtual std::unique_ptr<ri_swapchain> create_swapchain(window& for_window, const char* debug_name = nullptr) = 0;
    
    // Creates a fence for syncronisation between the cpu and gpu.
    virtual std::unique_ptr<ri_fence> create_fence(const char* debug_name = nullptr) = 0;

    // Creates a class to handle compiling shaders for offline use.
    virtual std::unique_ptr<ri_shader_compiler> create_shader_compiler() = 0;

    // Creates a pipeline describing the gpu state at the point of a draw call.
    virtual std::unique_ptr<ri_pipeline> create_pipeline(const ri_pipeline::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates an archetype that represents a param block type with the given layout and scope.
    virtual std::unique_ptr<ri_param_block_archetype> create_param_block_archetype(const ri_param_block_archetype::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a texture based on the description given.
    virtual std::unique_ptr<ri_texture> create_texture(const ri_texture::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a sampler based on the description given.
    virtual std::unique_ptr<ri_sampler> create_sampler(const ri_sampler::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a buffer of an arbitrary size.
    virtual std::unique_ptr<ri_buffer> create_buffer(const ri_buffer::create_params& params, const char* debug_name = nullptr) = 0;

    // Creates a factory for laying out buffer data in a format consumable by the gpu.
    virtual std::unique_ptr<ri_layout_factory> create_layout_factory(ri_data_layout layout) = 0;

    // Gets the main graphics command queue responsible for raster ops.
    virtual ri_command_queue& get_graphics_queue() = 0;

    // Gets the main graphics command queue responsible for performing memory copies.
    virtual ri_command_queue& get_copy_queue() = 0;

    // Gets the maximum number of frames that can be in flight at the same time.
    virtual size_t get_pipeline_depth() = 0;


};

}; // namespace ws