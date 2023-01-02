// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.render_interface/ri_interface.h"
#include "workshop.render_interface.dx12/dx12_ri_descriptor_heap.h"
#include "workshop.render_interface.dx12/dx12_headers.h"
#include <array>

namespace ws {

class dx12_ri_command_queue;
class dx12_ri_upload_manager;
class dx12_ri_descriptor_table;

// ================================================================================================
//  Implementation of a renderer using DirectX 12.
// ================================================================================================
class dx12_render_interface : public ri_interface
{
public:
    // How many frames can be in the pipeline at a given time, including
    // the one currently being built. 
    // The number of swap chain targets is one lower than this.
    constexpr static size_t k_max_pipeline_depth = 3;

    // Size of the SRV/UAV/CBV heap.
    constexpr static size_t k_srv_heap_size = 1'000'000;

    // Size of the sampler heap.
    constexpr static size_t k_sampler_heap_size = 2048;

    // Size of the render target heap.
    constexpr static size_t k_rtv_heap_size = 1000;

    // Size of the depth stencil target heap.
    constexpr static size_t k_dsv_heap_size = 1000;

    // Maximum amount of descriptors each srv table can use.
    constexpr static size_t k_srv_descriptor_table_size = 200'000;

    // Maximum amount of descriptors each sampler table can use.
    constexpr static size_t k_sampler_descriptor_table_size = k_sampler_heap_size;

    // Maximum amount of descriptors each rtv table can use.
    constexpr static size_t k_rtv_descriptor_table_size = k_rtv_heap_size;

    // Maximum amount of descriptors each dsv table can use.
    constexpr static size_t k_dsv_descriptor_table_size = k_dsv_heap_size;

    using deferred_delete_function_t = std::function<void()>;
 
    dx12_render_interface();
    virtual ~dx12_render_interface();

    virtual void register_init(init_list& list) override;
    virtual void begin_frame() override;
    virtual void end_frame() override;
    virtual std::unique_ptr<ri_swapchain> create_swapchain(window& for_window, const char* debug_name) override;
    virtual std::unique_ptr<ri_fence> create_fence(const char* debug_name) override;
    virtual std::unique_ptr<ri_shader_compiler> create_shader_compiler() override;
    virtual std::unique_ptr<ri_pipeline> create_pipeline(const ri_pipeline::create_params& params, const char* debug_name) override;
    virtual std::unique_ptr<ri_param_block_archetype> create_param_block_archetype(const ri_param_block_archetype::create_params& params, const char* debug_name) override;
    virtual std::unique_ptr<ri_texture> create_texture(const ri_texture::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_sampler> create_sampler(const ri_sampler::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_buffer> create_buffer(const ri_buffer::create_params& params, const char* debug_name = nullptr) override;
    virtual std::unique_ptr<ri_layout_factory> create_layout_factory(ri_data_layout layout) override;
    virtual ri_command_queue& get_graphics_queue() override;
    virtual ri_command_queue& get_copy_queue() override;
    virtual size_t get_pipeline_depth() override;

    dx12_ri_upload_manager& get_upload_manager();

    bool is_tearing_allowed();
    Microsoft::WRL::ComPtr<IDXGIFactory4> get_dxgi_factory();
    Microsoft::WRL::ComPtr<ID3D12Device> get_device();

    dx12_ri_descriptor_heap& get_srv_descriptor_heap();
    dx12_ri_descriptor_heap& get_sampler_descriptor_heap();
    dx12_ri_descriptor_heap& get_rtv_descriptor_heap();
    dx12_ri_descriptor_heap& get_dsv_descriptor_heap();

    dx12_ri_descriptor_table& get_descriptor_table(ri_descriptor_table table);

    size_t get_frame_index();

    // Used to defer a resource deletion until the gpu is no longer referencing it.
    void defer_delete(const deferred_delete_function_t& func);

private:

    result<void> create_device();
    result<void> destroy_device();

    result<void> create_command_queues();
    result<void> destroy_command_queues();

    result<void> create_heaps();
    result<void> destroy_heaps();

    result<void> create_misc();
    result<void> destroy_misc();

    result<void> select_adapter();
    result<void> check_feature_support();

    void process_pending_deletes();

private:

    Microsoft::WRL::ComPtr<ID3D12Device> m_device = nullptr;

    std::unique_ptr<dx12_ri_command_queue> m_graphics_queue = nullptr;
    std::unique_ptr<dx12_ri_command_queue> m_copy_queue = nullptr;
    std::unique_ptr<dx12_ri_upload_manager> m_upload_manager = nullptr;

    std::unique_ptr<dx12_ri_descriptor_heap> m_srv_descriptor_heap = nullptr;
    std::unique_ptr<dx12_ri_descriptor_heap> m_sampler_descriptor_heap = nullptr;
    std::unique_ptr<dx12_ri_descriptor_heap> m_rtv_descriptor_heap = nullptr;
    std::unique_ptr<dx12_ri_descriptor_heap> m_dsv_descriptor_heap = nullptr;

    std::array<std::unique_ptr<dx12_ri_descriptor_table>, static_cast<int>(ri_descriptor_table::COUNT)> m_descriptor_tables;

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgi_factory = nullptr;
    Microsoft::WRL::ComPtr<IDXGIFactory5> m_dxgi_factory_5 = nullptr;
    Microsoft::WRL::ComPtr<IDXGIAdapter4> m_dxgi_adapter = nullptr;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> m_info_queue = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Debug> m_debug_interface = nullptr;

    D3D12_FEATURE_DATA_D3D12_OPTIONS m_options = {};
    bool m_allow_tearing = false;

    size_t m_frame_index = 0;

    std::array<std::vector<deferred_delete_function_t>, k_max_pipeline_depth> m_pending_deletions;

};

}; // namespace ws