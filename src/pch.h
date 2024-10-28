// pch.h
#pragma once

// Third-party includes
#include <spdlog/sinks/basic_file_sink.h>

// CommonLib includes
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

// Standard library includes used everywhere
#include <string>
#include <format>

namespace logger = SKSE::log;
using namespace std::literals;