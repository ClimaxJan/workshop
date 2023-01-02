// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.render_interface.dx12/dx12_ri_param_block_archetype.h"
#include "workshop.render_interface.dx12/dx12_ri_param_block.h"
#include "workshop.render_interface.dx12/dx12_ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_layout_factory.h"
#include "workshop.render_interface/ri_layout_factory.h"

namespace ws {

dx12_ri_param_block_archetype::dx12_ri_param_block_archetype(dx12_render_interface& renderer, const ri_param_block_archetype::create_params& params, const char* debug_name)
    : m_renderer(renderer)
    , m_create_params(params)
    , m_debug_name(debug_name)
{
    m_layout_factory = m_renderer.create_layout_factory(m_create_params.layout);
    m_instance_size = m_layout_factory->get_instance_size();
    m_instance_stride = math::round_up_multiple(m_instance_size, k_instance_alignment);
}

dx12_ri_param_block_archetype::~dx12_ri_param_block_archetype()
{
    for (size_t i = 0; i < m_pages.size(); i++)
    {
        alloc_page& instance = m_pages[i];
        db_assert(instance.free_list.size() == k_page_size);

        D3D12_RANGE range;
        range.Begin = 0;
        range.End = m_instance_stride * k_page_size;
        instance.handle->Unmap(0, &range);

        instance.handle = nullptr;
    }

    m_pages.clear();
}

result<void> dx12_ri_param_block_archetype::create_resources()
{
    add_page();
    return true;
}

dx12_ri_param_block_archetype::allocation dx12_ri_param_block_archetype::allocate()
{
    std::scoped_lock lock(m_allocation_mutex);

    while (true)
    {
        for (size_t i = 0; i < m_pages.size(); i++)
        {
            alloc_page& instance = m_pages[i];
            if (instance.free_list.size() > 0)
            {
                size_t index = instance.free_list.back();
                instance.free_list.pop_back();

                allocation result;
                result.cpu_address = instance.base_address_cpu + (index * m_instance_stride);
                result.gpu_address = instance.base_address_gpu + (index * m_instance_stride);
                result.pool_index = i;
                result.allocation_index = index;
                result.size = m_instance_stride;
                result.valid = true;

                return result;
            }
        }

        add_page();
    }

    return {};
}

void dx12_ri_param_block_archetype::free(allocation alloc)
{
    std::scoped_lock lock(m_allocation_mutex);

    m_pages[alloc.pool_index].free_list.push_back(alloc.allocation_index);
}

void dx12_ri_param_block_archetype::add_page()
{
    std::scoped_lock lock(m_allocation_mutex);

    alloc_page& instance = m_pages.emplace_back();

    D3D12_HEAP_PROPERTIES heap_properties;
    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_properties.CreationNodeMask = 0;
    heap_properties.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc;
    desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.DepthOrArraySize = 1;
    desc.Width = m_instance_stride * k_page_size;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    HRESULT hr = m_renderer.get_device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&instance.handle)
    );
    if (FAILED(hr))
    {
        db_fatal(render_interface, "CreateCommittedResource failed with error 0x%08x.", hr);
    }

    // Set a debug name.
    instance.handle->SetName(L"Param Block Archetype Page");
    instance.base_address_gpu = reinterpret_cast<uint8_t*>(instance.handle->GetGPUVirtualAddress());

    // TODO: We should copy to default heap after modifying.
    D3D12_RANGE range;
    range.Begin = 0;
    range.End = desc.Width;
    hr = instance.handle->Map(0, &range, reinterpret_cast<void**>(&instance.base_address_cpu));
    if (FAILED(hr))
    {
        db_fatal(render_interface, "Failed to map param block memory with error 0x%08x.", hr);
    }

    // Zero out the entire param block memory.
    memset(instance.base_address_cpu, 0, desc.Width);

    // Fill page free list with all instances in the heap.
    instance.free_list.resize(k_page_size);
    for (size_t i = 0; i < k_page_size; i++)
    {
        instance.free_list[i] = i;
    }
}

std::unique_ptr<ri_param_block> dx12_ri_param_block_archetype::create_param_block()
{
    return std::make_unique<dx12_ri_param_block>(m_renderer, *this);
}

dx12_ri_layout_factory& dx12_ri_param_block_archetype::get_layout_factory()
{
    return static_cast<dx12_ri_layout_factory&>(*m_layout_factory);
}

const char* dx12_ri_param_block_archetype::get_name()
{
    return m_debug_name.c_str();
}

bool dx12_ri_param_block_archetype::allocation::is_valid() const
{
    return valid;
}

}; // namespace ws