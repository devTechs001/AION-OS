#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>
#include <stdbool.h>

// Audio Formats
#define AUDIO_FORMAT_PCM_U8     1
#define AUDIO_FORMAT_PCM_S16LE  2
#define AUDIO_FORMAT_PCM_S24LE  3
#define AUDIO_FORMAT_PCM_S32LE  4
#define AUDIO_FORMAT_PCM_FLOAT  5

// Sample Rates
#define SAMPLE_RATE_8000    8000
#define SAMPLE_RATE_16000   16000
#define SAMPLE_RATE_22050   22050
#define SAMPLE_RATE_44100   44100
#define SAMPLE_RATE_48000   48000
#define SAMPLE_RATE_96000   96000

// Audio Device
typedef struct audio_device {
    char name[64];
    int id;
    
    // Capabilities
    uint32_t supported_formats;
    uint32_t supported_rates[16];
    int num_channels;
    
    // Current Configuration
    int format;
    int sample_rate;
    int channels;
    int buffer_size;
    
    // Operations
    int (*open)(struct audio_device* dev);
    int (*close)(struct audio_device* dev);
    int (*write)(struct audio_device* dev, const void* buffer, size_t size);
    int (*read)(struct audio_device* dev, void* buffer, size_t size);
    int (*set_volume)(struct audio_device* dev, int volume);
    
    void* private_data;
} audio_device_t;

// Audio Stream
typedef struct {
    int id;
    audio_device_t* device;
    
    int format;
    int sample_rate;
    int channels;
    
    void* buffer;
    size_t buffer_size;
    size_t buffer_pos;
    
    float volume;
    bool muted;
    bool playing;
    
    // AI Enhancement
    bool noise_reduction;
    bool auto_leveling;
    float* equalizer_bands; // 10-band EQ
} audio_stream_t;

// Audio Mixer
typedef struct {
    audio_stream_t* streams[32];
    int num_streams;
    
    float master_volume;
    bool master_mute;
    
    // Effects
    bool reverb_enabled;
    bool echo_enabled;
    float bass_boost;
    float treble_boost;
    
    spinlock_t lock;
} audio_mixer_t;

// Initialize audio subsystem
void audio_init(void);

// Device Management
int audio_register_device(audio_device_t* dev);
audio_device_t* audio_get_device(int id);
void audio_list_devices(void);

// Stream Management
audio_stream_t* audio_create_stream(int device_id, int format, 
                                    int sample_rate, int channels);
void audio_destroy_stream(audio_stream_t* stream);
int audio_play_stream(audio_stream_t* stream, const void* data, size_t size);
void audio_pause_stream(audio_stream_t* stream);
void audio_resume_stream(audio_stream_t* stream);
void audio_set_volume(audio_stream_t* stream, float volume);

// Mixer
void audio_mixer_init(void);
void audio_mixer_add_stream(audio_stream_t* stream);
void audio_mixer_remove_stream(audio_stream_t* stream);
void audio_mixer_process(void* output, size_t frames);

// AI-Enhanced Processing
void audio_ai_denoise(void* buffer, size_t frames, int channels);
void audio_ai_enhance(void* buffer, size_t frames, int channels);
void audio_ai_spatialize(void* buffer, size_t frames, float* position);

#endif // AUDIO_H