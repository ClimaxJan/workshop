// ================================================================================================
//  workshop
//  Copyright (C) 2021 Tim Leonard
// ================================================================================================
#pragma once

#include <string>
#include <vector>
#include <mutex>

namespace ws {

// ================================================================================================
//  Defines the severity of a log messages.
//  The log level is purely for semantics, there are no behaviour difference when writing any of them.
// ================================================================================================
enum class log_level
{
    fatal,
    error,
    warning,
    success,
    log,
    verbose,

    count
};

// ================================================================================================
//  Defines the source of a log message.
// ================================================================================================
enum class log_source
{
    core,
    engine,
    game,
    window,
    render_interface,
    renderer,
    asset,

    count
};

// ================================================================================================
//  This is an abstract base class that handles all debug message logging.
//  Each instanced of a derive class automatically registers itself for notifications of new 
//  log messages. These messages are recieved via the write virtual function.
// ================================================================================================
struct log_handler
{
public:
    
    log_handler();
    virtual ~log_handler();

    // Called each time a message is recieved that needs to be logged. The log_level is already
    // formatted into the message, its passed in only so log handlers can make visual changes
    // if wanted (eg. terminal color changes).
    virtual void write(log_level level, const std::string& message) = 0;

    // This is the entry point for all log message macros defined in debug.h. This will appropriatly
    // format the recieve data before forwarding it to all instanced log_handlers.
    static void static_write(log_level level, log_source source, const char* format, ...);

    // Sets the maximum log level to show. All logs beyond this will be ignored.
    static void set_max_log_level(log_level level);

protected:

    static void static_write_formatted(log_level level, log_source source, const char* log);

private:

    // How much stack space to use for formatting logs. If logs require
    // more than this then heap memory will be allocated.
    inline static constexpr int k_stack_space = 256;

#ifdef WS_DEBUG
    inline static log_level m_max_log_level = log_level::verbose;
#else
    inline static log_level m_max_log_level = log_level::log;
#endif

    inline static std::recursive_mutex m_handlers_mutex;
    inline static std::vector<log_handler*> m_handlers;

};

}; // namespace workshop
