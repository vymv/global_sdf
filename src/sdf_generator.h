#pragma once

#include "sdf.h"

SDF* generate_sdf(uint32_t resolution, uint32_t vertex_count, float* vertices, uint32_t index_count, uint32_t* indices);