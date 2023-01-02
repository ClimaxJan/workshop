// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_buffer.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_command_queue.h"
#include "workshop.render_interface.dx12/dx12_ri_upload_manager.h"
#include "workshop.render_interface.dx12/dx12_types.h"
#include "workshop.windowing/window.h"

namespace ws {

dx12_ri_buffer::dx12_ri_buffer(dx12_render_interface& renderer, const char* debug_name, const ri_buffer::create_params& params)
    : m_renderer(renderer)
    , m_debug_name(debug_name)
    , m_create_params(params)
{
}

dx12_ri_buffer::~dx12_ri_buffer()
{
    m_renderer.defer_delete([renderer = &m_renderer, handle = m_handle, srv = m_srv]()
    {
        if (srv.is_valid())
        {
            renderer->get_descriptor_table(ri_descriptor_table::buffer).free(srv);
        }
        CheckedRelease(handle);    
    });
}

result<void> dx12_ri_buffer::create_resources()
{
    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = 0;
    heap_properties.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.DepthOrArraySize = 1;
    desc.Width = m_create_params.element_count * m_create_params.element_size;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    switch (m_create_params.usage)
    {
    case ri_buffer_usage::index_buffer:
        {
            m_common_state = ri_resource_state::index_buffer;
            break;
        }
    case ri_buffer_usage::vertex_buffer:
    case ri_buffer_usage::generic:
    default:
        {
            m_common_state = ri_resource_state::pixel_shader_resource;
            break;
        }
    }

    HRESULT hr = m_renderer.get_device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        ri_to_dx12(m_common_state),
        nullptr,
        IID_PPV_ARGS(&m_handle)
    );
    if (FAILED(hr))
    {
        db_error(render_interface, "CreateCommittedResource failed with error 0x%08x.", hr);
        return false;
    }        

    // Set a debug name.
    m_handle->SetName(widen_string(m_debug_name).c_str());

    // Upload the linear data if any has been provided.
    if (!m_create_params.linear_data.empty())
    {
        m_renderer.get_upload_manager().upload(
            *this, 
            m_create_params.linear_data
        );
    }

    // Create SRV view for buffer.
    // TODO: need to create a UAV if writable
    D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = {};
    view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    view_desc.Buffer.FirstElement = 0;
    view_desc.Buffer.NumElements = (m_create_params.element_count * m_create_params.element_size) / 4;
    view_desc.Buffer.StructureByteStride = 0;
    view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    m_srv = m_renderer.get_descriptor_table(ri_descriptor_table::buffer).allocate();
    m_renderer.get_device()->CreateShaderResourceView(m_handle.Get(), &view_desc, m_srv.cpu_handle);

    return true;
}

size_t dx12_ri_buffer::get_element_count()
{
    return m_create_params.element_count;
}

size_t dx12_ri_buffer::get_element_size()
{
    return m_create_params.element_size;
}

ID3D12Resource* dx12_ri_buffer::get_resource()
{
    return m_handle.Get();
}

ri_resource_state dx12_ri_buffer::get_initial_state()
{
    return m_common_state;
}

dx12_ri_descriptor_table::allocation dx12_ri_buffer::get_srv() const
{
    return m_srv;
}

}; // namespace ws