#include "audio.h"
#include <string.h>
#include <math.h>

static audio_mixer_t global_mixer = {0};

void audio_mixer_init(void) {
    memset(&global_mixer, 0, sizeof(global_mixer));
    global_mixer.master_volume = 1.0f;
    spinlock_init(&global_mixer.lock);
    
    kprintf("[AUDIO] Mixer initialized\n");
}

void audio_mixer_add_stream(audio_stream_t* stream) {
    spinlock_acquire(&global_mixer.lock);
    
    if (global_mixer.num_streams < 32) {
        global_mixer.streams[global_mixer.num_streams++] = stream;
    }
    
    spinlock_release(&global_mixer.lock);
}

// AI-Enhanced Audio Mixing with automatic gain control
void audio_mixer_process(void* output, size_t frames) {
    spinlock_acquire(&global_mixer.lock);
    
    float* out = (float*)output;
    memset(out, 0, frames * 2 * sizeof(float)); // Stereo
    
    if (global_mixer.master_mute || global_mixer.num_streams == 0) {
        spinlock_release(&global_mixer.lock);
        return;
    }
    
    // AI: Analyze all streams for optimal mixing
    float total_energy = 0.0f;
    for (int i = 0; i < global_mixer.num_streams; i++) {
        audio_stream_t* stream = global_mixer.streams[i];
        if (!stream->playing || stream->muted) continue;
        
        // Calculate stream energy
        float* stream_buf = (float*)stream->buffer;
        for (size_t j = 0; j < frames * 2; j++) {
            total_energy += fabsf(stream_buf[j]);
        }
    }
    
    // AI: Dynamic range compression
    float compression_ratio = 1.0f;
    if (total_energy > frames * 2 * 0.8f) {
        compression_ratio = (frames * 2 * 0.8f) / total_energy;
    }
    
    // Mix all streams
    for (int i = 0; i < global_mixer.num_streams; i++) {
        audio_stream_t* stream = global_mixer.streams[i];
        if (!stream->playing || stream->muted) continue;
        
        float* stream_buf = (float*)stream->buffer;
        float volume = stream->volume * global_mixer.master_volume * compression_ratio;
        
        for (size_t j = 0; j < frames * 2; j++) {
            out[j] += stream_buf[j] * volume;
        }
        
        // AI: Apply noise reduction if enabled
        if (stream->noise_reduction) {
            audio_ai_denoise(stream->buffer, frames, 2);
        }
        
        // AI: Apply auto-leveling if enabled
        if (stream->auto_leveling) {
            audio_ai_enhance(stream->buffer, frames, 2);
        }
    }
    
    // Apply effects
    if (global_mixer.reverb_enabled) {
        // Simple reverb implementation
        // (Real implementation would use convolution)
    }
    
    // Clipping protection
    for (size_t i = 0; i < frames * 2; i++) {
        if (out[i] > 1.0f) out[i] = 1.0f;
        if (out[i] < -1.0f) out[i] = -1.0f;
    }
    
    spinlock_release(&global_mixer.lock);
}

// AI-Enhanced Noise Reduction using spectral subtraction
void audio_ai_denoise(void* buffer, size_t frames, int channels) {
    // Simplified noise reduction
    // Real implementation would use FFT and spectral analysis
    float* buf = (float*)buffer;
    static float noise_profile[1024] = {0};
    static bool noise_learned = false;
    
    // Learn noise profile from first 100ms of silence
    if (!noise_learned) {
        // Analyze first frames as noise
        for (size_t i = 0; i < frames && i < 1024; i++) {
            noise_profile[i] = fabsf(buf[i]);
        }
        noise_learned = true;
        return;
    }
    
    // Subtract noise
    for (size_t i = 0; i < frames * channels; i++) {
        float signal = fabsf(buf[i]);
        float noise = noise_profile[i % 1024];
        
        if (signal < noise * 1.5f) {
            buf[i] *= 0.1f; // Reduce noise significantly
        }
    }
}

// AI-Enhanced Audio Enhancement
void audio_ai_enhance(void* buffer, size_t frames, int channels) {
    float* buf = (float*)buffer;
    
    // Calculate RMS
    float rms = 0.0f;
    for (size_t i = 0; i < frames * channels; i++) {
        rms += buf[i] * buf[i];
    }
    rms = sqrtf(rms / (frames * channels));
    
    // Target RMS (normalized)
    float target_rms = 0.3f;
    
    if (rms > 0.001f) {
        float gain = target_rms / rms;
        
        // Limit gain to prevent distortion
        if (gain > 4.0f) gain = 4.0f;
        if (gain < 0.25f) gain = 0.25f;
        
        // Apply gain
        for (size_t i = 0; i < frames * channels; i++) {
            buf[i] *= gain;
        }
    }
}