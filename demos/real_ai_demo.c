/**
 * AION OS - REAL AI Integration Demo
 * Using actual TensorFlow Lite models
 */

 #include "../kernel/ai/ml/tflite_real.h"
 #include "../kernel/ai/nlp/bert_engine.h"
 #include "../userland/ai-ide/code_completion_real.h"
 #include <stdio.h>
 #include <stdlib.h>
 
 void demo_real_bert_nlp() {
     printf("\n╔══════════════════════════════════════╗\n");
     printf("║   REAL BERT NLP ENGINE DEMO          ║\n");
     printf("╚══════════════════════════════════════╝\n\n");
     
     bert_engine_t bert;
     
     // Initialize with real BERT model
     if (bert_init(&bert, "models/mobilebert.tflite", "models/vocab.txt") != 0) {
         printf("❌ Failed to initialize BERT (model file missing?)\n");
         printf("Run: make setup-models\n");
         return;
     }
     
     // Test intent classification
     const char* commands[] = {
         "open the file /etc/config",
         "show me the CPU usage",
         "kill process nginx",
         "help me debug this code",
         "find all files containing 'error'"
     };
     
     printf("Intent Classification:\n");
     printf("─────────────────────\n");
     
     for (int i = 0; i < 5; i++) {
         nlp_result_t result = bert_classify_intent(&bert, commands[i]);
         
         printf("\n📝 Command: \"%s\"\n", commands[i]);
         printf("   Intent: %d\n", result.intent);
         printf("   Confidence: %.1f%%\n", result.confidence * 100);
     }
     
     // Test text similarity
     printf("\n\nText Similarity:\n");
     printf("────────────────\n");
     
     const char* text1 = "The system is running out of memory";
     const char* text2 = "RAM usage is too high";
     const char* text3 = "The weather is nice today";
     
     float sim1 = bert_similarity(&bert, text1, text2);
     float sim2 = bert_similarity(&bert, text1, text3);
     
     printf("Text 1: \"%s\"\n", text1);
     printf("Text 2: \"%s\"\n", text2);
     printf("Similarity: %.2f%%\n\n", sim1 * 100);
     
     printf("Text 1: \"%s\"\n", text1);
     printf("Text 3: \"%s\"\n", text3);
     printf("Similarity: %.2f%%\n", sim2 * 100);
     
     bert_cleanup(&bert);
 }
 
 void demo_real_code_completion() {
     printf("\n╔══════════════════════════════════════╗\n");
     printf("║   REAL CODE COMPLETION DEMO          ║\n");
     printf("╚══════════════════════════════════════╝\n\n");
     
     code_completion_t cc;
     
     // Initialize with CodeGen/GPT-2 model
     if (code_completion_init(&cc, "models/codegen.tflite") != 0) {
         printf("❌ Failed to initialize code completion model\n");
         printf("Run: make setup-models\n");
         return;
     }
     
     // Test code completion
     const char* code_prefix = 
         "int binary_search(int* arr, int n, int target) {\n"
         "    int left = 0, right = n - 1;\n"
         "    while (left <= right) {\n"
         "        ";
     
     printf("Code prefix:\n");
     printf("────────────\n%s\n\n", code_prefix);
     
     printf("Generating completions...\n\n");
     
     completion_result_t* completions = code_completion_generate(&cc, code_prefix, 3);
     
     if (completions) {
         for (int i = 0; i < 3; i++) {
             printf("Completion %d (score: %.1f):\n", i + 1, completions[i].score);
             printf("──────────────────────────────\n");
             printf("%s\n\n", completions[i].code);
         }
         free(completions);
     }
     
     // Test function generation
     printf("\nFunction Generation:\n");
     printf("────────────────────\n");
     
     const char* signature = "char* reverse_string(const char* str)";
     const char* docstring = "// Reverses a string in-place";
     
     char* function = code_completion_generate_function(&cc, signature, docstring);
     if (function) {
         printf("%s\n", function);
         free(function);
     }
     
     code_completion_cleanup(&cc);
 }
 
 void demo_tflite_raw() {
     printf("\n╔══════════════════════════════════════╗\n");
     printf("║   RAW TENSORFLOW LITE DEMO           ║\n");
     printf("╚══════════════════════════════════════╝\n\n");
     
     aion_tflite_t tflite;
     
     // Initialize
     if (aion_tflite_init(&tflite, 4) != 0) {
         printf("❌ Failed to initialize TFLite\n");
         return;
     }
     
     printf("✅ TensorFlow Lite runtime initialized\n");
     printf("   Threads: %u\n", tflite.num_threads);
     
     // Load a simple model (example: MobileNet for image classification)
     if (aion_tflite_load_model_file(&tflite, "models/mobilenet_v1.tflite") != 0) {
         printf("❌ Model not found (this is expected if not downloaded)\n");
         printf("   To test with real model, run: make setup-models\n");
         aion_tflite_cleanup(&tflite);
         return;
     }
     
     printf("✅ Model loaded\n");
     
     // Allocate tensors
     if (aion_tflite_allocate_tensors(&tflite) != 0) {
         printf("❌ Failed to allocate tensors\n");
         aion_tflite_cleanup(&tflite);
         return;
     }
     
     printf("✅ Tensors allocated\n");
     
     // Get input tensor info
     TfLiteTensor* input = aion_tflite_get_input_tensor(&tflite, 0);
     if (input) {
         printf("   Input tensor:\n");
         printf("      Type: %d\n", input->type);
         printf("      Dims: %d\n", input->dims->size);
         for (int i = 0; i < input->dims->size; i++) {
             printf("        [%d]: %d\n", i, input->dims->data[i]);
         }
     }
     
     // Dummy inference (would use real image data)
     printf("\n🚀 Running inference...\n");
     
     if (aion_tflite_invoke(&tflite) == 0) {
         printf("✅ Inference successful!\n");
         
         // Get output
         const TfLiteTensor* output = aion_tflite_get_output_tensor(&tflite, 0);
         if (output) {
             printf("   Output tensor size: %d\n", output->dims->data[0]);
         }
     } else {
         printf("❌ Inference failed\n");
     }
     
     aion_tflite_cleanup(&tflite);
 }
 
 int main() {
     printf("╔════════════════════════════════════════════════════╗\n");
     printf("║   AION OS - REAL AI INTEGRATION DEMONSTRATION     ║\n");
     printf("║   Using actual TensorFlow Lite models             ║\n");
     printf("╚════════════════════════════════════════════════════╝\n");
     
     printf("\n⚠️  NOTE: This demo requires real TensorFlow Lite models\n");
     printf("    Run these commands first:\n");
     printf("      make setup-tflite    # Install TFLite library\n");
     printf("      make setup-models    # Download AI models\n");
     printf("\n");
     
     // Test raw TFLite
     demo_tflite_raw();
     
     // Test BERT NLP
     demo_real_bert_nlp();
     
     // Test code completion
     demo_real_code_completion();
     
     printf("\n╔════════════════════════════════════════════════════╗\n");
     printf("║                 DEMO COMPLETE                      ║\n");
     printf("╚════════════════════════════════════════════════════╝\n");
     
     return 0;
 }