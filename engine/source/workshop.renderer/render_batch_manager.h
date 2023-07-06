// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.core/utils/init_list.h"
#include "workshop.core/hashing/hash.h"
#include "workshop.core/hashing/string_hash.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.renderer/render_effect.h"
#include "workshop.render_interface/ri_param_block.h"

#include <unordered_map>
#include <string>

namespace ws {

class renderer;
class asset_manager;
class shader;
class render_object;
class model;
class material;
class ri_buffer;
enum class material_domain;

// ================================================================================================
//  This enum dictates what kind of things this batch is going to be used for rendering.
// ================================================================================================
enum class render_batch_usage
{
    static_mesh
};

// ================================================================================================
//  Contains all the identifying information for an individual batch. All instances in a batch
//  will contain the same key
// ================================================================================================
struct render_batch_key
{
    asset_ptr<model> model;
    size_t material_index;
    material_domain domain;
    render_batch_usage usage;

    bool operator==(const render_batch_key& other) const
    {
        return model == other.model && 
               material_index == other.material_index && 
               domain == other.domain &&
               usage == other.usage;
    }

    bool operator!=(const render_batch_key& other) const
    {
        return !operator==(other);
    }
};

// ================================================================================================
//  Represents an individual instances in a render batch.
// ================================================================================================
struct render_batch_instance
{
    // Key for which batch this instance should be sorted into.
    render_batch_key key;

    // Object this instance refers to.
    render_object* object;

    // Param block containing instance specific fields.
    ri_param_block* param_block;
};

}; // namespace ws

// ================================================================================================
//  Specialization hash for batch types.
// ================================================================================================
template<>
struct std::hash<ws::render_batch_key>
{
    std::size_t operator()(const ws::render_batch_key& k) const
    {
        size_t hash = 0;
        ws::hash_combine(hash, k.material_index);
        ws::hash_combine(hash, k.model);
        ws::hash_combine(hash, k.domain);
        ws::hash_combine(hash, k.usage);
        return hash;
    }
};

template<>
struct std::hash<ws::asset_ptr<ws::model>>
{
    std::size_t operator()(const ws::asset_ptr<ws::model>& k) const
    {
        size_t hash = 0;
        ws::hash_combine(hash, k.get_hash());
        return hash;
    }
};

namespace ws {

// ================================================================================================
//  Represents a buffer that holds offsets of instance param blocks associated with
//  a batch. 
// ================================================================================================
class render_batch_instance_buffer
{
public:
    render_batch_instance_buffer(renderer& render);

    void add(uint32_t table_index, uint32_t table_offset);
    void commit();
    size_t size();
    size_t capacity();

    ri_buffer& get_buffer();

private:
    void resize(size_t size);

private:
    renderer& m_renderer;
    std::unique_ptr<ri_buffer> m_buffer;

    struct slot
    {
        bool dirty = false;
        size_t table_index = 0;
        size_t table_offset = 0;
        size_t last_frame_used = 0;
    };
    std::vector<slot> m_slots;

    // Minimum number of slots the buffer starts with.
    static inline constexpr size_t k_min_slot_count = 128;

    // Factor the buffer grows by.
    static inline constexpr size_t k_slot_growth_factor = 2;

};

// ================================================================================================
//  A unique render batch that buckets a set of renderable instances
//  with similar properties.
// ================================================================================================
class render_batch
{
public:
    render_batch(render_batch_key key, renderer& render);

    render_batch_key get_key();
    const std::vector<render_batch_instance>& get_instances();

    void clear();

    // Finds a param block matching the given key, if one is not found, one is
    // created and the creation callback is called for it.
    ri_param_block* find_or_create_param_block(
        void* key, 
        const char* param_block_name, 
        std::function<void(ri_param_block& block)> creation_callback);

    // Finds or creates an instance buffer with the matching key, if one is not
    // found a new one is created.
    render_batch_instance_buffer* find_or_create_instance_buffer(void* key);

private:
    friend class render_batch_manager;

    void add_instance(const render_batch_instance& instance);
    void remove_instance(const render_batch_instance& instance);

    // Clears all param blocks/instance buffers/etc that are cached in this batch.
    void clear_cached_data();

private:
    render_batch_key m_key;
    renderer& m_renderer;
    std::vector<render_batch_instance> m_instances;

    struct param_block
    {
        void* key;
        std::string name;
        std::unique_ptr<ri_param_block> block;
    };
    std::vector<param_block> m_blocks;

    struct instance_buffer
    {
        void* key;
        std::unique_ptr<render_batch_instance_buffer> buffer;
    };
    std::unordered_map<void*, instance_buffer> m_instance_buffers;    

};

// ================================================================================================
//  Responsible for calculating and storing batching information.
// ================================================================================================
class render_batch_manager
{
public:

    render_batch_manager(renderer& render);

    // Registers all the steps required to initialize the system.
    void register_init(init_list& list);

    // Called at the start of a frame.
    void begin_frame();

    // Registers a render instances and inserts it into an active batch.
    void register_instance(const render_batch_instance& instance);

    // Unregisters a render instance and removes it from all active batches.
    void unregister_instance(const render_batch_instance& instance);

    // Gets all the batches that have the given domain and usage.
    std::vector<render_batch*> get_batches(material_domain domain, render_batch_usage usage);

    // Clears all cached data.
    void clear_cached_data();

    // Invalidates any cached state that uses the given materail.
    void clear_cached_material_data(material* material);

protected:
    
    // Finds or creates a batch that uses the given key.
    render_batch* find_or_create_batch(render_batch_key key);

private:

    renderer& m_renderer;

    std::unordered_map<render_batch_key, std::unique_ptr<render_batch>> m_batches;

};

}; // namespace ws