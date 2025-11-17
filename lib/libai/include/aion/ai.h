/*
 * AION OS - AI Library Header
 * Public interface for AI library
 */

#ifndef AION_LIBAI_H
#define AION_LIBAI_H

#include "aion/types.h"

// AI Library Initialization
bool libai_init(void);

// AI Inference Functions
aion_status_t libai_inference(void* model, void* input, void* output);
aion_status_t libai_load_model(const char* path, void** model);

// Model Management
aion_status_t libai_unload_model(void* model);
aion_status_t libai_optimize_model(void* model);

#endif