


#pragma once

#include <cstdarg>
#include <cstdio>

namespace rtlog 
{
    class Logger
    {
    public:
        void Log(const char* fstring, ...) { 
            va_list args;
            va_start(args, fstring);
            vprintf(fstring, args);
            va_end(args);
        }
    };
}
