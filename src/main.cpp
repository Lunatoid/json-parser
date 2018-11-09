
#define JSON_IMPLEMENTATION
// #define JSON_USE_SINGLE_BYTE
#include "json.h"

int main() {
  
  JsonValue str = json_string(JSTR("DON'T"));
  JsonValue arr1 = json_array();
  
  json_add_element(&arr1, str);
  
  JsonValue arr2 = json_array();
  
  json_add_element(&arr2, str);
  
  wprintf(L"1. %s\n", arr1[0].string_value);
  wprintf(L"2. %s\n", arr2[0].string_value);
  printf("----------\n");
  json_free(&arr2);
  wprintf(L"1. %s\n", arr1[0].string_value);
  wprintf(L"2. %s\n", arr2[0].string_value);
  
  return 0;
}
