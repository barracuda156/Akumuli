#pragma once
#include <akumuli.h>
#include <string>

namespace Akumuli {
namespace V2 {

struct Logger
{
    static aku_logger_cb_t set_logger(aku_logger_cb_t new_logger);
    static void msg(aku_LogLevel lvl, const char* msg);
    static void msg(aku_LogLevel lvl, std::string msg);
};

}}  // namespaces
