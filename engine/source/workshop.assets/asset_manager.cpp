// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.assets/asset_manager.h"
#include "workshop.assets/asset_loader.h"
#include "workshop.assets/asset_cache.h"
#include "workshop.assets/asset.h"
#include "workshop.core/debug/debug.h"
#include "workshop.core/async/async.h"
#include "workshop.core/filesystem/virtual_file_system.h"

#include <algorithm>
#include <thread>
#include <future>
#include <array>

// The asset manager is multithreaded, its important to know how it behaves before 
// attempting to make changes to it.
//
// An asset is first requested via a call to request_asset, this returns an asset_ptr
// which can be used to check the current state of the asset.
// 
// When the asset is requested a call to request_load is made which marks the asset
// as wanting to be loaded and notifies a background thread (which runs do_work) 
// that an asset state has changed.
//
// When all reference to an asset are lost a call to request_unload is made that 
// will mark the asset as wanting to be unloaded and notify the background thread.
//
// The background thread wakes up whenever notified and looks at pending tasks, if
// there are less in-process operations (loads or unloads) it task a pending task
// and begins processing it (in process_asset). 
//
// Processing an asset involes essentially running a state machine to determine if
// the asset is in the state it wants to be in and if not it will call begin_load
// or begin_unload to start changing to the start it wants to be in.
//
// begin_load and begin_unload queue asynchronous operations which run in the 
// task_scheduler worker pool. Once they finish doing their task process_asset
// is called again incase its state has changed while the operation has been in progress.
// 
// If the task is now in the correct state the asset_manager is done with it until its
// next state change.
//
// All functions accessible to calling-code (requesting an asset, checking an asset state, etc)
// are expected to be thread-safe and callable from anywhere.

namespace ws {

namespace {

static std::array<const char*, static_cast<int>(asset_loading_state::COUNT)> k_loading_state_strings = {
    "Unloaded",
    "Unloading",
    "Loading",
    "Loaded",
    "Failed"
};

};

asset_manager::asset_manager(platform_type asset_platform, config_type asset_config)
    : m_asset_platform(asset_platform)
    , m_asset_config(asset_config)
{
    m_load_thread = std::thread([this]() {

        db_set_thread_name("Asset Manager Coordinator");    
        do_work();

    });
}

asset_manager::~asset_manager()
{
    {
        std::unique_lock lock(m_states_mutex);
        m_shutting_down = true;
        m_states_convar.notify_all();
    }

    m_load_thread.join();
    m_load_thread = std::thread();
}

asset_manager::loader_id asset_manager::register_loader(std::unique_ptr<asset_loader>&& loader)
{
    std::scoped_lock lock(m_loaders_mutex);
    
    registered_loader handle;
    handle.loader = std::move(loader);
    handle.id = m_next_loader_id++;
    m_loaders.push_back(std::move(handle));

    return handle.id;
}

void asset_manager::unregister_loader(loader_id id)
{
    std::scoped_lock lock(m_loaders_mutex);

    auto iter = std::find_if(m_loaders.begin(), m_loaders.end(), [id](const registered_loader& handle){
        return handle.id == id;
    });

    if (iter != m_loaders.end())
    {
        m_loaders.erase(iter);
    }
}

asset_manager::cache_id asset_manager::register_cache(std::unique_ptr<asset_cache>&& cache)
{
    std::scoped_lock lock(m_cache_mutex);
    
    registered_cache handle;
    handle.cache = std::move(cache);
    handle.id = m_next_cache_id++;
    m_caches.push_back(std::move(handle));

    return handle.id;
}

void asset_manager::unregister_cache(cache_id id)
{
    std::scoped_lock lock(m_cache_mutex);

    auto iter = std::find_if(m_caches.begin(), m_caches.end(), [id](const registered_cache& handle){
        return handle.id == id;
    });

    if (iter != m_caches.end())
    {
        m_caches.erase(iter);
    }
}

asset_loader* asset_manager::get_loader_for_type(const std::type_info* id)
{
    std::scoped_lock lock(m_loaders_mutex);

    auto iter = std::find_if(m_loaders.begin(), m_loaders.end(), [id](const registered_loader& handle){
        return &handle.loader->get_type() == id;
    });

    if (iter != m_loaders.end())
    {
        return iter->loader.get();
    }

    return nullptr;
}

void asset_manager::drain_queue()
{
    std::unique_lock lock(m_states_mutex);

    while (m_pending_queue.size() > 0)
    {
        m_states_convar.wait(lock);
    }
}

asset_state* asset_manager::create_asset_state(const std::type_info& id, const char* path, int32_t priority)
{
    std::unique_lock lock(m_states_mutex);

    asset_state* state = new asset_state();
    state->default_asset = nullptr;
    state->loading_state = asset_loading_state::unloaded;
    state->priority = priority;
    state->references = 0;
    state->type_id = &id;
    state->path = path;

    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader != nullptr)
    {
        state->default_asset = loader->get_default_asset();
    }

    m_states.push_back(state);

    return state;
}

void asset_manager::request_load(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);
    
    state->should_be_loaded = true;

    if (!state->is_pending)
    {
        state->is_pending = true;
        m_pending_queue.push(state);

        m_states_convar.notify_all();
    }
}

void asset_manager::request_unload(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);

    state->should_be_loaded = false;

    if (!state->is_pending)
    {
        state->is_pending = true;
        m_pending_queue.push(state);

        m_states_convar.notify_all();
    }
}

void asset_manager::wait_for_load(asset_state* state)
{
    std::unique_lock lock(m_states_mutex);

    while (true)
    {
        if (state->loading_state == asset_loading_state::loaded ||
            state->loading_state == asset_loading_state::failed ||
            !state->is_pending)
        {
            return;
        }

        m_states_convar.wait(lock);
    }
}

void asset_manager::do_work()
{
    std::unique_lock lock(m_states_mutex);

    while (!m_shutting_down)
    {
        if (m_outstanding_ops.load() < k_max_concurrent_ops)
        {
            if (m_pending_queue.size() > 0)
            {
                asset_state* state = m_pending_queue.front();
                m_pending_queue.pop();

                process_asset(state);

                continue;
            }
        }

        m_states_convar.wait(lock);
    }
}

void asset_manager::process_asset(asset_state* state)
{
    switch (state->loading_state)
    {
    case asset_loading_state::loaded:
        {
            if (!state->should_be_loaded)
            {
                begin_unload(state);
            }
            break;
        }
    case asset_loading_state::unloaded:
        {
            if (state->should_be_loaded)
            {
                begin_load(state);
            }
            break;
        }
    case asset_loading_state::failed:
        {
            // We do nothing to failed assets, they sit in
            // this state and return a default asset if available.
            break;
        }
    default:
        {
            db_assert(false);
            break;
        }
    }
}

void asset_manager::begin_load(asset_state* state)
{
    db_assert(state->loading_state == asset_loading_state::unloaded)
    set_load_state(state, asset_loading_state::loading);

    async("Load Asset", task_queue::loading, [this, state]() {
    
        do_load(state);

        {
            std::unique_lock lock(m_states_mutex);
            set_load_state(state, state->instance ? asset_loading_state::loaded : asset_loading_state::failed);

            // Process the asset again incase the requested state
            // has changed during this process.
            process_asset(state);

            m_states_convar.notify_all();
        }
    
    });
}

void asset_manager::begin_unload(asset_state* state)
{
    db_assert(state->loading_state == asset_loading_state::loaded);
    set_load_state(state, asset_loading_state::unloading);

    async("Unload Asset", task_queue::loading, [this, state]() {
    
        do_unload(state);

        {
            std::unique_lock lock(m_states_mutex);
            set_load_state(state, asset_loading_state::unloaded);

            // Process the asset again incase the requested state
            // has changed during this process.
            process_asset(state);

            m_states_convar.notify_all();
        }
    
    });
}

bool asset_manager::search_cache_for_key(const asset_cache_key& cache_key, std::string& compiled_path)
{
    std::scoped_lock lock(m_cache_mutex);

    for (size_t i = 0; i < m_caches.size(); i++)
    {
        registered_cache& cache = m_caches[i];
        if (cache.cache->get(cache_key, compiled_path))
        {
            // If we have found it in a later cache, move it to earlier caches.
            // We assume later caches have higher latency/costs to access.
            if (i > 0)
            {
                size_t best_cache = std::numeric_limits<size_t>::max();

                for (size_t j = 0; j < i; i++)
                {
                    registered_cache& other_cache = m_caches[j];
                    if (!other_cache.cache->is_read_only())
                    {
                        db_verbose(asset, "[%s] Migrating compiled asset to earlier cache.", cache_key.source.path.c_str());
                        if (other_cache.cache->set(cache_key, compiled_path.c_str()))
                        {
                            best_cache = j;
                        }
                    }
                }

                // Now that we've populated all the caches, try to get the new path from the best cache.
                if (best_cache != std::numeric_limits<size_t>::max())
                {
                    m_caches[best_cache].cache->get(cache_key, compiled_path);
                }
            }

            return true;
        }
    }

    return false;
}

bool asset_manager::compile_asset(asset_cache_key& cache_key, asset_loader* loader, asset_state* state, std::string& compiled_path)
{
    std::string temporary_path = string_format("temp:%s", to_string(guid::generate()).c_str());

    if (!loader->compile(state->path.c_str(), temporary_path.c_str(), m_asset_platform, m_asset_config))
    {
        db_error(asset, "[%s] Failed to compile asset.", state->path.c_str());
        return false;
    }
    else
    {
        db_log(asset, "[%s] Finished compiling, storing in cache.", state->path.c_str());
    }

    // Store compiled version in all writable caches.
    {
        std::scoped_lock lock(m_cache_mutex);

        for (registered_cache& cache : m_caches)
        {
            if (!cache.cache->is_read_only())
            {
                db_verbose(asset, "[%s] Inserting compiled asset into cache.", state->path.c_str());

                if (cache.cache->set(cache_key, temporary_path.c_str()))
                {
                    if (compiled_path.empty())
                    {
                        cache.cache->get(cache_key, compiled_path);
                    }
                }
            }
        }

        // If we couldn't store it in the cache, use the temporary file directly.
        if (compiled_path.empty())
        {
            compiled_path = temporary_path;
        }

        // Otherwise we can remove the temporary file now.
        else
        {
            virtual_file_system::get().remove(temporary_path.c_str());
        }
    }

    return true;
}

bool asset_manager::get_asset_compiled_path(asset_loader* loader, asset_state* state, std::string& compiled_path)
{
    asset_cache_key cache_key;

    std::string with_compiled_extension = state->path + k_compiled_asset_extension;

    // Try and find compiled version of asset in VFS. 
    if (virtual_file_system::get().type(with_compiled_extension.c_str()) == virtual_file_system_path_type::file)
    {
        compiled_path = with_compiled_extension;
    }

    // Otherwise check in cache for compiled version.
    else if (!m_caches.empty())
    {
        // Generate a key with no dependencies.
        if (!loader->get_cache_key(state->path.c_str(), m_asset_platform, m_asset_config, cache_key, { }))
        {
            db_error(asset, "[%s] Failed to calculate cache key for asset.", state->path.c_str());
            return false;
        }

        // Search for key with no dependencies in cache.
        search_cache_for_key(cache_key, compiled_path);

        bool needs_compile = false;

        // Always compile if no compiled asset is found.
        if (compiled_path.empty())
        {
            db_log(asset, "[%s] No compiled version available, compiling now.", state->path.c_str());
            needs_compile = true;
        }
        // If compile asset exists, read the dependency header block and generate a cache key from the data
        // if it differs from the one in the header, we need to rebuild it.
        else
        {
            compiled_asset_header header;

            if (!loader->load_header(compiled_path.c_str(), header))
            {
                db_error(asset, "[%s] Failed to read header from compiled asset: %s", state->path.c_str(), compiled_path.c_str());
                return false;
            }

            asset_cache_key compiled_cache_key;
            if (!loader->get_cache_key(state->path.c_str(), m_asset_platform, m_asset_config, compiled_cache_key, header.dependencies))
            {
                // This can fail if one of our dependencies has been deleted, in which case we know we need to rebuild.
                db_warning(asset, "[%s] Failed to calculate dependency cache key for asset, recompile required.", state->path.c_str());
                needs_compile = true;
            }
            else
            {
                if (compiled_cache_key.hash() != header.compiled_hash)
                {
                    db_warning(asset, "[%s] Compiled asset looks to be out of date, recompile required.", state->path.c_str());
                    needs_compile = true;
                }
            }
        }

        // If no compiled version is available, compile to a temporary location.
        if (needs_compile)
        {
            if (!compile_asset(cache_key, loader, state, compiled_path))
            {
                return false;
            }
        }
    }

    return true;
}

void asset_manager::do_load(asset_state* state)
{
    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader != nullptr)
    {
        std::string compiled_path;

        if (!get_asset_compiled_path(loader, state, compiled_path))
        {
            return;
        }

        // Load the resulting compiled asset.
        if (!compiled_path.empty())
        {
            state->instance = loader->load(compiled_path.c_str());

            if (state->instance)
            {
                if (!state->instance->post_load())
                {
                    db_error(asset, "[%s] Failed to post load asset.", state->path.c_str());

                    loader->unload(state->instance);
                    state->instance = nullptr;
                }
            }
        }
        else
        {
            db_error(asset, "[%s] Failed to find compiled data for asset.", state->path.c_str());
        }
    }
}

void asset_manager::do_unload(asset_state* state)
{
    asset_loader* loader = get_loader_for_type(state->type_id);
    if (loader != nullptr)
    {
        loader->unload(state->instance);
        state->instance = nullptr;
    }
}

void asset_manager::set_load_state(asset_state* state, asset_loading_state new_state)
{
    //db_log(asset, "[%s] %s", state->path.c_str(), k_loading_state_strings[static_cast<int>(new_state)]);
    state->loading_state = new_state;
}

}; // namespace workshop