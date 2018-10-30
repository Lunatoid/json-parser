
#define JSON_IMPLEMENTATION
// #define JSON_USE_SINGLE_BYTE
#include "json.h"

int main() {
  
  /*
  JsonValue val = json_object();
  json_add_field(&val, L"greeting", json_string(L"Hello, world!"));
  json_add_field(&val, L"numnum", json_number(69.69));
  
  JsonValue arr = json_array();
  json_add_field(&val, L"order", arr);
  json_add_value(&arr, json_number(1.0f));
  json_add_value(&arr, json_number(2.0f));
  json_add_value(&arr, json_number(3.0f));
  
  json_add_field(&val, L"funny", json_null());
  
  json_export(&val, "data/export.json");
  */
  
  JsonValue val = json_parse("data/test.json");
  json_export(&val, "data/test_exp.json");
  json_free(&val);
  
  return 0;
}
