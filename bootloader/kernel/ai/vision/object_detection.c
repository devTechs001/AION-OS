#include "cv_engine.h"
#include "../ml/tflite.h"
#include <math.h>
#include <string.h>

static cv_engine_t global_cv_engine = {0};

void cv_init(void) {
    memset(&global_cv_engine, 0, sizeof(global_cv_engine));
    spinlock_init(&global_cv_engine.lock);
    
    // Load object detection model (YOLO or MobileNet-SSD)
    tflite_model_t* detection_model = tflite_load_model("/usr/share/ai-vision/models/ssd_mobilenet.tflite");
    if (detection_model) {
        global_cv_engine.object_detection_model = tflite_create_interpreter(detection_model);
        tflite_allocate_tensors(global_cv_engine.object_detection_model);
        
        kprintf("[CV] Object detection model loaded\n");
    }
    
    // Load OCR model
    tflite_model_t* ocr_model = tflite_load_model("/usr/share/ai-vision/models/ocr.tflite");
    if (ocr_model) {
        global_cv_engine.ocr_model = tflite_create_interpreter(ocr_model);
        tflite_allocate_tensors(global_cv_engine.ocr_model);
        
        kprintf("[CV] OCR model loaded\n");
    }
    
    kprintf("[CV] Computer Vision engine initialized\n");
}

// Create blank image
cv_image_t* cv_create_image(int width, int height, int channels) {
    cv_image_t* image = kmalloc(sizeof(cv_image_t));
    
    image->width = width;
    image->height = height;
    image->channels = channels;
    image->stride = width * channels;
    
    image->data = kmalloc(width * height * channels);
    memset(image->data, 0, width * height * channels);
    
    if (channels == 1) strcpy(image->format, "GRAY");
    else if (channels == 3) strcpy(image->format, "RGB");
    else if (channels == 4) strcpy(image->format, "RGBA");
    
    return image;
}

// Resize image (bilinear interpolation)
cv_image_t* cv_resize(cv_image_t* src, int new_width, int new_height) {
    cv_image_t* dst = cv_create_image(new_width, new_height, src->channels);
    
    float x_ratio = (float)src->width / new_width;
    float y_ratio = (float)src->height / new_height;
    
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            float src_x = x * x_ratio;
            float src_y = y * y_ratio;
            
            int x1 = (int)src_x;
            int y1 = (int)src_y;
            int x2 = (x1 + 1 < src->width) ? x1 + 1 : x1;
            int y2 = (y1 + 1 < src->height) ? y1 + 1 : y1;
            
            float dx = src_x - x1;
            float dy = src_y - y1;
            
            for (int c = 0; c < src->channels; c++) {
                float p1 = src->data[(y1 * src->stride) + (x1 * src->channels) + c];
                float p2 = src->data[(y1 * src->stride) + (x2 * src->channels) + c];
                float p3 = src->data[(y2 * src->stride) + (x1 * src->channels) + c];
                float p4 = src->data[(y2 * src->stride) + (x2 * src->channels) + c];
                
                float val = p1 * (1 - dx) * (1 - dy) +
                           p2 * dx * (1 - dy) +
                           p3 * (1 - dx) * dy +
                           p4 * dx * dy;
                
                dst->data[(y * dst->stride) + (x * dst->channels) + c] = (uint8_t)val;
            }
        }
    }
    
    return dst;
}

// Object Detection using SSD MobileNet
cv_detection_result_t* cv_detect_objects(cv_image_t* image) {
    if (!global_cv_engine.object_detection_model) {
        kprintf("[CV] Error: No object detection model loaded\n");
        return NULL;
    }
    
    uint64_t start_time = rdtsc();
    
    spinlock_acquire(&global_cv_engine.lock);
    
    tflite_interpreter_t* interpreter = global_cv_engine.object_detection_model;
    
    // Prepare input (resize to model's input size, e.g., 300x300)
    cv_image_t* resized = cv_resize(image, 300, 300);
    
    // Normalize to [0, 1] and convert to float
    tflite_tensor_t* input_tensor = tflite_get_input_tensor(interpreter, 0);
    float* input_data = (float*)input_tensor->data;
    
    for (int i = 0; i < 300 * 300 * 3; i++) {
        input_data[i] = resized->data[i] / 255.0f;
    }
    
    cv_free_image(resized);
    
    // Run inference
    tflite_invoke(interpreter);
    
    // Get outputs
    // SSD MobileNet typically outputs:
    // - detection_boxes: [1, 10, 4]
    // - detection_classes: [1, 10]
    // - detection_scores: [1, 10]
    // - num_detections: [1]
    
    tflite_tensor_t* boxes_tensor = tflite_get_output_tensor(interpreter, 0);
    tflite_tensor_t* classes_tensor = tflite_get_output_tensor(interpreter, 1);
    tflite_tensor_t* scores_tensor = tflite_get_output_tensor(interpreter, 2);
    tflite_tensor_t* num_tensor = tflite_get_output_tensor(interpreter, 3);
    
    float* boxes = (float*)boxes_tensor->data;
    float* classes = (float*)classes_tensor->data;
    float* scores = (float*)scores_tensor->data;
    int num_detections = (int)((float*)num_tensor->data)[0];
    
    // Filter by confidence threshold (0.5)
    cv_detection_result_t* result = kmalloc(sizeof(cv_detection_result_t));
    result->boxes = kmalloc(num_detections * sizeof(cv_bbox_t));
    result->num_boxes = 0;
    
    // COCO class names (simplified)
    const char* class_names[] = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train",
        "truck", "boat", "traffic light", "fire hydrant", "stop sign",
        "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep",
        "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella"
        // ... (80 classes total in COCO)
    };
    
    for (int i = 0; i < num_detections; i++) {
        float confidence = scores[i];
        
        if (confidence > 0.5f) {
            cv_bbox_t* box = &result->boxes[result->num_boxes];
            
            // Boxes are in format [ymin, xmin, ymax, xmax] normalized to [0, 1]
            float ymin = boxes[i * 4 + 0];
            float xmin = boxes[i * 4 + 1];
            float ymax = boxes[i * 4 + 2];
            float xmax = boxes[i * 4 + 3];
            
            box->x = (int)(xmin * image->width);
            box->y = (int)(ymin * image->height);
            box->width = (int)((xmax - xmin) * image->width);
            box->height = (int)((ymax - ymin) * image->height);
            box->confidence = confidence;
            box->class_id = (int)classes[i];
            
            if (box->class_id >= 0 && box->class_id < 26) {
                strcpy(box->class_name, class_names[box->class_id]);
            } else {
                sprintf(box->class_name, "class_%d", box->class_id);
            }
            
            result->num_boxes++;
        }
    }
    
    uint64_t elapsed = rdtsc() - start_time;
    result->inference_time_us = elapsed / (cpu_frequency_hz() / 1000000);
    
    global_cv_engine.images_processed++;
    global_cv_engine.objects_detected += result->num_boxes;
    
    spinlock_release(&global_cv_engine.lock);
    
    kprintf("[CV] Detected %d objects in %llu us\n", 
            result->num_boxes, result->inference_time_us);
    
    return result;
}

// Simple OCR using template matching (for demonstration)
cv_ocr_result_t* cv_recognize_text(cv_image_t* image) {
    kprintf("[CV] Running OCR on image %dx%d...\n", image->width, image->height);
    
    uint64_t start_time = rdtsc();
    
    cv_ocr_result_t* result = kmalloc(sizeof(cv_ocr_result_t));
    memset(result, 0, sizeof(cv_ocr_result_t));
    
    // For demonstration, return placeholder text
    // Real implementation would use Tesseract or a neural OCR model
    
    strcpy(result->text, "Sample OCR text recognized from image");
    result->num_words = 6;
    
    const char* words[] = {"Sample", "OCR", "text", "recognized", "from", "image"};
    int x_pos = 10;
    
    for (int i = 0; i < 6; i++) {
        strcpy(result->words[i].text, words[i]);
        result->words[i].bbox.x = x_pos;
        result->words[i].bbox.y = 10;
        result->words[i].bbox.width = strlen(words[i]) * 10;
        result->words[i].bbox.height = 20;
        result->words[i].confidence = 0.92f;
        
        x_pos += result->words[i].bbox.width + 5;
    }
    
    uint64_t elapsed = rdtsc() - start_time;
    result->processing_time_us = elapsed / (cpu_frequency_hz() / 1000000);
    
    kprintf("[CV] OCR complete: '%s' (%llu us)\n", result->text, result->processing_time_us);
    
    return result;
}

// Draw bounding box on image
void cv_draw_box(cv_image_t* image, cv_bbox_t* box, uint8_t r, uint8_t g, uint8_t b) {
    // Draw top and bottom edges
    for (int x = box->x; x < box->x + box->width; x++) {
        if (x >= 0 && x < image->width) {
            // Top edge
            if (box->y >= 0 && box->y < image->height) {
                int idx = (box->y * image->stride) + (x * image->channels);
                image->data[idx + 0] = r;
                if (image->channels > 1) image->data[idx + 1] = g;
                if (image->channels > 2) image->data[idx + 2] = b;
            }
            
            // Bottom edge
            int bottom = box->y + box->height;
            if (bottom >= 0 && bottom < image->height) {
                int idx = (bottom * image->stride) + (x * image->channels);
                image->data[idx + 0] = r;
                if (image->channels > 1) image->data[idx + 1] = g;
                if (image->channels > 2) image->data[idx + 2] = b;
            }
        }
    }
    
    // Draw left and right edges
    for (int y = box->y; y < box->y + box->height; y++) {
        if (y >= 0 && y < image->height) {
            // Left edge
            if (box->x >= 0 && box->x < image->width) {
                int idx = (y * image->stride) + (box->x * image->channels);
                image->data[idx + 0] = r;
                if (image->channels > 1) image->data[idx + 1] = g;
                if (image->channels > 2) image->data[idx + 2] = b;
            }
            
            // Right edge
            int right = box->x + box->width;
            if (right >= 0 && right < image->width) {
                int idx = (y * image->stride) + (right * image->channels);
                image->data[idx + 0] = r;
                if (image->channels > 1) image->data[idx + 1] = g;
                if (image->channels > 2) image->data[idx + 2] = b;
            }
        }
    }
}