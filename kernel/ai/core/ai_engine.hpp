/*
 * AION OS - AI Engine C++ Header
 * Advanced AI engine interface
 */

#ifndef AION_AI_ENGINE_HPP
#define AION_AI_ENGINE_HPP

#include <aion/types.h>
#include <vector>
#include <memory>

namespace aion::ai {

class AIEngine {
public:
    // Initialize the AI engine
    bool initialize();
    
    // Run inference
    bool run_inference(void* model, void* input, void* output);
    
    // Load model
    bool load_model(const char* path);
    
    // Get engine instance
    static AIEngine& get_instance();
    
private:
    bool initialize_neural_runtime();
    bool initialize_model_manager();
    bool initialize_inference_engine();
    
    // Private members
    std::vector<void*> loaded_models_;
    void* neural_runtime_;
    void* model_manager_;
    void* inference_engine_;
};

} // namespace aion::ai

#endif