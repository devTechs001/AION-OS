#ifndef CV_ENGINE_H
#define CV_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

// Image Format
typedef struct {
    uint8_t* data;
    int width;
    int height;
    int channels;  // 1=grayscale, 3=RGB, 4=RGBA
    int stride;
    
    char format[16];  // "RGB", "RGBA", "GRAY", "YUV"
} cv_image_t;

// Bounding Box
typedef struct {
    int x;
    int y;
    int width;
    int height;
    float confidence;
    int class_id;
    char class_name[64];
} cv_bbox_t;

// Object Detection Result
typedef struct {
    cv_bbox_t* boxes;
    int num_boxes;
    
    uint64_t inference_time_us;
} cv_detection_result_t;

// OCR Result
typedef struct {
    char text[4096];
    
    struct {
        cv_bbox_t bbox;
        char text[256];
        float confidence;
    } words[512];
    int num_words;
    
    uint64_t processing_time_us;
} cv_ocr_result_t;

// Face Detection Result
typedef struct {
    struct {
        cv_bbox_t bbox;
        
        // Facial landmarks
        struct {
            int x, y;
        } landmarks[5];  // eyes, nose, mouth corners
        
        float confidence;
    } faces[32];
    int num_faces;
} cv_face_result_t;

// Computer Vision Engine
typedef struct {
    // Models
    void* object_detection_model;  // YOLO or SSD
    void* ocr_model;               // Tesseract or CRNN
    void* face_detection_model;    // MTCNN or RetinaFace
    void* image_classification_model;  // ResNet or MobileNet
    
    // Statistics
    uint64_t images_processed;
    uint64_t objects_detected;
    uint64_t text_recognized;
    
    spinlock_t lock;
} cv_engine_t;

// Function Prototypes
void cv_init(void);
cv_engine_t* cv_get_engine(void);

// Image Operations
cv_image_t* cv_load_image(const char* filename);
cv_image_t* cv_create_image(int width, int height, int channels);
void cv_free_image(cv_image_t* image);
void cv_save_image(cv_image_t* image, const char* filename);

// Image Processing
cv_image_t* cv_resize(cv_image_t* src, int new_width, int new_height);
cv_image_t* cv_convert_color(cv_image_t* src, const char* target_format);
cv_image_t* cv_crop(cv_image_t* src, int x, int y, int width, int height);
void cv_apply_gaussian_blur(cv_image_t* image, float sigma);
void cv_edge_detection(cv_image_t* image);

// Object Detection
cv_detection_result_t* cv_detect_objects(cv_image_t* image);
void cv_free_detection_result(cv_detection_result_t* result);

// OCR (Optical Character Recognition)
cv_ocr_result_t* cv_recognize_text(cv_image_t* image);
void cv_free_ocr_result(cv_ocr_result_t* result);

// Face Detection
cv_face_result_t* cv_detect_faces(cv_image_t* image);
void cv_free_face_result(cv_face_result_t* result);

// Image Classification
char* cv_classify_image(cv_image_t* image, float* confidence);

// Drawing
void cv_draw_box(cv_image_t* image, cv_bbox_t* box, uint8_t r, uint8_t g, uint8_t b);
void cv_draw_text(cv_image_t* image, const char* text, int x, int y);

#endif // CV_ENGINE_H