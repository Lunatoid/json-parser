# json-parser
A zlib licensed json parser written in C, supports wchar_t.

## API
The JSON parser works with both `char` and `wchar_t`. It will use `wchar_t` by default,
if you want to use single-byte characters instead you can `#define JSON_USE_SINGLE_BYTE`
`json_char` is either defined as a `char` or `wchar_t`, depending on which to use.


The macro JSTR will append an L to your string if using multibyte and do nothing in single byte.
Example:
  * Multibyte:   `JSTR("Hello world")` -> `L"Hello world"`
  * Single byte: `JSTR("Hello world")` ->  `"Hello world"`

### Parsing
To parse JSON, you have the following options:
```cpp
json_char* some_text = JSTR("[ 1, 2, 3, 4, 5, null ]");
JsonValue json = json_parse(some_text);
```
Or if you want to load it from a file
```cpp
JsonValue json = json_parse_from_file("config.json");
```

Note that `json_parse_from_file()` will heap-allocate a string that can hold the
value and then call `json_parse()` on that and free it.

### Accessing

To access the various types that the JsonValue can hold, you can access them in various different ways.
There are 6 defined types in the implementation:
  * `JSON_NULL`
  * `JSON_STRING` -> `JsonValue.string_value` (`json_char*`)
  * `JSON_NUMBER` -> `JsonValue.number_value` (`double`)
  * `JSON_OBJECT` -> `JsonValue.object_value` (`JsonObject*`)
  * `JSON_ARRAY`  -> `JsonValue.array_value`  (`JsonArray*`)
  * `JSON_BOOL`   -> `JsonValue.bool_value`   (`bool`)

You can get the type of the `JsonValue` by doing `JsonValue.type` and comparing it with the enum listed above.
Note that all these values are in a union.

For arrays and objects you can use the overloaded `[]` operator if using C++:
```cpp
JsonValue json = json_parse(JSTR("[ 0, 1, 2, 3 ]"));

for (int i = 0; i < json.array_value->count; ++i) {
  JsonValue& val = json[i];
  // OR
  JsonValue& val = json.array_value->values[i];
  // ...
}
```
Or with objects:
```cpp
JsonValue json = json_parse(JSTR("{ \"age\": 41.9 }");

JsonValue& age = json[JSTR("age")];
// OR
JsonValue& age = json_find_field_ref(&json, JSTR("age"));
```

### Creating

Creating new JSON values is quite easy.
You can call `json_*type*` to get a `JsonValue` of that type:
```cpp
json_char* str = json_string(JSTR("Hello, world!"));
double num = json_number(10.04);
```

To add elements to an array you can use `json_add_element():`
```cpp
JsonValue arr = json_array();
json_add_element(&arr, json_number(10.0));
json_add_element(&arr, json_bool(1));

JsonValue nested_arr = json_array();
json_add_element(&nested_arr, json_string(JSTR("wow")));

json_add_element(&arr, nested_arr);
```
If you want to remove an element, you can use `json_remove_element(&arr, index)`
Note that it will take care of resizing an array. It only grows.
If you add a heap-allocated `JsonValue` (array/object/string) to a `JsonValue` it will assume ownership of the pointer.

DON'T DO THIS:
```cpp
JsonValue str = json_string(JSTR("DON'T"));

JsonValue arr1 = json_array();
json_add_element(&arr1, str);

JsonValue arr2 = json_array();
json_add_element(&arr2, str);
```
In the above example, both arrays have assumed ownership over the string.
If you were to free 1 and still use the other you'd have a free-after-use error. BEWARE!
Consider using `json_duplicate()` to make copies of a JsonValue.

Adding fields to objects is similar to arrays, you can use `json_add_field()`:
```cpp
JsonValue obj = json_object();
json_add_field(&obj, JSTR("key"), json_number(10.0));
```

Note that it will not check if a certain field already exists as per the ECMA-404 standard.
More control over seeing if fields exist/removing fields/etc is still `@TODO`

### Exporting

To export a JsonValue all you have to do is call `json_export()`:
```cpp
JsonValue json = ...;
JSON_BOOL_TYPE minified = 1;
json_export(&json, "path/to/file.json", minified);
```
Stringifying a `JsonValue` is still @TODO

Customize your export:
  * `#define JSON_INDENT_CHAR` to change the character used for indenting. Space ' ' by default.
  * `#define JSON_INDENT_STEP` by how many characters it will indent. 2 is the default.

### Settings

These settings allow you to customize how the parser behaves.
Enabling these settings will make the parser no longer compliant with ECMA-404.

 * `#define JSON_ALLOW_EXP_DECIMALS`:  Allow floating-point numbers in the number exponent e.g. `10e2.4`
 * `#define JSON_ALLOW_COMMENTS` Allows single-line comments using `#`, will be ignored by the parser
