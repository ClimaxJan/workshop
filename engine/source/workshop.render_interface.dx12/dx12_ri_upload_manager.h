// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/result.h"
#include "workshop.core/containers/memory_heap.h"

#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>
#include <span>

namespace ws {

class engine;
class dx12_render_interface;
class dx12_ri_texture;
class dx12_ri_buffer;
class dx12_ri_command_list;
class ri_fence;

// ================================================================================================
//  Handles copying CPU data to GPU memory.
// ================================================================================================
class dx12_ri_upload_manager
{
public:
    dx12_ri_upload_manager(dx12_render_interface& renderer);
    virtual ~dx12_ri_upload_manager();

    result<void> create_resources();

    void upload(dx12_ri_texture& source, const std::span<uint8_t>& data);
    void upload(dx12_ri_buffer& source, const std::span<uint8_t>& data);

    void new_frame(size_t index);

private:
    struct upload_state
    {
        size_t queued_frame_index;
        size_t heap_offset;

        dx12_ri_texture* texture = nullptr;
        dx12_ri_buffer* buffer = nullptr;

        dx12_ri_command_list* list = nullptr;
    };

    // Size of the heap, this should be able to store the maximum
    // size of any resource we try to upload.
    static constexpr size_t k_heap_size = 128 * 1024 * 1024;

private:
    upload_state allocate_upload(size_t size, size_t alignment);
    void queue_upload(upload_state state);

    void perform_uploads();
    void free_uploads();

private:
    std::recursive_mutex m_pending_upload_mutex;
    std::vector<upload_state> m_pending_uploads;
    std::vector<upload_state> m_pending_free;
    
    dx12_render_interface& m_renderer;
    std::unique_ptr<ri_fence> m_graphics_queue_fence;
    std::unique_ptr<ri_fence> m_copy_queue_fence;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_heap_handle = nullptr;
    uint8_t* m_upload_heap_ptr = nullptr;

    size_t m_frame_index = 0;

    std::unique_ptr<memory_heap> m_upload_heap;

};

}; // namespace ws