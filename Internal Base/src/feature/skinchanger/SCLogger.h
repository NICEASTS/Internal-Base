#pragma once
#include <cstdarg>


class SCLogger
{
public:
    void Init() {}
    void Log(const char*, ...) {}
};

inline SCLogger g_Logger;

