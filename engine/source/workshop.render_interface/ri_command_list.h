// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/math/rect.h"
#include "workshop.core/math/vector4.h"

#include "workshop.render_interface/ri_types.h"

namespace ws {

class ri_pipeline;
class ri_texture;
class ri_buffer;
class ri_param_block;
class color;

// ================================================================================================
//  Represents a list of commands that can be sent to a command_queue to execute.
// ================================================================================================
class ri_command_list
{
public:

    // Called before recording command to this list.
    virtual void open() = 0;

    // Called after recording command to this list. The list is 
    // considered immutable after this call.
    virtual void close() = 0;

    // Inserts a resource barrier.
    virtual void barrier(ri_texture& resource, ri_resource_state source_state, ri_resource_state destination_state) = 0;
    virtual void barrier(ri_buffer& resource, ri_resource_state source_state, ri_resource_state destination_state) = 0;

    // Clears a render target to a specific color.
    virtual void clear(ri_texture& resource, const color& destination) = 0;

    // Changes the rendering pipeline state.
    virtual void set_pipeline(ri_pipeline& pipeline) = 0;

    // Sets the param blocks to use for next draw call. These should match
    // the param blocks expected by the pipeline. This should always be called
    // after set_pipeline not before as it uses the context for validation.
    virtual void set_param_blocks(const std::vector<ri_param_block*> param_blocks) = 0;

    // Sets the viewport in pixels that determines the 
    // bounds that are rendering within.
    virtual void set_viewport(const recti& rect) = 0;

    // Sets the scissor rectangle that stops all
    // rendering from occuring outside it.
    virtual void set_scissor(const recti& rect) = 0;

    // Sets the blend factor used when pipelines use the 
    // blending operand ri_blend_operand::blend_factor
    virtual void set_blend_factor(const vector4& factor) = 0;

    // Sets the reference value matched against when 
    // pipeline is set to perform stencil testing.
    virtual void set_stencil_ref(uint32_t value) = 0;

    // Sets the topology of primitives in the input vertex data.
    virtual void set_primitive_topology(ri_primitive value) = 0;

    // Sets the index buffer used for future draws calls. 
    // Note: Vertex buffers are accessed bindlessly, so there
    // is no equivalent set_vertex_buffer.
    virtual void set_index_buffer(ri_buffer& buffer) = 0;

    // Sets the output targets that should be rendered to. This should
    // match the set of output targets defined in the active pipeline.
    virtual void set_render_targets(const std::vector<ri_texture*>& colors, ri_texture* depth) = 0;

    // Dispatches a draw call with all the set state. 
    // Note: Vertex buffers are accessed bindlessly, so offsets for
    // selecting instance specific data is handled in the shader.
    virtual void draw(size_t indexes_per_instance, size_t instance_count) = 0;

    // Begins a profiling scope within the queue.
    virtual void begin_event(const color& color, const char* name, ...) = 0;

    // End a profiling scope within the queue.
    virtual void end_event() = 0;

};

}; // namespace ws