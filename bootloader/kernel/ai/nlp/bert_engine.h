/**
 * AION OS - Real BERT NLP Engine
 * Using actual BERT model for natural language understanding
 */

 #ifndef BERT_ENGINE_H
 #define BERT_ENGINE_H
 
 #include "../ml/tflite_real.h"
 #include <stdint.h>
 
 // BERT Configuration
 #define BERT_MAX_SEQ_LENGTH 512
 #define BERT_VOCAB_SIZE 30522
 #define BERT_EMBEDDING_DIM 768
 
 // Token IDs
 #define BERT_TOKEN_PAD 0
 #define BERT_TOKEN_UNK 100
 #define BERT_TOKEN_CLS 101
 #define BERT_TOKEN_SEP 102
 #define BERT_TOKEN_MASK 103
 
 // Intent Classification
 typedef enum {
     INTENT_UNKNOWN,
     INTENT_FILE_OPERATION,    // "open file", "delete file"
     INTENT_PROCESS_CONTROL,   // "kill process", "start app"
     INTENT_SYSTEM_QUERY,      // "show memory", "cpu usage"
     INTENT_CODE_ASSISTANCE,   // "complete this code", "find bug"
     INTENT_SEARCH,            // "find", "search for"
     INTENT_HELP,              // "how to", "help"
     INTENT_MAX
 } intent_type_t;
 
 // NLP Result
 typedef struct {
     intent_type_t intent;
     float confidence;
     char entities[256];  // Extracted entities
     char normalized_text[512];
 } nlp_result_t;
 
 // BERT Engine Context
 typedef struct {
     aion_tflite_t tflite;
     
     // Tokenizer (WordPiece)
     char** vocab;
     uint32_t vocab_size;
     
     // Model paths
     char bert_model_path[256];
     char vocab_path[256];
     
     // Embeddings cache
     float* embedding_cache;
     
     bool initialized;
 } bert_engine_t;
 
 // Initialize BERT engine
 int bert_init(bert_engine_t* bert, const char* model_path, const char* vocab_path);
 
 // Tokenize text
 int32_t* bert_tokenize(bert_engine_t* bert, const char* text, uint32_t* num_tokens);
 
 // Get text embedding
 float* bert_encode(bert_engine_t* bert, const char* text, uint32_t* embedding_size);
 
 // Classify intent
 nlp_result_t bert_classify_intent(bert_engine_t* bert, const char* text);
 
 // Question answering
 char* bert_answer_question(bert_engine_t* bert, const char* context, const char* question);
 
 // Text similarity
 float bert_similarity(bert_engine_t* bert, const char* text1, const char* text2);
 
 // Named entity recognition
 char** bert_extract_entities(bert_engine_t* bert, const char* text, uint32_t* num_entities);
 
 // Cleanup
 void bert_cleanup(bert_engine_t* bert);
 
 #endif // BERT_ENGINE_H