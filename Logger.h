#pragma once

#ifndef _AMD64_
#define _AMD64_
#endif

#include <string>
#include <ostream>
#include <debugapi.h>
#include <xstring>


namespace Logger
{
    void Log(const std::string& masege);
};