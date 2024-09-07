#pragma once

#define WIN32_LEAN_AND_MEAN
#include "xor.hpp"
#include <ArduinoJson.h>
#include <Windows.h>
#include <filesystem>
#include <string>
#include <vector>

#ifdef DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif