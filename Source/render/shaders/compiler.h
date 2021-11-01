#pragma once

#include "utils/buffer.h"
#include "utils/cache_manager.h"
extern CCacheManager cached_shaders;

bool compileShader(const char* filename, const char* entry_point, const char* shader_model, TBuffer& out_buffer);
