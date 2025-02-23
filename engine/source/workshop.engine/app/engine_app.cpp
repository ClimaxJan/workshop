// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#include "workshop.engine/app/engine_app.h"
#include "workshop.engine/engine/engine.h"
#include "workshop.core/utils/init_list.h"

namespace ws {

engine_app::engine_app()
    : app()
{
    m_engine = std::make_unique<engine>();
}

engine_app::~engine_app()
{
    m_engine = nullptr;
}

engine& engine_app::get_engine()
{
    return *m_engine;
}

void engine_app::register_init(init_list& list)
{
    app::register_init(list);

    list.add_step(
        "Configure Engine",
        [this]() -> result<void> { configure_engine(*m_engine); return true; },
        [this]() -> result<void> { return true; }
    );

    m_engine->register_init(list);
}

void engine_app::configure_engine(engine& engine)
{
    // Implemented in derived classes.
}

result<void> engine_app::loop()
{
    while (!is_quitting())
    {
        m_engine->step();

        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return true;
}

}; // namespace ws
