
#define JSON_IMPLEMENTATION
// #define JSON_USE_SINGLE_BYTE
#include "json.h"

int main() {
  
  JsonValue val = json_object();
  json_add_field(&val, L"greeting", json_string(L"Hello, world!"));
  json_add_field(&val, L"numnum", json_number(69.69));
  
  JsonValue arr = json_array();
  json_add_field(&val, L"order", arr);
  json_add_element(&arr, json_number(1.0f));
  json_add_element(&arr, json_number(2.0f));
  json_add_element(&arr, json_number(3.0f));
  
  json_add_field(&val, L"funny", json_null());
  json_add_field(&val, L"haha", json_string(L'\u0FC3'));
  
  json_export(&val, "data/export.json");
  
  JsonValue val2 = json_parse_from_file("data/canada.json");
  json_export(&val2, "data/test_exp.json");
  json_free(&val);
  json_free(&val2);
  
  return 0;
}
