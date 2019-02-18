
#define JSON_IMPLEMENTATION
// #define JSON_USE_SINGLE_BYTE
#include "json.h"

int main() {
  JsonValue json = json_object();
  
  // Add array
  JsonValue arr = json_array();
  
  for (int i = 0; i < 10; ++i) {
    json_add_element(&arr, json_number(i));
  }
  
  json_remove_element(&arr, 4);
  
  json_add_field(&json, JSTR("array_test"), arr);
  
  // Add number
  json_add_field(&json, JSTR("number_test"), json_number(123));
  
  // Add object
  JsonValue obj = json_object();
  json_add_field(&obj, JSTR("field"), json_string(JSTR("I am an object!")));
  json_add_field(&obj, JSTR("is_object"), json_bool(1));
  
  json_add_field(&json, JSTR("object_test"), obj);
  
  // Add a null
  json_add_field(&json, JSTR("existence"), json_null());
  
  // Export json object and free it
  json_export(&json, "export.json", 0);
  json_free(&json);
  
  return 0;
}
