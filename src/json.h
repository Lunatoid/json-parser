//
// Do this:
//  #define JSON_IMPLEMENTATION
// before you include this file in *one* C++ file to create the implementation.
//
// Example:
//  #include ...
//  #include ...
//  #include ...
//  #define JSON_IMPLEMENTATION
//  #include "json.h"
//
// If you don't want to use malloc/realloc/free you can define #define JSON_MALLOC(size)/JSON_REALLOC(ptr, size)/JSON_FREE(ptr).
// Note that whenever the parser allocates anything on the heap that it will memset the allocated block to 0.
// If you have a custom allocator that returns zeroed memory, you can #define JSON_MEM_ALREADY_ZEROED to avoid memsetting it again.
//
// This parser follows the ECMA-404 standard.
// 
// License:
//  See end of file for license information
//
// API:
//  The JSON parser works with both char and wchar_t. It will use wchar_t by default, if you want to use single-byte characters instead
//  you can #define JSON_USE_SINGLE_BYTE
//  json_char is either defined as a char or wchar_t, depending on which to use.
//
//  The macro JSTR will append an L to your string if using multibyte and do nothing in single byte.
//  Example:
//    Multibyte:   JSTR("Hello world") -> L"Hello world"
//    Single byte: JSTR("Hello world") ->  "Hello world"
//
//  PARSING:
//   To parse JSON, you have the following options:
//     json_char* some_text = JSTR("[ 1, 2, 3, 4, 5, null ]");
//     JsonValue json = json_parse(some_text);
// 
//   Or if you want to load it from a file
//     JsonValue json = json_parse_from_file("config.json");
//
//   Note that json_parse_from_file() will heap-allocate a string that can hold the value and then call json_parse() on that and free it.
//
//  ACCESSING:
//   To access the various types that the JsonValue can hold, you can access them in various different ways.
//   There are 6 defined types in the implementation:
//     JSON_NULL
//     JSON_STRING -> JsonValue.string_value (json_char*)
//     JSON_NUMBER -> JsonValue.number_value (double)
//     JSON_OBJECT -> JsonValue.object_value (JsonObject*)
//     JSON_ARRAY  -> JsonValue.array_value  (JsonArray*)
//     JSON_BOOL   -> JsonValue.bool_value   (bool)
//
//   You can get the type of the JsonValue by doing JsonValue.type and comparing it with the enum listed above.
//   Note that all these values are in an union.
//
//   For arrays and objects you can use the overloaded [] operator:
//     JsonValue json = json_parse(JSTR("[ 0, 1, 2, 3 ]"));
//     
//     for (int i = 0; i < json.array_value->count; ++i) {
//       JsonValue& val = json[i];
//       // OR
//       JsonValue& val = json,array_value->values[i];
//       ...
//     }
//
//  Or with objects:
//     JsonValue json = json_parse(JSTR("{ \"age\": 41.9 }");
//
//     JsonValue& age = json[JSTR("age")];
//     // OR
//     JsonValue& age = json_find_field_ref(&json, JSTR("age"));
//
//  CREATING:
//   Creating new JSON values is quite easy.
//   You can call json_*type* to get a JsonValue of that type:
//      json_char* str = json_string(JSTR("Hello, world!"));
//      double num = json_number(10.04);
//
//   To add elements to an array you can use json_add_element():
//      JsonValue arr = json_array();
//      json_add_element(&arr, json_number(10.0));
//      json_add_element(&arr, json_bool(1));
//
//      JsonValue nested_arr = json_array();
//      json_add_element(&nested_arr, json_string(JSTR("wow")));
//
//      json_add_element(&arr, nested_arr);
//
//   If you want to remove an element, you can use json_remove_element(&arr, index)
//   Note that it will take care of resizing an array. It only grows.
//   If you add a heap-allocated JsonValue (array/object/string) to a JsonValue it will assume ownership of the pointer.
//   
//   DON'T DO THIS:
//      JsonValue str = json_string(JSTR("DON'T"));
//     
//      JsonValue arr1 = json_array();
//      json_add_element(&arr1, str);
//
//      JsonValue arr2 = json_array();
//      json_add_element(&arr2, str);
//
//   In the above example, both arrays have assumed ownership over the string.
//   If you were to free 1 and still use the other you'd have a free-after-use error. BEWARE!
//   Consider using json_duplicate() to make copies of a JsonValue.
//
//   Adding fields to objects is similar to arrays, you can use json_add_field():
//      JsonValue obj = json_object();
//      json_add_field(&obj, JSTR("key"), json_number(10.0));
//
//   Note that there it will not check if a certain field already exists as per the ECMA-404 standard.
//   More control over seeing if fields exist/removing fields/etc is still @TODO
//
//  EXPORTING:
//
//   To export a JsonValue all you have to do is call json_export():
//     JsonValue json = ...;
//     JSON_BOOL_TYPE minified = 1;
//     json_export(&json, "path/to/file.json", minified);
//
//   Stringifying a JsonValue is still @TODO
//
//   Customize your export:
//     #define JSON_INDENT_CHAR to change the character used for indenting. Space ' ' by default.
//     #define JSON_INDENT_STEP by how many characters it will indent. 2 by default.
//
//  SETTINGS:
//
//    These settings allow you to customize how the parser behaves.
//    Enabling these settings will make the parser no longer compliant with ECMA-404.
//
//    #define JSON_ALLOW_EXP_DECIMALS
//      Allow floating-point numbers in the number exponent e.g. 10e2.4
//

#ifndef JSON_H_
#define JSON_H_

#ifdef __cplusplus
extern "C" {
#endif
  
#include <stdint.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <wchar.h>
#include <malloc.h>
  
  // @TODO: cleanup
#ifndef JSON_USE_SINGLE_BYTE
#  define json_scanf swscanf
#  define json_printf wprintf
#  define json_strcmp wcscmp
#  define json_strcpy wcscpy
#  define json_strlen wcslen
#  define json_strncpy wcsncpy
#  define json_char wchar_t
#  define json_fgets fgetws
#  define json_fprintf fwprintf
#  define JSON_CHAR_MAX WCHAR_MAX
  // @TODO: check if this is required for Linux
#  define JSON_READ_MODE "r, ccs=UTF-8"
#  define JSON_WRITE_MODE "w, ccs=UTF-8"
#  define JSTR_CONCAT(a, b) a##b
#  define JSTR(str) JSTR_CONCAT(L,str)
#else
#  define json_scanf sscanf
#  define json_printf printf
#  define json_strcmp strcmp
#  define json_strcpy strcpy
#  define json_strlen strlen
#  define json_strncpy strncpy
#  define json_char char
#  define json_fgets fgets
#  define json_fprintf fprintf
#  define JSON_CHAR_MAX CHAR_MAX
#  define JSON_READ_MODE "r"
#  define JSON_WRITE_MODE "w"
#  define JSTR(str) str
#endif
  
#ifndef JSON_BOOL_TYPE
#  ifdef __cplusplus
#    define JSON_BOOL_TYPE bool
#  else
#    define JSON_BOOL_TYPE uint8_t
#  endif
#endif
  
  // Types within JSON
  typedef enum {
    JSON_NULL,
    JSON_STRING,
    JSON_NUMBER,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_BOOL
  } JsonType;
  
  // Datatypes
  typedef struct {
    struct _JsonValue* values;
    uint32_t count;
    uint32_t capacity;
  } JsonArray;
  
  typedef struct _JsonObject {
    json_char* key;
    struct _JsonValue* value;
    
    struct _JsonObject* next;
  } JsonObject;
  
  typedef struct _JsonValue {
    JsonType type;
    
    union {
      json_char* string_value;
      double number_value;
      JsonObject* object_value;
      JsonArray* array_value;
      JSON_BOOL_TYPE bool_value;
    };
    
  } JsonValue;
  
  // API
  JsonValue json_parse_from_file(const char* path);
  JsonValue json_parse(const json_char* json_text);
  
  JSON_BOOL_TYPE json_export(JsonValue* json, const char* path, JSON_BOOL_TYPE minified);
  
  JsonValue* json_find_field(JsonValue* json, const json_char* key);
  
#ifdef __cplusplus
  JsonValue& json_find_field_ref(JsonValue* json, const json_char* key);
#endif
  
  int json_field_count(JsonValue* json, const json_char* key);
  
  void json_free(JsonValue* json);
  JsonValue json_duplicate(JsonValue* json);
  
  inline JsonValue json_null();
  inline JsonValue json_number(double value);
  inline JsonValue json_string(const json_char* value);
  inline JsonValue json_string_char(json_char value);
  
  inline JsonValue json_object();
  inline void json_add_field(JsonValue* json, const json_char* key, JsonValue value);
  
  inline JsonValue json_array();
  inline void json_add_element(JsonValue* json, JsonValue value);
  inline void json_remove_element(JsonValue* json, int index);
  
  inline JsonValue json_bool(JSON_BOOL_TYPE value);
  
  // Overwritable #defines
#if defined(JSON_MALLOC) && defined(JSON_REALLOC) && defined(JSON_FREE)
  // Ok
#elif !defined(JSON_MALLOC) && !defined(JSON_REALLOC) && !defined(JSON_FREE)
  // Ok
#else
#  error "Must define all or none of JSON_MALLOC, JSON_REALLOC and JSON_FREE"
#endif
  
#ifndef JSON_MALLOC
#  include <malloc.h>
#  define JSON_MALLOC(size) malloc(size)
#  define JSON_REALLOC(ptr, size) realloc(ptr, size)
#  define JSON_FREE(ptr) free(ptr)
#endif
  
  
#ifndef JSON_INDENT_CHAR
#  define JSON_INDENT_CHAR ' '
#endif
  
#ifndef JSON_INDENT_STEP
#  define JSON_INDENT_STEP 2
#endif
  
#ifdef __cplusplus
}
#endif

#endif // JSON_H_

// Implementation starts here
#ifdef JSON_IMPLEMENTATION
#undef JSON_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif
  
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
  
  typedef struct JsonContext {
    const json_char* text;
    uint64_t len;
    uint64_t curr;
    
    json_char buffer[256];
    
    JSON_BOOL_TYPE is_parsing;
  } JsonContext;
  
  static void* json_alloc(uint32_t size) {
    void* ptr = (void*)JSON_MALLOC(size);
    
#ifndef JSON_MEM_ALREADY_ZEROED
    memset(ptr, 0, size);
#endif
    
    return ptr;
  }
  
  inline JsonValue json_null() {
    JsonValue json = {0};
    json.type = JSON_NULL;
    
    return json;
  }
  
  inline JsonValue json_number(double value) {
    JsonValue json  = {0};
    json.type = JSON_NUMBER;
    json.number_value = value;
    
    return json;
  }
  
  inline JsonValue json_string(const json_char* value) {
    JsonValue json  = {0};
    json.type = JSON_STRING;
    json.string_value = (json_char*)json_alloc((json_strlen(value) + 1) * sizeof(json_char));
    json_strcpy(json.string_value, value);
    
    return json;
  }
  
  inline JsonValue json_string_char(json_char value) {
    JsonValue json  = {0};
    json.type = JSON_STRING;
    json.string_value = (json_char*)json_alloc(2 * sizeof(json_char));
    json.string_value[0] = value;
    json.string_value[1] = JSTR('\0');
    
    return json;
  }
  
  static inline JsonObject* json_alloc_object(const json_char* key, JsonValue value) {
    JsonObject* object_value = (JsonObject*)json_alloc(sizeof(JsonObject)); 
    object_value->key = (json_char*)json_alloc((json_strlen(key) + 1) * sizeof(json_char));
    json_strcpy(object_value->key, key);
    
    object_value->value = (JsonValue*)json_alloc(sizeof(JsonValue));
    *object_value->value = value;
    
    return object_value;
  }
  
  inline JsonValue json_object() {
    JsonValue json  = {0};
    json.type = JSON_OBJECT;
    
    return json;
  }
  
  inline void json_add_field(JsonValue* json, const json_char* key, JsonValue value) {
    assert(json->type == JSON_OBJECT);
    
    if (!json->object_value) {
      json->object_value = json_alloc_object(key, value);
    } else {
      JsonObject* curr = json->object_value;
      for (;;) {
        if (curr->next) {
          curr = curr->next;
        } else {
          curr->next = json_alloc_object(key, value);
          break;
        }
      }
    }
  }
  
  inline JsonValue json_array() {
    JsonValue json  = {0};
    json.type = JSON_ARRAY;
    
    json.array_value = (JsonArray*)json_alloc(sizeof(JsonArray));
    
    return json;
  }
  
  inline void json_add_element(JsonValue* json, JsonValue value) {
    // @HARDCODED
    const int ARRAY_START_SIZE = 32;
    
    if (!json->array_value->values) {
      json->array_value->capacity = ARRAY_START_SIZE;
      json->array_value->values = (JsonValue*)json_alloc(sizeof(JsonValue) * json->array_value->capacity);
    } else if (json->array_value->count >= json->array_value->capacity) {
      // Double the capacity
      json->array_value->capacity *= 2;
      json->array_value->values = (JsonValue*)JSON_REALLOC(json->array_value->values,
                                                           sizeof(JsonValue) * json->array_value->capacity);
    }
    
    json->array_value->values[json->array_value->count++] = value;
  }
  
  inline void json_remove_element(JsonValue* json, int index) {
    if (index < 0 || index >= json->array_value->count || !json->array_value->values) return;
    
    // Free the value
    json_free(&json->array_value->values[index]);
    
    // If it's the last element, just decrease the count
    if (index == json->array_value->count - 1) {
      --json->array_value->count;
    } else {
      // Else, copy everything after it over it
      memcpy(json->array_value->values + index, json->array_value->values + (index + 1),
             (json->array_value->count-- - index) * sizeof(JsonValue));
    }
  }
  
  inline JsonValue json_bool(JSON_BOOL_TYPE value) {
    JsonValue json = {0};
    json.type = JSON_BOOL;
    json.bool_value = value;
    return json;
  }
  
  static void json_read(JsonContext* c, uint32_t count) {
    if (c->curr + count <= c->len) {
      json_strncpy(c->buffer, (c->text + c->curr), count);
      c->curr += count;
      c->buffer[count] = JSTR('\0');
    } else {
      c->buffer[0] = JSTR('\0');
      c->is_parsing = 0;
    }
  }
  
  static json_char json_get(JsonContext* c) {
    // Get next json_character, skip any whitespace
    do {
      json_read(c, 1);
    } while (isspace(c->buffer[0]));
    
    return c->buffer[0];
  }
  
  static json_char json_peek(JsonContext* c) {
    json_char peek = json_get(c);
    c->curr--;
    
    return peek;
  }
  
  static double json_parse_number(JsonContext* c) {
    uint64_t num_start = c->curr;
    
    // Consume negate
    if (json_peek(c) == JSTR('-')) {
      json_read(c, 1);
    }
    
    // Parse integer part
    JSON_BOOL_TYPE exponent_found = 0;
    int radix_count = 0;
    do {
      json_read(c, 1);
      
      if (c->buffer[0] == JSTR('.')) {
        ++radix_count;
        continue;
      }
      
      if (c->buffer[0] == JSTR('e') ||
          c->buffer[0] == JSTR('E')) {
        exponent_found = 1;
        break;
      }
    } while (isdigit(c->buffer[0]) ||
             (c->buffer[0] == JSTR('.') && radix_count == 1));
    
    uint64_t num_end = c->curr - 1;
    
    uint32_t num_length = num_end - num_start;
    c->curr = num_start;
    json_read(c, num_length);
    
    double num;
    json_scanf(c->buffer, JSTR("%lf"), &num);
    
    // Parse exponent
    if (exponent_found) {
      // Consume 'e' token
      json_read(c, 1);
      
      char sign = json_peek(c);
      
      JSON_BOOL_TYPE is_positive = 1;
      if (sign == '+') {
        json_read(c, 1);
        is_positive = 1;
      } else if (sign == '-') {
        is_positive = 0;
        json_read(c, 1);
      }
      
      num_start = c->curr;
      radix_count = 0;
      do {
        json_read(c, 1);
        
#ifdef JSON_ALLOW_EXP_DECIMALS
        if (c->buffer[0] == JSTR('.')) {
          ++radix_count;
          continue;
        }
#else
        // If we don't allow it (which is by default)
        // break when we see one, so we don't reach the
        // while statement's check and pass it as valid.
        if (c->buffer[0] == JSTR('.')) {
          break;
        }
#endif
      } while (isdigit(c->buffer[0]) ||
               (c->buffer[0] == JSTR('.') && radix_count == 1));
      
      num_end = c->curr;
      
      num_length = num_end - num_start;
      
      c->curr = num_start;
      json_read(c, num_length);
      
      double exp_num;
      json_scanf(c->buffer, JSTR("%lf"), &exp_num);
      
      if (!is_positive) {
        exp_num *= -1.0;
      }
      num *= pow(10.0, exp_num);
    }
    
    return num;
  }
  
  static json_char* json_parse_string(JsonContext* c) {
    // Consume starting quote
    json_read(c, 1);
    
    // Remember file position
    uint64_t string_start = c->curr;
    
    // Calculate string length
    uint32_t string_length = 0;
    do {
      json_read(c, 1);
      
      if (c->buffer[0] == JSTR('\\')) {
        json_read(c, 1);
        
        switch (c->buffer[0]) {
          case JSTR('"'):
          case JSTR('\\'):
          case JSTR('/'):
          case JSTR('b'):
          case JSTR('f'):
          case JSTR('n'):
          case JSTR('r'):
          case JSTR('t'): 
          case JSTR('u'): {
            // Only 1 json_character
            ++string_length;
            break;
          }
          
          default: {
            string_length += 2;
          }
        }
      } else {
        if (c->buffer[0] == JSTR('"')) break;
        ++string_length;
      }
    } while (c->is_parsing);
    
    json_char* str = (json_char*)json_alloc((string_length + 1) * sizeof(json_char));
    c->curr = string_start;
    
    // Calculate string length
    int curr = 0;
    do {
      json_read(c, 1);
      
      if (c->buffer[0] == JSTR('\\')) {
        json_read(c, 1);
        
        switch (c->buffer[0]) {
          case JSTR('"'):  str[curr++] = JSTR('\"'); break;
          case JSTR('\\'): str[curr++] = JSTR('\\'); break;
          case JSTR('/'):  str[curr++] = JSTR('/'); break;
          case JSTR('b'):  str[curr++] = JSTR('\b'); break;
          case JSTR('f'):  str[curr++] = JSTR('\f'); break;
          case JSTR('n'):  str[curr++] = JSTR('\n'); break;
          case JSTR('r'):  str[curr++] = JSTR('\r'); break;
          case JSTR('t'):  str[curr++] = JSTR('\t'); break;
          
          case JSTR('u'): {
            // Read 4 hex digits
            json_read(c, 4);
            
            int valid = 0;
            for (int i = 0; i < 4; ++i) {
              json_char h = c->buffer[i];
              valid += ((h >= JSTR('A') && h <= JSTR('F')) ||
                        (h >= JSTR('a') && h <= JSTR('f')) ||
                        isdigit(h));
            }
            
            if (valid == 4) {
              int hex_num;
              json_scanf(c->buffer, JSTR("%x"), &hex_num);
              
              if (hex_num > JSON_CHAR_MAX) {
                json_printf(JSTR("Warning: unicode codepoint \\u%s doesn't fit\n"), c->buffer);
                json_printf(JSTR("    Decimal value: %d\n    Max value: %d\n"), hex_num, JSON_CHAR_MAX);
                str[curr++] = JSTR('?');
              } else {
                str[curr++] = (json_char)hex_num;
              }
            } else {
              for (int i = 0; i < 5; ++i) {
                str[curr++] = c->buffer[i];
              }
            }
            break;
          }
          
          default: {
            str[curr++] = c->buffer[0];
            str[curr++] = c->buffer[1];
          }
        }
      } else {
        if (c->buffer[0] == JSTR('"')) break;
        
        str[curr++] = c->buffer[0];
      }
    } while (c->is_parsing);
    
    return str;
  }
  
  static void json_parse_value(JsonContext* c, JsonValue* value);
  
  static JsonArray* json_parse_array(JsonContext* c) {
    // Consume tarting bracket
    json_read(c, 1);
    
    // @HARDCODED
    const int ARRAY_START_SIZE = 32;
    
    JsonArray* arr = (JsonArray*)json_alloc(sizeof(JsonArray));
    if (json_peek(c) != JSTR(']')) {
      // Initial value
      arr->capacity = ARRAY_START_SIZE;
      arr->values = (JsonValue*)json_alloc(sizeof(JsonValue) * arr->capacity);
      
      while (1) {
        if (arr->count >= arr->capacity) {
          // Double the capacity
          arr->capacity *= 2;
          arr->values = (JsonValue*)JSON_REALLOC(arr->values, sizeof(JsonValue) * arr->capacity);
        }
        
        json_parse_value(c, &arr->values[arr->count++]);
        
        json_char next = json_get(c);
        
        if (next == JSTR(']')) break;
        if (next != JSTR(',')) {
          json_printf(JSTR("Unknown token in array '%c'\n"), next);
          break;
        }
      }
    } else {
      // Consume closing bracket
      json_read(c, 1);
    }
    
    return arr;
  }
  
  static JSON_BOOL_TYPE json_parse_field(JsonContext* c, JsonObject* curr) {
    JsonValue tmp  = {0};
    json_parse_value(c, &tmp);
    
    if (tmp.type != JSON_STRING) {
      json_printf(JSTR("Object field must be string\n"));
      json_free(&tmp);
      return 0;
    }
    
    curr->key = tmp.string_value;
    
    if (json_get(c) != JSTR(':')) {
      json_printf(JSTR("Expected ':' in object\n"));
      return 0;
    }
    
    curr->value = (JsonValue*)json_alloc(sizeof(JsonValue));
    json_parse_value(c, curr->value);
    return 1;
  }
  
  static JsonObject* json_parse_object(JsonContext* c) {
    // Consume starting brace
    json_read(c, 1);
    
    JsonObject* head = (JsonObject*)json_alloc(sizeof(JsonObject));
    JsonObject* prev = head;
    
    if (json_peek(c) != JSTR('}')) {
      json_parse_field(c, head);
      
      json_char next = json_get(c);
      if (next == JSTR('}') || next != JSTR(',')) {
        if (next != JSTR('}') && next != JSTR(',')) {
          json_printf(JSTR("Unknown token in object '%c'\n"), next);
        }
        return head;
      }
      
      while (1) {
        JsonObject* curr = (JsonObject*)json_alloc(sizeof(JsonObject));
        
        if (!json_parse_field(c, curr)) break;
        
        prev->next = curr;
        prev = curr;
        
        json_char next = json_get(c);
        if (next == JSTR('}')) break;
        if (next != JSTR(',')) {
          json_printf(JSTR("Unknown token in object '%c'\n"), next);
          break;
        }
      }
    } else {
      // Consume closing bracket
      json_get(c);
    }
    
    return head;
  }
  
  static void json_parse_value(JsonContext* c, JsonValue* value) {
    json_char peek = json_peek(c);
    
    switch (peek) {
      case JSTR('{'): {
        value->type = JSON_OBJECT;
        value->object_value = json_parse_object(c);
        break;
      }
      
      case JSTR('['): {
        value->type = JSON_ARRAY;
        value->array_value = json_parse_array(c);
        break;
      }
      
      case JSTR('"'): {
        value->type = JSON_STRING;
        value->string_value = json_parse_string(c);
        break;
      }
      
      default: {
        if (c->buffer[0] == JSTR('-') ||
            isdigit(c->buffer[0])) {
          value->type = JSON_NUMBER;
          value->number_value = json_parse_number(c);
        } else {
          uint64_t word_start = c->curr;
          uint32_t word_length = 0;
          
          do {
            json_read(c, 1);
            if (c->buffer[0] >= JSTR('a') && c->buffer[0] <= JSTR('z')) {
              ++word_length;
            } else {
              break;
            }
          } while (1);
          
          c->curr = word_start;
          json_read(c, word_length);
          
          if (json_strcmp(c->buffer, JSTR("1")) == 0) {
            value->type = JSON_BOOL;
            value->bool_value = 1;
          } else if (json_strcmp(c->buffer, JSTR("0")) == 0) {
            value->type = JSON_BOOL;
            value->bool_value = 0;
          } else if (json_strcmp(c->buffer, JSTR("null")) == 0) {
            value->type = JSON_NULL;
          } else {
            json_printf(JSTR("Unknown token '%s'\n"), c->buffer);
            c->is_parsing = 0;
          }
        }
      }
    }
  }
  
  JsonValue json_parse_from_file(const char* path) {
    FILE* file = fopen(path, JSON_READ_MODE);
    
    if (!file) {
      printf("Could not open file '%s'\n", path);
      
      JsonValue dummy  = {0};
      return dummy;
    }
    
    fseek(file, 0, SEEK_END);
    uint64_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    json_char* json_text = (json_char*)json_alloc((size + 1) * sizeof(json_char));
    
    json_char* cursor = json_text;
    while (!feof(file)) {
      json_fgets(cursor, size, file);
      cursor = json_text + json_strlen(json_text);
    }
    
    fclose(file);
    
    JsonValue value = json_parse(json_text);
    
    JSON_FREE(json_text);
    
    return value;
  }
  
  JsonValue json_parse(const json_char* json_text) {
    JsonContext c  = {0};
    c.is_parsing = 1;
    
    // Ignore byte order mark if present
    const int BOM_CHAR = 65279;
    if (JSON_CHAR_MAX >= BOM_CHAR && json_text[0] == BOM_CHAR) {
      ++json_text;
    }
    
    c.text = json_text;
    c.len = json_strlen(c.text);
    
    JsonValue value  = {0};
    json_parse_value(&c, &value);
    
    return value;
  }
  
  static void json_print_indent(FILE* file, JSON_BOOL_TYPE minified, int indent_level) {
    if (!minified) {
      for (int i = 0; i < indent_level; ++i) {
        json_fprintf(file, JSTR("%c"), JSON_INDENT_CHAR);
      }
    }
  }
  
  static void json_print_newline(FILE* file, JSON_BOOL_TYPE minified) {
    if (!minified) {
      json_fprintf(file, JSTR("\n"));
    }
  }
  
  static void json_export_value(JsonValue* value, FILE* file, int indent_level, JSON_BOOL_TYPE minified) {
    
    switch (value->type) {
      case JSON_NULL: {
        json_fprintf(file, JSTR("null"));
        break;
      }
      
      case JSON_STRING: {
        json_fprintf(file, JSTR("\"%s\""), value->string_value);
        break;
      }
      
      case JSON_NUMBER: {
        // Check if number has a decimal place
        if (fmod(value->number_value, 1.0) == 0.0) {
          if (value->number_value > 0.0) {
            json_fprintf(file, JSTR("%" PRIu64), (uint64_t)value->number_value);
          } else {
            json_fprintf(file, JSTR("%" PRId64), (int64_t)value->number_value);
          }
        } else {
          json_fprintf(file, JSTR("%.6g"), value->number_value);
        }
        break;
      }
      
      case JSON_OBJECT: {
        json_fprintf(file, JSTR("{"));
        json_print_newline(file, minified);
        
        if (value->object_value) {
          for (JsonObject* obj = value->object_value; obj != NULL; obj = obj->next) {
            json_print_indent(file, minified, indent_level);
            json_fprintf(file, JSTR("\"%s\": "), obj->key);
            json_export_value(obj->value, file, indent_level + JSON_INDENT_STEP, minified);
            
            if (obj->next) {
              json_fprintf(file, JSTR(","));
              json_print_newline(file, minified);
            } else {
              json_print_newline(file, minified);
            }
          }
        }
        indent_level -= JSON_INDENT_STEP;
        json_print_indent(file, minified, indent_level);
        json_fprintf(file, JSTR("}"));
        break;
      }
      
      case JSON_ARRAY: {
        json_fprintf(file, JSTR("["));
        json_print_newline(file, minified);
        
        for (int i = 0; i < value->array_value->count; ++i) {
          json_print_indent(file, minified, indent_level);
          json_export_value(&value->array_value->values[i], file, indent_level + JSON_INDENT_STEP, minified);
          
          if (i + 1 < value->array_value->count) {
            json_fprintf(file, JSTR(","));
            json_print_newline(file, minified);
          } else {
            json_print_newline(file, minified);
          }
        }
        indent_level -= JSON_INDENT_STEP;
        json_print_indent(file, minified, indent_level);
        json_fprintf(file, JSTR("]"));
        break;
      }
      
      case JSON_BOOL: {
        json_fprintf(file, JSTR("%s"), (value->bool_value) ? JSTR("1") : JSTR("0"));
        break;
      }
    }
  }
  
  JSON_BOOL_TYPE json_export(JsonValue* json, const char* path, JSON_BOOL_TYPE minified) {
    if (!json || !path) return 0;
    
    FILE* file = fopen(path, JSON_WRITE_MODE);
    
    if (!file) {
      printf("Could not create file for writing '%s'\n", path);
      return 0;
    }
    
    json_export_value(json, file, JSON_INDENT_STEP, minified);
    
    fflush(file);
    fclose(file);
    
    return 1;
  }
  
  JsonValue* json_get_field(JsonValue* json, const json_char* key) {
    if (!json || !key || json->type != JSON_OBJECT) return NULL;
    
    for (JsonObject* v = json->object_value; v != NULL; v = v->next) {
      if (json_strcmp(key, v->key) == 0) {
        return v->value;
      }
    }
    
    return NULL;
  }
  
#ifdef __cplusplus
  
  JsonValue& json_get_field_ref(JsonValue* json, const json_char* key) {
    JsonValue null = json_null();
    JsonValue& val = null;
    
    for (JsonObject* v = json->object_value; v != NULL; v = v->next) {
      if (json_strcmp(key, v->key) == 0) {
        val = *v->value;
        break;
      }
    }
    
    return val;
  }
  
#endif
  
  int json_field_count(JsonValue* json, const json_char* key) {
    int count = 0;
    
    for (JsonObject* v = json->object_value; v != NULL; v = v->next) {
      if (json_strcmp(key, v->key) == 0) {
        ++count;
      }
    }
    
    return count;
  }
  
  void json_free(JsonValue* json) {
    if (!json) return;
    
    switch (json->type) {
      case JSON_STRING: {
        JSON_FREE(json->string_value);
        break;
      }
      
      case JSON_OBJECT: {
        // Get latest node
        
        JsonObject* head = json->object_value;
        JsonObject* tmp;
        
        while (head) {
          tmp = head;
          head = head->next;
          
          JSON_FREE(tmp->key);
          json_free(tmp->value);
          JSON_FREE(tmp->value);
          JSON_FREE(tmp);
        }
        
        break;
      }
      
      case JSON_ARRAY: {
        for (int i = 0; i < json->array_value->count; ++i) {
          json_free(&json->array_value->values[i]);
        }
        JSON_FREE(json->array_value->values);
        JSON_FREE(json->array_value);
        break;
      }
    }
    
    *json = json_null();
  }
  
  JsonValue json_duplicate(JsonValue* json) {
    if (!json) return json_null();
    
    switch (json->type) {
      case JSON_NULL: {
        return json_null();
      }
      
      case JSON_STRING: {
        return json_string(json->string_value);
      }
      
      case JSON_NUMBER: {
        return json_number(json->number_value);
      }
      
      case JSON_OBJECT: {
        JsonValue dup = json_object();
        
        for (JsonObject* obj = json->object_value; obj != NULL; obj = obj->next) {
          json_add_field(&dup, obj->key, json_duplicate(obj->value));
        }
        
        return dup;
      }
      
      case JSON_ARRAY: {
        JsonValue dup = json_array();
        
        for (int i = 0; i < json->array_value->count; ++i) {
          json_add_element(&dup, json_duplicate(&json->array_value->values[i]));
        }
        
        return dup;
      }
      
      case JSON_BOOL: {
        return json_bool(json->bool_value);
      }
    }
    
    return json_null();
  }
  
#ifdef __cplusplus
}
#endif

#endif // JSON_IMPLEMENTATION

/*

zlib License

(C) Copyright 2018 Tom Mol

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

*/
