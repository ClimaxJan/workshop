// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.renderer/render_object.h"
#include "workshop.renderer/render_batch_manager.h"
#include "workshop.renderer/assets/model/model.h"
#include "workshop.assets/asset_manager.h"
#include "workshop.core/utils/event.h"

namespace ws {

// ================================================================================================
//  Represets a static non-animated mesh within the scene.
// ================================================================================================
class render_static_mesh : public render_object
{
public:
    render_static_mesh(render_object_id id, renderer& renderer);
    virtual ~render_static_mesh();

    // Gets/sets the model this static mesh renders with.
    asset_ptr<model> get_model();
    void set_model(const asset_ptr<model>& model);

    // Overrides the normal set transform to update instance data when the transform changes.
    virtual void set_local_transform(const vector3& location, const quat& rotation, const vector3& scale) override;

    // Overrides the default bounds to return the obb of the model bounds.
    virtual obb get_bounds() override;

    // Called when the bounds of an object is modified.
    virtual void bounds_modified() override;

protected:

    // Creates/destroys data required to render this mesh - instance data/etc.
    void create_render_data();
    void destroy_render_data();
    void recreate_render_data();

    // Updates the properties contained in the render data - transform/etc - to reflect
    // any changes that have been made to the static mesh.
    void update_render_data();

    // Registers/unregisters callbacks for asset changes.
    void register_asset_change_callbacks();
    void unregister_asset_change_callbacks();

private:
    asset_ptr<model> m_model;

    std::unique_ptr<ri_param_block> m_geometry_instance_info;

    std::vector<render_batch_instance> m_registered_batches;

    struct mesh_visibility
    {
        render_visibility_manager::object_id id;
        size_t mesh_index;
    };

    std::vector<mesh_visibility> m_mesh_visibility;

    struct material_callback
    {
        asset_ptr<material> material;
        event<>::key_t key;
    };

    event<>::key_t m_model_change_callback_key = 0;
    std::vector<material_callback> m_material_change_callback_keys;

};

}; // namespace ws
