// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_fence.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.window_interface/window.h"

namespace ws {

dx12_ri_fence::dx12_ri_fence(dx12_render_interface& renderer, const char* debug_name)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
{
}

dx12_ri_fence::~dx12_ri_fence()
{
    m_renderer.defer_delete([fence = m_fence, fence_event = m_fence_event]() 
    {
        //CheckedRelease(fence);
        CheckedCloseHandle(fence_event);
    });
}

result<void> dx12_ri_fence::create_resources()
{
    HRESULT hr = m_renderer.get_device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateFence failed with error 0x%08x.", hr);
        return false;
    }

    m_fence->SetName(widen_string(m_debug_name).c_str());

    m_fence_event = CreateEvent(nullptr, false, false, nullptr);
    if (m_fence_event == nullptr)
    {
        db_error(render_interface, "CreateEvent failed with error 0x%08x.", GetLastError());
        return false;
    }

    return true;
}

void dx12_ri_fence::wait(size_t value)
{
    HRESULT hr = m_fence->SetEventOnCompletion(value, m_fence_event);
    if (FAILED(hr)) 
    {
        db_error(render_interface, "SetEventOnCompletion failed with error 0x%08x.", hr);
    }

    WaitForSingleObject(m_fence_event, INFINITE);
}

void dx12_ri_fence::wait(ri_command_queue& queue, size_t value)
{
    dx12_ri_command_queue& dx12_queue = static_cast<dx12_ri_command_queue&>(queue);

    HRESULT hr = dx12_queue.get_queue()->Wait(m_fence.Get(), value);
    if (FAILED(hr))
    {
        db_error(render_interface, "Wait failed with error 0x%08x.", hr);
    }
}

size_t dx12_ri_fence::current_value()
{
    return m_fence->GetCompletedValue();
}

void dx12_ri_fence::signal(size_t value)
{
    HRESULT hr = m_fence->Signal(value);
    if (FAILED(hr))
    {
        db_error(render_interface, "Signal failed with error 0x%08x.", hr);
    }
}

void dx12_ri_fence::signal(ri_command_queue& queue, size_t value) 
{
    dx12_ri_command_queue& dx12_queue = static_cast<dx12_ri_command_queue&>(queue);

    HRESULT hr = dx12_queue.get_queue()->Signal(m_fence.Get(), value);
    if (FAILED(hr))
    {
        db_error(render_interface, "Signal failed with error 0x%08x.", hr);
    }
}

}; // namespace ws
