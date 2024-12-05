#include "Logger.h"

namespace Logger {

    void Logger::Log(const std::string& masege)
    {
        OutputDebugStringA(masege.c_str());
    }

}