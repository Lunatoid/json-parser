#ifndef JSON_H_
#define JSON_H_

#include <stdint.h>
#include <memory.h>
#include <cstdio>
#include <cstdarg>
#include <cassert>
#include <cwchar>

#ifndef JSON_MALLOC
#  include <malloc.h>
#  define JSON_MALLOC(size) malloc(size)
#endif

#ifndef JSON_REALLOC
#  include <malloc.h>
#  define JSON_REALLOC(ptr, size) realloc(ptr, size)
#endif

#ifndef JSON_FREE
#  include <malloc.h>
#  define JSON_FREE(ptr) free(ptr)
#endif

#ifndef JSON_INDENT_CHAR
#  define JSON_INDENT_CHAR ' '
#endif

#ifndef JSON_INDENT_STEP
#  define JSON_INDENT_STEP 2
#endif

#ifndef JSON_USE_SINGLE_BYTE
#  define json_scanf swscanf
#  define json_printf wprintf
#  define json_strcmp wcscmp
#  define json_strcpy wcscpy
#  define json_strlen wcslen
#  define json_char wchar_t
#  define json_fgets fgetws
#  define json_fprintf fwprintf
#  define JSON_CHAR_MAX WCHAR_MAX
#  define JSON_READ_MODE "r, ccs=UTF-8"
#  define JSON_WRITE_MODE "w, ccs=UTF-8"
#  define JSTR_CONCAT(a, b) a##b
#  define JSTR(str) JSTR_CONCAT(L,str)
#else
#  define json_scanf sscanf
#  define json_printf printf
#  define json_strcmp strcmp
#  define json_strcpy strcpy
#  define json_strcpy strlen
#  define json_char char
#  define json_fgets fgets
#  define json_fprintf fprintf
#  define JSON_CHAR_MAX CHAR_MAX
#  define JSON_READ_MODE "r"
#  define JSON_WRITE_MODE "w"
#  define JSTR(str) str
#endif

// How to use:
//   #define JSON_IMPLEMENTATION in ONE translation unit

// Allocated memory MUST be memsetted to zero.
// It will do this automatically.
// If you have some custom allocator set up that already gives
// out zeroed memory, you can define JSON_ALREADY_ZEROED to avoid memsetting it twice

// Datatypes
enum JsonType {
  JSON_NULL,
  JSON_STRING,
  JSON_NUMBER,
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_BOOL
};

struct JsonValue;

JsonValue json_parse(const char* path);
bool json_export(JsonValue* json, const char* path, bool minified = false);
JsonValue* json_find_field(JsonValue* json, const json_char* key);
JsonValue& json_find_field_ref(JsonValue* json, const json_char* key);
void json_free(JsonValue* json);

struct JsonArray {
  JsonValue* values;
  uint32_t count;
  uint32_t capacity;
};

struct JsonObject {
  json_char* key;
  JsonValue* value;
  
  JsonObject* next;
};

struct JsonValue {
  JsonType type;
  
  union {
    json_char* string_value;
    double number_value;
    JsonObject* object_value;
    JsonArray* array_value;
    bool bool_value;
  };
  
  JsonValue& operator[](const json_char* key) {
    return json_find_field_ref(this, key);
  }
  
  JsonValue& operator[](int index) {
    assert(this->type == JSON_ARRAY);
    return this->array_value->values[index];
  }
};

inline JsonValue json_null();
inline JsonValue json_number(double value);
inline JsonValue json_string(const json_char* value);
inline JsonValue json_object();
inline void json_add_field(JsonValue* json, const json_char* key, JsonValue value);
inline JsonValue json_array(int capacity = 0);
inline void json_add_value(JsonValue* json, JsonValue value);
inline JsonValue json_bool(bool value);

#endif // JSON_H_

#ifdef JSON_IMPLEMENTATION
#undef JSON_IMPLEMENTATION

#include <ctype.h>
#include <string.h>
#include <cstdlib>
#include <math.h>

struct JsonContext {
  FILE* file;
  json_char buffer[256];
  
  bool is_parsing;
};

static void* json_alloc(uint32_t size) {
  void* ptr = (void*)JSON_MALLOC(size);
  
#ifndef JSON_ALREADY_ZEROED
  memset(ptr, 0, size);
#endif
  
  return ptr;
}

inline JsonValue json_null() {
  JsonValue json = {};
  json.type = JSON_NULL;
  
  return json;
}

inline JsonValue json_number(double value) {
  JsonValue json = {};
  json.type = JSON_NUMBER;
  json.number_value = value;
  
  return json;
}

inline JsonValue json_string(const json_char* value) {
  JsonValue json = {};
  json.type = JSON_STRING;
  json.string_value = (json_char*)json_alloc((json_strlen(value) + 1) * sizeof(json_char));
  json_strcpy(json.string_value, value);
  
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
  JsonValue json = {};
  json.type = JSON_OBJECT;
  
  return json;
}

inline void json_add_field(JsonValue* json, const json_char* key, JsonValue value) {
  assert(json->type == JSON_OBJECT);
  
  if (!json->object_value) {
    json->object_value = json_alloc_object(key, value);
  } else {
    JsonObject* curr = json->object_value;
    while (true) {
      if (curr->next) {
        curr = curr->next;
      } else {
        curr->next = json_alloc_object(key, value);
        break;
      }
    }
  }
}

inline JsonValue json_array(int capacity) {
  JsonValue json = {};
  json.type = JSON_ARRAY;
  
  json.array_value = (JsonArray*)json_alloc(sizeof(JsonArray));
  if (capacity > 0) {
    json.array_value->capacity = capacity;
    json.array_value->values = (JsonValue*)json_alloc(sizeof(JsonValue) * json.array_value->capacity);
  }
  
  return json;
}

inline void json_add_value(JsonValue* json, JsonValue value) {
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

inline JsonValue json_bool(bool value) {
  JsonValue json = {};
  json.type = JSON_BOOL;
  json.bool_value = value;
  return json;
}

static void json_read(JsonContext* c, uint32_t count) {
  if (!feof(c->file) && json_fgets(c->buffer, count + 1, c->file)) {
    c->buffer[count] = JSTR('\0');
  } else {
    c->buffer[0] = JSTR('\0');
    c->is_parsing = false;
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
  fseek(c->file, -1, SEEK_CUR);
  
  return peek;
}

static double json_parse_number(JsonContext* c) {
  uint64_t num_start = ftell(c->file);
  
  // Consume negate
  if (json_peek(c) == JSTR('-')) {
    json_read(c, 1);
  }
  
  // Parse integer part
  bool exponent_found = false;
  int radix_count = 0;
  do {
    json_read(c, 1);
    
    if (c->buffer[0] == JSTR('.')) {
      ++radix_count;
      continue;
    }
    
    if (c->buffer[0] == JSTR('e') ||
        c->buffer[0] == JSTR('E')) {
      exponent_found = true;
      break;
    }
  } while (isdigit(c->buffer[0]) ||
           (c->buffer[0] == JSTR('.') && radix_count == 1));
  
  uint64_t num_end = ftell(c->file) - 1;
  
  uint32_t num_length = num_end - num_start;
  fseek(c->file, num_start, SEEK_SET);
  json_read(c, num_length);
  
  double num;
  json_scanf(c->buffer, JSTR("%lf"), &num);
  
  // Parse exponent
  if (exponent_found) {
    // Consume 'e' token
    json_read(c, 1);
    
    num_start = ftell(c->file);
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
    
    num_end = ftell(c->file) - 1;
    
    num_length = num_end - num_start;
    
    fseek(c->file, num_start, SEEK_SET);
    json_read(c, num_length);
    
    double exp_num;
    json_scanf(c->buffer, JSTR("%lf"), &exp_num);
    num *= pow(10.0, exp_num);
  }
  
  return num;
}

static json_char* json_parse_string(JsonContext* c) {
  // Consume starting quote
  json_read(c, 1);
  
  // Remember file position
  uint64_t string_start = ftell(c->file);
  
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
  fseek(c->file, string_start, SEEK_SET);
  
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
              str[curr++] = json_char(hex_num);
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
  // Consume s tarting bracket
  json_read(c, 1);
  
  const int ARRAY_START_SIZE = 32;
  
  JsonArray* arr = (JsonArray*)json_alloc(sizeof(JsonArray));
  if (json_peek(c) != JSTR(']')) {
    // Initial value
    arr->capacity = ARRAY_START_SIZE;
    arr->values = (JsonValue*)json_alloc(sizeof(JsonValue) * arr->capacity);
    
    while (true) {
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
  }
  
  return arr;
}

static JsonObject* json_parse_object(JsonContext* c) {
  // Consume starting brace
  json_read(c, 1);
  
  JsonObject* head = (JsonObject*)json_alloc(sizeof(JsonObject));
  JsonObject* prev = head;
  
  auto parse_field = [c](JsonObject* curr) -> bool {
    
    JsonValue tmp = {};
    json_parse_value(c, &tmp);
    
    if (tmp.type != JSON_STRING) {
      json_printf(JSTR("Object field must be string\n"));
      json_free(&tmp);
      return false;
    }
    
    curr->key = tmp.string_value;
    
    if (json_get(c) != JSTR(':')) {
      json_printf(JSTR("Expected ':' in object\n"));
      return false;
    }
    
    curr->value = (JsonValue*)json_alloc(sizeof(JsonValue));
    json_parse_value(c, curr->value);
    return true;
  };
  
  if (json_peek(c) != JSTR('}')) {
    
    parse_field(head);
    
    json_char next = json_get(c);
    if (next == JSTR('}') || next != JSTR(',')) {
      if (next != JSTR('}') && next != JSTR(',')) {
        json_printf(JSTR("Unknown token in object '%c'\n"), next);
      }
      return head;
    }
    
    while (true) {
      JsonObject* curr = (JsonObject*)json_alloc(sizeof(JsonObject));
      
      if (!parse_field(curr)) break;
      
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
        uint64_t word_start = ftell(c->file);
        
        uint32_t word_length = 0;
        do {
          json_read(c, 1);
          if (c->buffer[0] >= JSTR('a') && c->buffer[0] <= JSTR('z')) {
            ++word_length;
          } else {
            break;
          }
        } while (true);
        
        fseek(c->file, word_start, SEEK_SET);
        json_read(c, word_length);
        
        if (json_strcmp(c->buffer, JSTR("true")) == 0) {
          value->type = JSON_BOOL;
          value->bool_value = true;
        } else if (json_strcmp(c->buffer, JSTR("false")) == 0) {
          value->type = JSON_BOOL;
          value->bool_value = false;
        } else if (json_strcmp(c->buffer, JSTR("null")) == 0) {
          value->type = JSON_NULL;
        } else {
          json_printf(JSTR("Unknown token '%s'\n"), c->buffer);
          c->is_parsing = false;
        }
      }
    }
  }
}

JsonValue json_parse(const char* path) {
  JsonContext c = {};
  c.is_parsing = true;
  
  JsonValue value = {};
  
  c.file = fopen(path, JSON_READ_MODE);
  
  if (!c.file) {
    printf("Could not open file '%s'\n", path);
    return {};
  }
  
  json_parse_value(&c, &value);
  
  fclose(c.file);
  
  return value;
}

static void json_export_value(JsonValue* value, FILE* file, int indent_level, bool minified) {
  auto print_indent = [file, minified](int indent_level) {
    if (indent_level > 0 && !minified) {
      json_fprintf(file, JSTR("%*c"), indent_level, JSON_INDENT_CHAR);
    }
  };
  
  auto print_newline = [file, minified]() {
    if (!minified) {
      json_fprintf(file, JSTR("\n"));
    }
  };
  
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
      json_fprintf(file, JSTR("%lf"), value->number_value);
      break;
    }
    
    case JSON_OBJECT: {
      json_fprintf(file, JSTR("{"));
      print_newline();
      
      if (value->object_value) {
        for (JsonObject* obj = value->object_value; obj != nullptr; obj = obj->next) {
          print_indent(indent_level);
          json_fprintf(file, JSTR("\"%s\": "), obj->key);
          json_export_value(obj->value, file, indent_level + JSON_INDENT_STEP, minified);
          
          if (obj->next) {
            json_fprintf(file, JSTR(","));
            print_newline();
          } else {
            print_newline();
          }
        }
      }
      indent_level -= JSON_INDENT_STEP;
      print_indent(indent_level);
      json_fprintf(file, JSTR("}"));
      break;
    }
    
    case JSON_ARRAY: {
      json_fprintf(file, JSTR("["));
      print_newline();
      
      for (int i = 0; i < value->array_value->count; ++i) {
        print_indent(indent_level);
        json_export_value(&value->array_value->values[i], file, indent_level + JSON_INDENT_STEP, minified);
        
        if (i + 1 < value->array_value->count) {
          json_fprintf(file, JSTR(","));
          print_newline();
        } else {
          print_newline();
        }
      }
      indent_level -= JSON_INDENT_STEP;
      print_indent(indent_level);
      json_fprintf(file, JSTR("]"));
      break;
    }
    
    case JSON_BOOL: {
      json_fprintf(file, JSTR("%s"), (value->bool_value) ? JSTR("true") : JSTR("false"));
      break;
    }
  }
}

bool json_export(JsonValue* value, const char* path, bool minified) {
  if (!value || !path) return false;
  
  FILE* file = fopen(path, JSON_WRITE_MODE);
  
  if (!file) {
    printf("Could not create file for writing '%s'\n", path);
    return false;
  }
  
  json_export_value(value, file, JSON_INDENT_STEP, minified);
  
  fflush(file);
  fclose(file);
  
  return true;
}

JsonValue* json_find_field(JsonValue* value, const json_char* key) {
  if (!value || !key || value->type != JSON_OBJECT) return nullptr;
  
  for (JsonObject* v = value->object_value; v != nullptr; v = v->next) {
    if (json_strcmp(key, v->key) == 0) {
      return v->value;
    }
  }
  
  return nullptr;
}

JsonValue& json_find_field_ref(JsonValue* value, const json_char* key) {
  for (JsonObject* v = value->object_value; v != nullptr; v = v->next) {
    if (json_strcmp(key, v->key) == 0) {
      return *v->value;
    }
  }
  
  JsonValue dummy = {};
  return dummy;
}

void json_free(JsonValue* value) {
  if (!value) return;
  
  switch (value->type) {
    case JSON_STRING: {
      JSON_FREE(value->string_value);
      break;
    }
    
    case JSON_OBJECT: {
      // Get latest node
      
      JsonObject* head = value->object_value;
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
      for (int i = 0; i < value->array_value->count; ++i) {
        json_free(&value->array_value->values[i]);
      }
      JSON_FREE(value->array_value->values);
      JSON_FREE(value->array_value);
      break;
    }
  }
}

#endif // JSON_IMPLEMENTATION
