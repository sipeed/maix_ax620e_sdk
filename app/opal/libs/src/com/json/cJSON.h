/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef opalJSON__h
#define opalJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 3 define options:

OPALJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
OPALJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
OPALJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the OPALJSON_API_VISIBILITY flag to "export" the same symbols the way OPALJSON_EXPORT_SYMBOLS does

*/

#define OPALJSON_CDECL __cdecl
#define OPALJSON_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(OPALJSON_HIDE_SYMBOLS) && !defined(OPALJSON_IMPORT_SYMBOLS) && !defined(OPALJSON_EXPORT_SYMBOLS)
#define OPALJSON_EXPORT_SYMBOLS
#endif

#if defined(OPALJSON_HIDE_SYMBOLS)
#define OPALJSON_PUBLIC(type)   type OPALJSON_STDCALL
#elif defined(OPALJSON_EXPORT_SYMBOLS)
#define OPALJSON_PUBLIC(type)   __declspec(dllexport) type OPALJSON_STDCALL
#elif defined(OPALJSON_IMPORT_SYMBOLS)
#define OPALJSON_PUBLIC(type)   __declspec(dllimport) type OPALJSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define OPALJSON_CDECL
#define OPALJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(OPALJSON_API_VISIBILITY)
#define OPALJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define OPALJSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define OPALJSON_VERSION_MAJOR 1
#define OPALJSON_VERSION_MINOR 7
#define OPALJSON_VERSION_PATCH 18

#include <stddef.h>

/* opalJSON Types: */
#define opalJSON_Invalid (0)
#define opalJSON_False  (1 << 0)
#define opalJSON_True   (1 << 1)
#define opalJSON_NULL   (1 << 2)
#define opalJSON_Number (1 << 3)
#define opalJSON_String (1 << 4)
#define opalJSON_Array  (1 << 5)
#define opalJSON_Object (1 << 6)
#define opalJSON_Raw    (1 << 7) /* raw json */

#define opalJSON_IsReference 256
#define opalJSON_StringIsConst 512

/* The opalJSON structure: */
typedef struct opalJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct opalJSON *next;
    struct opalJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct opalJSON *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==opalJSON_String  and type == opalJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use opalJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==opalJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} opalJSON;

typedef struct opalJSON_Hooks
{
      /* malloc/free are CDECL on Windows regardless of the default calling convention of the compiler, so ensure the hooks allow passing those functions directly. */
      void *(OPALJSON_CDECL *malloc_fn)(size_t sz);
      void (OPALJSON_CDECL *free_fn)(void *ptr);
} opalJSON_Hooks;

typedef int opalJSON_bool;

/* Limits how deeply nested arrays/objects can be before opalJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef OPALJSON_NESTING_LIMIT
#define OPALJSON_NESTING_LIMIT 1000
#endif

/* returns the version of opalJSON as a string */
OPALJSON_PUBLIC(const char*) opalJSON_Version(void);

/* Supply malloc, realloc and free functions to opalJSON */
OPALJSON_PUBLIC(void) opalJSON_InitHooks(opalJSON_Hooks* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of opalJSON_Parse (with opalJSON_Delete) and opalJSON_Print (with stdlib free, opalJSON_Hooks.free_fn, or opalJSON_free as appropriate). The exception is opalJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a opalJSON object you can interrogate. */
OPALJSON_PUBLIC(opalJSON *) opalJSON_Parse(const char *value);
OPALJSON_PUBLIC(opalJSON *) opalJSON_ParseWithLength(const char *value, size_t buffer_length);
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match opalJSON_GetErrorPtr(). */
OPALJSON_PUBLIC(opalJSON *) opalJSON_ParseWithOpts(const char *value, const char **return_parse_end, opalJSON_bool require_null_terminated);
OPALJSON_PUBLIC(opalJSON *) opalJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, opalJSON_bool require_null_terminated);

/* Render a opalJSON entity to text for transfer/storage. */
OPALJSON_PUBLIC(char *) opalJSON_Print(const opalJSON *item);
/* Render a opalJSON entity to text for transfer/storage without any formatting. */
OPALJSON_PUBLIC(char *) opalJSON_PrintUnformatted(const opalJSON *item);
/* Render a opalJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
OPALJSON_PUBLIC(char *) opalJSON_PrintBuffered(const opalJSON *item, int prebuffer, opalJSON_bool fmt);
/* Render a opalJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: opalJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_PrintPreallocated(opalJSON *item, char *buffer, const int length, const opalJSON_bool format);
/* Delete a opalJSON entity and all subentities. */
OPALJSON_PUBLIC(void) opalJSON_Delete(opalJSON *item);

/* Returns the number of items in an array (or object). */
OPALJSON_PUBLIC(int) opalJSON_GetArraySize(const opalJSON *array);
/* Retrieve item number "index" from array "array". Returns NULL if unsuccessful. */
OPALJSON_PUBLIC(opalJSON *) opalJSON_GetArrayItem(const opalJSON *array, int index);
/* Get item "string" from object. Case insensitive. */
OPALJSON_PUBLIC(opalJSON *) opalJSON_GetObjectItem(const opalJSON * const object, const char * const string);
OPALJSON_PUBLIC(opalJSON *) opalJSON_GetObjectItemCaseSensitive(const opalJSON * const object, const char * const string);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_HasObjectItem(const opalJSON *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when opalJSON_Parse() returns 0. 0 when opalJSON_Parse() succeeds. */
OPALJSON_PUBLIC(const char *) opalJSON_GetErrorPtr(void);

/* Check item type and return its value */
OPALJSON_PUBLIC(char *) opalJSON_GetStringValue(const opalJSON * const item);
OPALJSON_PUBLIC(double) opalJSON_GetNumberValue(const opalJSON * const item);

/* These functions check the type of an item */
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsInvalid(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsFalse(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsTrue(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsBool(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsNull(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsNumber(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsString(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsArray(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsObject(const opalJSON * const item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_IsRaw(const opalJSON * const item);

/* These calls create a opalJSON item of the appropriate type. */
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateNull(void);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateTrue(void);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateFalse(void);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateBool(opalJSON_bool boolean);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateNumber(double num);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateString(const char *string);
/* raw json */
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateRaw(const char *raw);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateArray(void);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by opalJSON_Delete */
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateStringReference(const char *string);
/* Create an object/array that only references it's elements so
 * they will not be freed by opalJSON_Delete */
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateObjectReference(const opalJSON *child);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateArrayReference(const opalJSON *child);

/* These utilities create an Array of count items.
 * The parameter count cannot be greater than the number of elements in the number array, otherwise array access will be out of bounds.*/
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateIntArray(const int *numbers, int count);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateFloatArray(const float *numbers, int count);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateDoubleArray(const double *numbers, int count);
OPALJSON_PUBLIC(opalJSON *) opalJSON_CreateStringArray(const char *const *strings, int count);

/* Append item to the specified array/object. */
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_AddItemToArray(opalJSON *array, opalJSON *item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_AddItemToObject(opalJSON *object, const char *string, opalJSON *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the opalJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & opalJSON_StringIsConst) is zero before
 * writing to `item->string` */
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_AddItemToObjectCS(opalJSON *object, const char *string, opalJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing opalJSON to a new opalJSON, but don't want to corrupt your existing opalJSON. */
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_AddItemReferenceToArray(opalJSON *array, opalJSON *item);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_AddItemReferenceToObject(opalJSON *object, const char *string, opalJSON *item);

/* Remove/Detach items from Arrays/Objects. */
OPALJSON_PUBLIC(opalJSON *) opalJSON_DetachItemViaPointer(opalJSON *parent, opalJSON * const item);
OPALJSON_PUBLIC(opalJSON *) opalJSON_DetachItemFromArray(opalJSON *array, int which);
OPALJSON_PUBLIC(void) opalJSON_DeleteItemFromArray(opalJSON *array, int which);
OPALJSON_PUBLIC(opalJSON *) opalJSON_DetachItemFromObject(opalJSON *object, const char *string);
OPALJSON_PUBLIC(opalJSON *) opalJSON_DetachItemFromObjectCaseSensitive(opalJSON *object, const char *string);
OPALJSON_PUBLIC(void) opalJSON_DeleteItemFromObject(opalJSON *object, const char *string);
OPALJSON_PUBLIC(void) opalJSON_DeleteItemFromObjectCaseSensitive(opalJSON *object, const char *string);

/* Update array items. */
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_InsertItemInArray(opalJSON *array, int which, opalJSON *newitem); /* Shifts pre-existing items to the right. */
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_ReplaceItemViaPointer(opalJSON * const parent, opalJSON * const item, opalJSON * replacement);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_ReplaceItemInArray(opalJSON *array, int which, opalJSON *newitem);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_ReplaceItemInObject(opalJSON *object,const char *string,opalJSON *newitem);
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_ReplaceItemInObjectCaseSensitive(opalJSON *object,const char *string,opalJSON *newitem);

/* Duplicate a opalJSON item */
OPALJSON_PUBLIC(opalJSON *) opalJSON_Duplicate(const opalJSON *item, opalJSON_bool recurse);
/* Duplicate will create a new, identical opalJSON item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two opalJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
OPALJSON_PUBLIC(opalJSON_bool) opalJSON_Compare(const opalJSON * const a, const opalJSON * const b, const opalJSON_bool case_sensitive);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings.
 * The input pointer json cannot point to a read-only address area, such as a string constant,
 * but should point to a readable and writable address area. */
OPALJSON_PUBLIC(void) opalJSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddNullToObject(opalJSON * const object, const char * const name);
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddTrueToObject(opalJSON * const object, const char * const name);
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddFalseToObject(opalJSON * const object, const char * const name);
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddBoolToObject(opalJSON * const object, const char * const name, const opalJSON_bool boolean);
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddNumberToObject(opalJSON * const object, const char * const name, const double number);
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddStringToObject(opalJSON * const object, const char * const name, const char * const string);
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddRawToObject(opalJSON * const object, const char * const name, const char * const raw);
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddObjectToObject(opalJSON * const object, const char * const name);
OPALJSON_PUBLIC(opalJSON*) opalJSON_AddArrayToObject(opalJSON * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define opalJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the opalJSON_SetNumberValue macro */
OPALJSON_PUBLIC(double) opalJSON_SetNumberHelper(opalJSON *object, double number);
#define opalJSON_SetNumberValue(object, number) ((object != NULL) ? opalJSON_SetNumberHelper(object, (double)number) : (number))
/* Change the valuestring of a opalJSON_String object, only takes effect when type of object is opalJSON_String */
OPALJSON_PUBLIC(char*) opalJSON_SetValuestring(opalJSON *object, const char *valuestring);

/* If the object is not a boolean type this does nothing and returns opalJSON_Invalid else it returns the new type*/
#define opalJSON_SetBoolValue(object, boolValue) ( \
    (object != NULL && ((object)->type & (opalJSON_False|opalJSON_True))) ? \
    (object)->type=((object)->type &(~(opalJSON_False|opalJSON_True)))|((boolValue)?opalJSON_True:opalJSON_False) : \
    opalJSON_Invalid\
)

/* Macro for iterating over an array or object */
#define opalJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with opalJSON_InitHooks */
OPALJSON_PUBLIC(void *) opalJSON_malloc(size_t size);
OPALJSON_PUBLIC(void) opalJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
