
#define JSON_IMPLEMENTATION
// #define JSON_USE_SINGLE_BYTE
#include "json.h"

int main() {
  
  JsonValue json = json_array();
  
  for (int i = 0; i < 10; ++i) {
    json_add_element(&json, json_number(i));
  }
  
  json_remove_element(&json, 4);
  
  for (int i = 0; i < json.array_value->count; ++i) {
    printf("%.0f", json.array_value->values[i].number_value);
  }
  
  return 0;
}
