// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include "workshop.assets/asset_loader.h"
#include "workshop.renderer/assets/model/model.h"

namespace ws {

class asset;
class model;
class render_interface;
class ri_interface;
class renderer;

// ================================================================================================
//  Loads model files.
// 
//  Model files contain vertex/index data along with misc data such as
//  animations and material references.
// ================================================================================================
class model_loader : public asset_loader
{
public:
    model_loader(ri_interface& instance, renderer& renderer);

    virtual const std::type_info& get_type() override;
    virtual asset* get_default_asset() override;
    virtual asset* load(const char* path) override;
    virtual void unload(asset* instance) override;
    virtual bool compile(const char* input_path, const char* output_path, platform_type asset_platform, config_type asset_config) override;
    virtual size_t get_compiled_version() override;

private:
    bool serialize(const char* path, model& asset, bool isSaving);

    bool save(const char* path, model& asset);

    bool parse_file(const char* path, model& asset);

private:
    ri_interface& m_ri_interface;
    renderer& m_renderer;

};

}; // namespace workshop