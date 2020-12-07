/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW11 program 2
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void guard(bool expression, char const *errorMessage);
void guardFmt(bool expression, char const *errorMessageFormat, ...);
void guardFmtVA(bool expression, char const *errorMessageFormat, va_list errorMessageFormatArgs);

void guardNotNull(void const *object, char const *paramName, char const *callerName);

void *safeMalloc(size_t size, char const *callerDescription);
void *safeRealloc(void *memory, size_t newSize, char const *callerDescription);

/**
 * Turn the given macro token into a string literal.
 *
 * @param macroToken The macro token.
 *
 * @returns The string literal.
 */
#define STRINGIFY(macroToken) #macroToken

/**
 * Get the length of the given compile-time array.
 *
 * @param array The array.
 *
 * @returns The length number literal.
 */
#define ARRAY_LENGTH(array) (sizeof (array) / sizeof (array)[0])

/**
 * Get a stack-allocated mutable string from the given string literal.
 *
 * @param stringLiteral The string literal.
 *
 * @returns The `char [length + 1]`-typed stack-allocated string.
*/
#define MUTABLE_STRING(stringLiteral) ((char [ARRAY_LENGTH(stringLiteral)]){ stringLiteral })

/**
 * Declare (.h file) a generic Result class which holds no success value but can hold a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DECLARE_VOID_RESULT(TResult, TError) \
    struct TResult; \
    typedef struct TResult * TResult; \
    typedef struct TResult const * Const##TResult; \
    \
    TResult TResult##_success(void); \
    TResult TResult##_failure(TError error); \
    void TResult##_destroy(TResult result); \
    \
    bool TResult##_isSuccess(Const##TResult result); \
    TError TResult##_getError(Const##TResult result); \
    TError TResult##_getErrorAndDestroy(TResult result);

/**
 * Define (.c file) a generic Result class which holds no success value but can hold a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DEFINE_VOID_RESULT(TResult, TError) \
    DECLARE_VOID_RESULT(TResult, TError) \
    \
    struct TResult { \
        bool success; \
        TError error; \
    }; \
    \
    TResult TResult##_success(void) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = true; \
        return result; \
    } \
    \
    TResult TResult##_failure(TError const error) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = false; \
        result->error = error; \
        return result; \
    } \
    \
    void TResult##_destroy(TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_destroy)); \
        free(result); \
    } \
    \
    bool TResult##_isSuccess(Const##TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_isSuccess)); \
        return result->success; \
    } \
    \
    TError TResult##_getError(Const##TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_getError)); \
        guardFmt(!result->success, "%s: Cannot get result error. Result is success", STRINGIFY(TResult##_getError)); \
        return result->error; \
    } \
    \
    TError TResult##_getErrorAndDestroy(TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_getErrorAndDestroy)); \
        guardFmt(!result->success, "%s: Cannot get result error. Result is success", STRINGIFY(TResult##_getErrorAndDestroy)); \
        \
        TError const error = result->error; \
        TResult##_destroy(result); \
        return error; \
    }

/**
 * Declare (.h file) a generic Result class which can hold either a sucess value or a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DECLARE_RESULT(TResult, TValue, TError) \
    struct TResult; \
    typedef struct TResult * TResult; \
    typedef struct TResult const * Const##TResult; \
    \
    TResult TResult##_success(TValue value); \
    TResult TResult##_failure(TError error); \
    void TResult##_destroy(TResult result); \
    \
    bool TResult##_isSuccess(Const##TResult result); \
    TValue TResult##_getValue(Const##TResult result); \
    TValue TResult##_getValueAndDestroy(TResult result); \
    TError TResult##_getError(Const##TResult result); \
    TError TResult##_getErrorAndDestroy(TResult result);

/**
 * Define (.c file) a generic Result class which can hold either a sucess value or a failure error.
 *
 * @param TResult The name of the new type.
 * @param TValue The type of the success value.
 * @param TError The type of the failure error
*/
#define DEFINE_RESULT(TResult, TValue, TError) \
    DECLARE_RESULT(TResult, TValue, TError) \
    \
    struct TResult { \
        bool success; \
        TValue value; \
        TError error; \
    }; \
    \
    TResult TResult##_success(TValue const value) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = true; \
        result->value = value; \
        return result; \
    } \
    \
    TResult TResult##_failure(TError const error) { \
        TResult const result = safeMalloc(sizeof *result, STRINGIFY(TResult##_success)); \
        result->success = false; \
        result->error = error; \
        return result; \
    } \
    \
    void TResult##_destroy(TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_destroy)); \
        free(result); \
    } \
    \
    bool TResult##_isSuccess(Const##TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_isSuccess)); \
        return result->success; \
    } \
    \
    TValue TResult##_getValue(Const##TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_getValue)); \
        guardFmt(result->success, "%s: Cannot get result value. Result is failure", STRINGIFY(TResult##_getValue)); \
        return result->value; \
    } \
    \
    TValue TResult##_getValueAndDestroy(TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_getValueAndDestroy)); \
        guardFmt(result->success, "%s: Cannot get result value. Result is failure", STRINGIFY(TResult##_getValueAndDestroy)); \
        \
        TValue const value = result->value; \
        TResult##_destroy(result); \
        return value; \
    } \
    \
    TError TResult##_getError(Const##TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_getError)); \
        guardFmt(!result->success, "%s: Cannot get result error. Result is success", STRINGIFY(TResult##_getError)); \
        return result->error; \
    } \
    \
    TError TResult##_getErrorAndDestroy(TResult const result) { \
        guardNotNull(result, "result", STRINGIFY(TResult##_getErrorAndDestroy)); \
        guardFmt(!result->success, "%s: Cannot get result error. Result is success", STRINGIFY(TResult##_getErrorAndDestroy)); \
        \
        TError const error = result->error; \
        TResult##_destroy(result); \
        return error; \
    }

void abortWithError(char const *errorMessage);
void abortWithErrorFmt(char const *errorMessageFormat, ...);
void abortWithErrorFmtVA(char const *errorMessageFormat, va_list errorMessageFormatArgs);

#define DECLARE_ENUMERATOR(TEnumerator, TItem) \
    struct TEnumerator; \
    typedef struct TEnumerator * TEnumerator; \
    typedef struct TEnumerator const * Const##TEnumerator; \
    \
    void TEnumerator##_destroy(TEnumerator enumerator); \
    \
    bool TEnumerator##_moveNext(TEnumerator enumerator); \
    TItem TEnumerator##_current(Const##TEnumerator enumerator); \
    void TEnumerator##_reset(TEnumerator enumerator);

/**
 * Declare (.h file) a generic ListEnumerator class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DECLARE_LIST_ENUMERATOR(TList, TItem) \
    DECLARE_ENUMERATOR(TList##Enumerator, TItem) \
    \
    TList##Enumerator TList##Enumerator##_create(Const##TList list, int direction);

/**
 * Define (.c file) a generic ListEnumerator class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DEFINE_LIST_ENUMERATOR(TList, TItem) \
    DECLARE_LIST_ENUMERATOR(TList, TItem) \
    \
    struct TList##Enumerator { \
        Const##TList list; \
        int direction; \
        int currentIndex; \
    }; \
    \
    static void TList##Enumerator##_guardCurrentIndexInRange(Const##TList##Enumerator enumerator, char const *callerName); \
    \
    TList##Enumerator TList##Enumerator##_create(Const##TList const list, int const direction) { \
        guardNotNull(list, "list", STRINGIFY(TList##Enumerator##_create)); \
        \
        TList##Enumerator const enumerator = safeMalloc(sizeof *enumerator, STRINGIFY(TList##Enumerator##_create)); \
        enumerator->list = list; \
        enumerator->direction = direction; \
        enumerator->currentIndex = direction == 1 ? -1 : (int)TList##_count(list); \
        return enumerator; \
    } \
    \
    void TList##Enumerator##_destroy(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_destroy)); \
        free(enumerator); \
    } \
    \
    bool TList##Enumerator##_moveNext(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_moveNext)); \
        \
        enumerator->currentIndex += enumerator->direction; \
        return enumerator->currentIndex >= 0 && enumerator->currentIndex < (int)TList##_count(enumerator->list); \
    } \
    \
    TItem TList##Enumerator##_current(Const##TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_current)); \
        TList##Enumerator##_guardCurrentIndexInRange(enumerator, STRINGIFY(TList##Enumerator##_current)); \
        return TList##_get(enumerator->list, (size_t)enumerator->currentIndex); \
    } \
    \
    void TList##Enumerator##_reset(TList##Enumerator const enumerator) { \
        guardNotNull(enumerator, "enumerator", STRINGIFY(TList##Enumerator##_reset)); \
        enumerator->currentIndex = enumerator->direction == 1 ? -1 : (int)TList##_count(enumerator->list); \
    } \
    \
    static void TList##Enumerator##_guardCurrentIndexInRange(Const##TList##Enumerator const enumerator, char const * const callerName) { \
        if (enumerator->currentIndex < 0 || enumerator->currentIndex >= (int)TList##_count(enumerator->list)) { \
            abortWithErrorFmt( \
                "%s: Current index (%d) is out of range (list count: %zu)", \
                callerName, \
                enumerator->currentIndex, \
                TList##_count(enumerator->list) \
            ); \
        } \
    }

/**
 * Declare (.h file) a typedef to a function pointer for a function with the specified args that returns void.
 *
 * @param TAction The name of the new type.
 * @param ... The function's parameter types.
 */
#define DECLARE_ACTION(TAction, ...) typedef void (*TAction)(__VA_ARGS__);

/**
 * Declare (.h file) a typedef to a function pointer for a function with the specified args and return type.
 *
 * @param TFunc The name of the new type.
 * @param TResult The function's return type.
 * @param ... The function's parameter types.
 */
#define DECLARE_FUNC(TFunc, TResult, ...) typedef TResult (*TFunc)(__VA_ARGS__);

/**
 * Declare (.h file) a generic List class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DECLARE_LIST(TList, TItem) \
    struct TList; \
    typedef struct TList * TList; \
    typedef struct TList const * Const##TList; \
    \
    DECLARE_ACTION(TList##ForEachCallback, void *, size_t, TItem) \
    DECLARE_FUNC(TList##FindCallback, bool, void *, size_t, TItem) \
    DECLARE_RESULT(TList##FindItemResult, TItem, void *) \
    \
    DECLARE_LIST_ENUMERATOR(TList, TItem) \
    \
    TList TList##_create(void); \
    TList TList##_fromItems(TItem const *items, size_t count); \
    TList TList##_fromList(Const##TList list); \
    void TList##_destroy(TList list); \
    \
    TItem const *TList##_items(Const##TList list); \
    size_t TList##_count(Const##TList list); \
    bool TList##_empty(Const##TList list); \
    \
    TItem TList##_get(Const##TList list, size_t index); \
    TItem *TList##_getPtr(TList list, size_t index); \
    TItem const *TList##_constGetPtr(Const##TList list, size_t index); \
    \
    void TList##_add(TList list, TItem item); \
    void TList##_addMany(TList list, TItem const *items, size_t count); \
    void TList##_insert(TList list, size_t index, TItem item); \
    void TList##_insertMany(TList list, size_t index, TItem const *items, size_t count); \
    void TList##_set(TList list, size_t index, TItem item); \
    \
    void TList##_removeAt(TList list, size_t index); \
    void TList##_removeManyAt(TList list, size_t startIndex, size_t count); \
    void TList##_clear(TList list); \
    \
    void TList##_forEach(Const##TList list, void *state, TList##ForEachCallback callback); \
    void TList##_forEachReverse(Const##TList list, void *state, TList##ForEachCallback callback); \
    bool TList##_has(Const##TList list, TItem item); \
    size_t TList##_indexOf(Const##TList list, TItem item); \
    size_t TList##_lastIndexOf(Const##TList list, TItem item); \
    bool TList##_findHas(Const##TList list, void *state, TList##FindCallback callback); \
    TList##FindItemResult TList##_find(Const##TList list, void *state, TList##FindCallback callback); \
    size_t TList##_findIndex(Const##TList list, void *state, TList##FindCallback callback); \
    TList##FindItemResult TList##_findLast(Const##TList list, void *state, TList##FindCallback callback); \
    size_t TList##_findLastIndex(Const##TList list, void *state, TList##FindCallback callback); \
    \
    TList##Enumerator TList##_enumerate(Const##TList list); \
    TList##Enumerator TList##_enumerateReverse(Const##TList list); \
    \
    void TList##_fillArray(Const##TList list, TItem *array, size_t startIndex, size_t count);

/**
 * Define (.c file) a generic List class.
 *
 * @param TList The name of the new type.
 * @param TItem The item type.
 */
#define DEFINE_LIST(TList, TItem) \
    DECLARE_LIST(TList, TItem) \
    \
    static void TList##_ensureCapacity(TList list, size_t targetCapacity); \
    static void TList##_guardIndexInRange(Const##TList list, size_t index, char const *callerName); \
    static void TList##_guardIndexInInsertRange(Const##TList list, size_t index, char const *callerName); \
    static void TList##_guardStartIndexAndCountInRange(Const##TList list, size_t startIndex, size_t count, char const *callerName); \
    \
    DEFINE_RESULT(TList##FindItemResult, TItem, void *) \
    \
    DEFINE_LIST_ENUMERATOR(TList, TItem) \
    \
    struct TList { \
        TItem *items; \
        size_t count; \
        size_t capacity; \
    }; \
    \
    TList TList##_create(void) { \
        TList const list = safeMalloc(sizeof *list, STRINGIFY(TList##_create)); \
        list->items = NULL; \
        list->count = 0; \
        list->capacity = 0; \
        return list; \
    } \
    \
    TList TList##_fromItems(TItem const * const items, size_t const count) { \
        guardNotNull(items, "items", STRINGIFY(TList##_fromItems)); \
        \
        TList const list = TList##_create(); \
        TList##_addMany(list, items, count); \
        return list; \
    } \
    \
    TList TList##_fromList(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_fromList)); \
        return TList##_fromItems(list->items, list->count); \
    } \
    \
    void TList##_destroy(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_destroy)); \
        \
        free(list->items); \
        free(list); \
    } \
    \
    TItem const *TList##_items(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_items)); \
        return list->items; \
    } \
    \
    size_t TList##_count(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_count)); \
        return list->count; \
    } \
    \
    bool TList##_empty(Const##TList list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_empty)); \
        return list->count == 0; \
    } \
    \
    TItem TList##_get(Const##TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_get)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_get)); \
        return list->items[index]; \
    } \
    \
    TItem *TList##_getPtr(TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_getPtr)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_getPtr)); \
        return &list->items[index]; \
    } \
    \
    TItem const *TList##_constGetPtr(Const##TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_constGetPtr)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_constGetPtr)); \
        return &list->items[index]; \
    } \
    \
    void TList##_add(TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_add)); \
        \
        TList##_ensureCapacity(list, list->count + 1); \
        list->items[list->count] = item; \
        list->count += 1; \
    } \
    \
    void TList##_addMany(TList const list, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_addMany)); \
        guardNotNull(items, "items", STRINGIFY(TList##_addMany)); \
        \
        TList##_ensureCapacity(list, list->count + count); \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            list->items[list->count + i] = item; \
        } \
        list->count += count; \
    } \
    \
    void TList##_insert(TList const list, size_t const index, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insert)); \
        TList##_guardIndexInInsertRange(list, index, STRINGIFY(TList##_insert)); \
        \
        TList##_ensureCapacity(list, list->count + 1); \
        for (int i = (int)list->count - 1; i >= (int)index; i -= 1) { \
            /* Shift each item at an index >= the target index one to the right */ \
            list->items[i + 1] = list->items[i]; \
        } \
        list->items[index] = item; \
        list->count += 1; \
    } \
    \
    void TList##_insertMany(TList const list, size_t const index, TItem const * const items, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_insertMany)); \
        TList##_guardIndexInInsertRange(list, index, STRINGIFY(TList##_insertMany)); \
        guardNotNull(items, "items", STRINGIFY(TList##_insertMany)); \
        \
        TList##_ensureCapacity(list, list->count + count); \
        for (int i = (int)list->count - 1; i >= (int)index; i -= 1) { \
            /* Shift each item at an index >= the target index count to the right */ \
            list->items[(size_t)i + count] = list->items[i]; \
        } \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = items[i]; \
            list->items[index + i] = item; \
        } \
        list->count += count; \
    } \
    \
    void TList##_set(TList const list, size_t const index, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_set)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_set)); \
        list->items[index] = item; \
    } \
    \
    void TList##_removeAt(TList const list, size_t const index) { \
        guardNotNull(list, "list", STRINGIFY(TList##_removeAt)); \
        TList##_guardIndexInRange(list, index, STRINGIFY(TList##_removeAt)); \
        \
        for (size_t i = index; i < list->count; i += 1) { \
            /* Shift each item at an index > the target index one to the left */ \
            list->items[i] = list->items[i + 1]; \
        } \
        list->count -= 1; \
    } \
    \
    void TList##_removeManyAt(TList const list, size_t const startIndex, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_removeManyAt)); \
        TList##_guardStartIndexAndCountInRange(list, startIndex, count, STRINGIFY(TList##_removeManyAt)); \
        \
        for (size_t i = startIndex; i < list->count; i += 1) { \
            /* Shift each item at an index > the start index count to the left */ \
            list->items[i] = list->items[i + count]; \
        } \
        list->count -= count; \
    } \
    \
    void TList##_clear(TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_clear)); \
        list->count = 0; \
    } \
    \
    void TList##_forEach(Const##TList const list, void * const state, TList##ForEachCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_forEach)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            callback(state, i, item); \
        } \
    } \
    \
    void TList##_forEachReverse(Const##TList const list, void * const state, TList##ForEachCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_forEachReverse)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            callback(state, (size_t)i, item); \
        } \
    } \
    \
    bool TList##_has(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_has)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return true; \
            } \
        } \
        \
        return false; \
    } \
    \
    size_t TList##_indexOf(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_indexOf)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    size_t TList##_lastIndexOf(Const##TList const list, TItem const item) { \
        guardNotNull(list, "list", STRINGIFY(TList##_lastIndexOf)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const someItem = list->items[i]; \
            if (someItem == item) { \
                return (size_t)i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    bool TList##_findHas(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findHas)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return true; \
            } \
        } \
        \
        return false; \
    } \
    \
    TList##FindItemResult TList##_find(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_find)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return TList##FindItemResult_success(item); \
            } \
        } \
        \
        return TList##FindItemResult_failure(NULL); \
    } \
    \
    size_t TList##_findIndex(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findIndex)); \
        \
        for (size_t i = 0; i < list->count; i += 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, i, item); \
            if (found) { \
                return i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    TList##FindItemResult TList##_findLast(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findLast)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, (size_t)i, item); \
            if (found) { \
                return TList##FindItemResult_success(item); \
            } \
        } \
        \
        return TList##FindItemResult_failure(NULL); \
    } \
    \
    size_t TList##_findLastIndex(Const##TList const list, void * const state, TList##FindCallback const callback) { \
        guardNotNull(list, "list", STRINGIFY(TList##_findLastIndex)); \
        \
        for (int i = (int)list->count - 1; i >= 0; i -= 1) { \
            TItem const item = list->items[i]; \
            bool const found = callback(state, (size_t)i, item); \
            if (found) { \
                return (size_t)i; \
            } \
        } \
        \
        return (size_t)-1; \
    } \
    \
    TList##Enumerator TList##_enumerate(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_enumerate)); \
        return TList##Enumerator_create(list, 1); \
    } \
    \
    TList##Enumerator TList##_enumerateReverse(Const##TList const list) { \
        guardNotNull(list, "list", STRINGIFY(TList##_enumerateReverse)); \
        return TList##Enumerator_create(list, -1); \
    } \
    \
    void TList##_fillArray(Const##TList const list, TItem * const array, size_t const startIndex, size_t const count) { \
        guardNotNull(list, "list", STRINGIFY(TList##_fillArray)); \
        guardNotNull(array, "array", STRINGIFY(TList##_fillArray)); \
        TList##_guardStartIndexAndCountInRange(list, startIndex, count, STRINGIFY(TList##_fillArray)); \
        \
        for (size_t i = 0; i < count; i += 1) { \
            TItem const item = list->items[i + startIndex]; \
            array[i] = item; \
        } \
    } \
    \
    static void TList##_ensureCapacity(TList const list, size_t const requiredCapacity) { \
        assert(list != NULL); \
        \
        if (requiredCapacity <= list->capacity) { \
            return; \
        } \
        \
        size_t newCapacity = list->capacity == 0 ? 4 : (list->capacity * 2); \
        while (newCapacity < requiredCapacity) { \
            newCapacity *= 2; \
        } \
        \
        list->items = safeRealloc(list->items, sizeof *list->items * newCapacity, STRINGIFY(TList##_ensureCapacity)); \
        list->capacity = newCapacity; \
    } \
    \
    static void TList##_guardIndexInRange(Const##TList const list, size_t const index, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        guardFmt( \
            index < list->count, \
            "%s: Index (%zu) must be in range (list count: %zu)", \
            callerName, \
            index, \
            list->count \
        ); \
    } \
    \
    static void TList##_guardIndexInInsertRange(Const##TList const list, size_t const index, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        guardFmt( \
            index <= list->count, \
            "%s: Index (%zu) must be in range (list count: %zu) or the next available index", \
            callerName, \
            index, \
            list->count \
        ); \
    } \
    \
    static void TList##_guardStartIndexAndCountInRange(Const##TList const list, size_t const startIndex, size_t const count, char const * const callerName) { \
        assert(list != NULL); \
        assert(callerName != NULL); \
        \
        if (count == 0) { \
            guardFmt( \
                startIndex <= list->count, \
                "%s: Start index (%zu) must be within range (list count: %zu)", \
                callerName, \
                startIndex, \
                list->count \
            ); \
        } else { \
            TList##_guardIndexInRange(list, startIndex, callerName); \
            \
            size_t const endIndex = startIndex + count - 1; \
            guardFmt( \
                endIndex < list->count, \
                "%s: End index (%zu) must be within range (list count: %zu)", \
                callerName, \
                endIndex, \
                list->count \
            ); \
        } \
    }

time_t safeTime(char const *callerDescription);

struct StringBuilder;
typedef struct StringBuilder * StringBuilder;
typedef struct StringBuilder const * ConstStringBuilder;

StringBuilder StringBuilder_create(void);
StringBuilder StringBuilder_fromChars(char const *value, size_t count);
StringBuilder StringBuilder_fromString(char const *value);
void StringBuilder_destroy(StringBuilder builder);

char const *StringBuilder_chars(ConstStringBuilder builder);
size_t StringBuilder_length(ConstStringBuilder builder);

void StringBuilder_appendChar(StringBuilder builder, char value);
void StringBuilder_appendChars(StringBuilder builder, char const *value, size_t count);
void StringBuilder_append(StringBuilder builder, char const *value);
void StringBuilder_appendFmt(StringBuilder builder, char const *valueFormat, ...);
void StringBuilder_appendFmtVA(StringBuilder builder, char const *valueFormat, va_list valueFormatArgs);
void StringBuilder_appendLine(StringBuilder builder, char const *value);
void StringBuilder_appendLineFmt(StringBuilder builder, char const *valueFormat, ...);
void StringBuilder_appendLineFmtVA(StringBuilder builder, char const *valueFormat, va_list valueFormatArgs);

void StringBuilder_insertChar(StringBuilder builder, size_t index, char value);
void StringBuilder_insertChars(StringBuilder builder, size_t index, char const *value, size_t count);
void StringBuilder_insert(StringBuilder builder, size_t index, char const *value);
void StringBuilder_insertFmt(StringBuilder builder, size_t index, char const *valueFormat, ...);
void StringBuilder_insertFmtVA(StringBuilder builder, size_t index, char const *valueFormat, va_list valueFormatArgs);

void StringBuilder_removeAt(StringBuilder builder, size_t index);
void StringBuilder_removeManyAt(StringBuilder builder, size_t startIndex, size_t count);

char *StringBuilder_toString(ConstStringBuilder builder);
char *StringBuilder_toStringAndDestroy(StringBuilder builder);

size_t safeSnprintf(
    char *buffer,
    size_t bufferLength,
    char const *callerDescription,
    char const *format,
    ...
);
size_t safeVsnprintf(
    char *buffer,
    size_t bufferLength,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);
size_t safeSprintf(
    char *buffer,
    char const *callerDescription,
    char const *format,
    ...
);
size_t safeVsprintf(
    char *buffer,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);

char *formatString(char const *format, ...);
char *formatStringVA(char const *format, va_list formatArgs);

int tryCreateSharedMemory(key_t key, size_t size, mode_t permissions);
int safeOpenSharedMemory(key_t key, size_t size, char const *callerDescription);
void safeDestroySharedMemory(int id, char const *callerDescription);

void *safeAttachSharedMemory(int id, char const *callerDescription);
void safeDetatchSharedMemory(void const *memory, char const *callerDescription);

sem_t *safeSemOpen(char const *name, char const *callerDescription);
sem_t *safeSemOpenOrCreate(
    char const *name,
    mode_t permissions,
    unsigned int initialValue,
    char const *callerDescription
);
sem_t *safeSemCreate(
    char const *name,
    mode_t permissions,
    unsigned int initialValue,
    char const *callerDescription
);
sem_t *tryCreateSemaphore(char const *name, mode_t permissions, unsigned int initialValue);

void safeSemClose(sem_t *semaphore, char const *callerDescription);

void safeSemWait(sem_t *semaphore, char const *callerDescription);
void safeSemPost(sem_t *semaphore, char const *callerDescription);

void initializeRandom(unsigned int seed);

int randomInt(int minInclusive, int maxExclusive);

pid_t safeFork(char const *callerDescription);

void safeExecvp(
    char const *filePath,
    char * const *argv,
    char const *callerDescription
);

pid_t safeWaitpid(
    pid_t processId,
    int *outStatusPtr,
    int options,
    char const *callerDescription
);

void safeCreatePipe(
    int *outReadFileDescriptor,
    int *outWriteFileDescriptor,
    char const *callerDescription
);

DECLARE_LIST(CharList, char)
DECLARE_LIST(StringList, char *)

FILE *safeFopen(char const *filePath, char const *modes, char const *callerDescription);
FILE *safeFdopen(int fileDescriptor, char const *modes, char const *callerDescription);

unsigned int safeFprintf(
    FILE *file,
    char const *callerDescription,
    char const *format,
    ...
);
unsigned int safeVfprintf(
    FILE *file,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);

bool safeFgetc(char *charPtr, FILE *file, char const *callerDescription);
bool safeFgets(char *buffer, size_t bufferLength, FILE *file, char const *callerDescription);

char *readFileLine(FILE *file);

char *readAllFileText(char const *filePath);

int safeFscanf(
    FILE *file,
    char const *callerDescription,
    char const *format,
    ...
);
int safeVfscanf(
    FILE *file,
    char const *format,
    va_list formatArgs,
    char const *callerDescription
);

bool scanFileExact(
    FILE *file,
    unsigned int expectedMatchCount,
    char const *format,
    ...
);
bool scanFileExactVA(
    FILE *file,
    unsigned int expectedMatchCount,
    char const *format,
    va_list formatArgs
);

void hw11Program2(
    int pipe1ReadFileDescriptor,
    int pipe2WriteFileDescriptor,
    char const *semaphore1Name,
    char const *semaphore2Name,
    key_t sharedMemoryKey
);

struct Hw11WordCounts {
    unsigned int type1;
    unsigned int type2;
};

/**
 * Run CSCI 451 HW11 program 2.
 */
void hw11Program2(
    int const pipe1ReadFileDescriptor,
    int const pipe2WriteFileDescriptor,
    char const * const semaphore1Name,
    char const * const semaphore2Name,
    key_t const sharedMemoryKey
) {
    guardNotNull(semaphore1Name, "semaphore1Name", "hw11Program2");
    guardNotNull(semaphore2Name, "semaphore2Name", "hw11Program2");

    FILE * const pipe1ReadFile = safeFdopen(pipe1ReadFileDescriptor, "r", "hw11Program2");
    FILE * const pipe2WriteFile = safeFdopen(pipe2WriteFileDescriptor, "w", "hw11Program2");
    sem_t * const semaphore1 = safeSemOpen(semaphore1Name, "hw11Program2");
    sem_t * const semaphore2 = safeSemOpen(semaphore2Name, "hw11Program2");
    int const sharedMemoryId = safeOpenSharedMemory(sharedMemoryKey, sizeof (struct Hw11WordCounts), "hw11Program2");
    struct Hw11WordCounts * const wordCounts = safeAttachSharedMemory(sharedMemoryId, "hw11Program2");

    unsigned int type1WordCount = 0;
    unsigned int type2WordCount = 0;

    safeSemWait(semaphore1, "hw11Program2");
    while (true) {
        char * const currentWord = readFileLine(pipe1ReadFile);
        if (currentWord == NULL) {
            break;
        }
        if (currentWord[0] == '\0') {
            free(currentWord);
            break;
        }

        StringBuilder const pigLatinWordBuilder = StringBuilder_fromString(currentWord);
        size_t const currentWordLength = StringBuilder_length(pigLatinWordBuilder);
        char const firstCharacter = currentWord[0];
        char const firstCharacterAsLower = (char)tolower(firstCharacter);
        char const lastCharacter = currentWord[currentWordLength - 1];
        if (
            firstCharacterAsLower == 'a'
            || firstCharacterAsLower == 'e'
            || firstCharacterAsLower == 'i'
            || firstCharacterAsLower == 'o'
            || firstCharacterAsLower == 'u'
            || firstCharacterAsLower == 'y'
        ) {
            type1WordCount += 1;

            size_t rayInsertPosition = (lastCharacter == ',' || lastCharacter == '.')
                ? currentWordLength - 1
                : currentWordLength;

            StringBuilder_insert(pigLatinWordBuilder, rayInsertPosition, "ray");
        } else {
            type2WordCount += 1;

            StringBuilder_removeAt(pigLatinWordBuilder, 0);

            size_t xayInsertPosition = (lastCharacter == ',' || lastCharacter == '.')
                ? currentWordLength - 2
                : currentWordLength - 1;

            StringBuilder_insertFmt(pigLatinWordBuilder, xayInsertPosition, "%cay", firstCharacter);
        }

        char * const pigLatinWord = StringBuilder_toStringAndDestroy(pigLatinWordBuilder);
        safeFprintf(pipe2WriteFile, "hw11Program2", "%s\n", pigLatinWord);
        free(pigLatinWord);

        free(currentWord);
    }
    safeSemPost(semaphore1, "hw11Program2");

    safeFprintf(pipe2WriteFile, "hw11Program2", "\n");
    *wordCounts = (struct Hw11WordCounts){
        .type1 = type1WordCount,
        .type2 = type2WordCount
    };
    safeSemPost(semaphore2, "hw11Program2");

    fclose(pipe1ReadFile);
    fclose(pipe2WriteFile);

    safeDetatchSharedMemory(wordCounts, "hw11Program2");
}

/**
 * Abort program execution after printing the specified error message to stderr.
 *
 * @param errorMessage The error message, not terminated by a newline.
 */
void abortWithError(char const * const errorMessage) {
    guardNotNull(errorMessage, "errorMessage", "abortWithError");

    fputs(errorMessage, stderr);
    fputc('\n', stderr);
    abort();
}

/**
 * Abort program execution after formatting and printing the specified error message to stderr.
 *
 * @param errorMessage The error message format (printf), not terminated by a newline.
 * @param ... The error message format arguments (printf).
 */
void abortWithErrorFmt(char const * const errorMessageFormat, ...) {
    va_list errorMessageFormatArgs;
    va_start(errorMessageFormatArgs, errorMessageFormat);
    abortWithErrorFmtVA(errorMessageFormat, errorMessageFormatArgs);
    va_end(errorMessageFormatArgs);
}

/**
 * Abort program execution after formatting and printing the specified error message to stderr.
 *
 * @param errorMessage The error message format (printf), not terminated by a newline.
 * @param errorMessageFormatArgs The error message format arguments (printf).
 */
void abortWithErrorFmtVA(char const * const errorMessageFormat, va_list errorMessageFormatArgs) {
    guardNotNull(errorMessageFormat, "errorMessageFormat", "abortWithErrorFmtVA");

    char * const errorMessage = formatStringVA(errorMessageFormat, errorMessageFormatArgs);
    abortWithError(errorMessage);
    free(errorMessage);
}

/**
 * Open the file using fopen. If the operation fails, abort the program with an error message.
 *
 * @param filePath The file path.
 * @param modes The fopen modes string.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The opened file.
 */
FILE *safeFopen(char const * const filePath, char const * const modes, char const * const callerDescription) {
    guardNotNull(filePath, "filePath", "safeFopen");
    guardNotNull(modes, "modes", "safeFopen");
    guardNotNull(callerDescription, "callerDescription", "safeFopen");

    FILE * const file = fopen(filePath, modes);
    if (file == NULL) {
        int const fopenErrorCode = errno;
        char const * const fopenErrorMessage = strerror(fopenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open file \"%s\" with modes \"%s\" using fopen (error code: %d; error message: \"%s\")",
            callerDescription,
            filePath,
            modes,
            fopenErrorCode,
            fopenErrorMessage
        );
        return NULL;
    }

    return file;
}

/**
 * Open the file using fdopen. If the operation fails, abort the program with an error message.
 *
 * @param fileDescriptor The file descriptor.
 * @param modes The fopen modes string.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The opened file.
 */
FILE *safeFdopen(int const fileDescriptor, char const * const modes, char const * const callerDescription) {
    guardNotNull(modes, "modes", "safeFdopen");
    guardNotNull(callerDescription, "callerDescription", "safeFdopen");

    FILE * const file = fdopen(fileDescriptor, modes);
    if (file == NULL) {
        int const fdopenErrorCode = errno;
        char const * const fdopenErrorMessage = strerror(fdopenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open file %d with modes \"%s\" using fdopen (error code: %d; error message: \"%s\")",
            callerDescription,
            fileDescriptor,
            modes,
            fdopenErrorCode,
            fdopenErrorMessage
        );
        return NULL;
    }

    return file;
}

/**
 * Print a formatted string to the given file. If the operation fails, abort the program with an error message.
 *
 * @param file The file.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param format The format (printf).
 * @param ... The format arguments (printf).
 *
 * @returns The number of charactes printed (no string terminator character).
 */
unsigned int safeFprintf(
    FILE * const file,
    char const * const callerDescription,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    unsigned int const printedCharCount = safeVfprintf(file, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return printedCharCount;
}

/**
 * Print a formatted string to the given file. If the operation fails, abort the program with an error message.
 *
 * @param file The file.
 * @param format The format (printf).
 * @param formatArgs The format arguments (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of charactes printed (no string terminator character).
 */
unsigned int safeVfprintf(
    FILE * const file,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(file, "file", "safeVfprintf");
    guardNotNull(format, "format", "safeVfprintf");
    guardNotNull(callerDescription, "callerDescription", "safeVfprintf");

    int const printedCharCount = vfprintf(file, format, formatArgs);
    if (printedCharCount < 0) {
        int const vfprintfErrorCode = errno;
        char const * const vfprintfErrorMessage = strerror(vfprintfErrorCode);

        abortWithErrorFmt(
            "%s: Failed to print format \"%s\" to file using vfprintf (error code: %d; error message: \"%s\")",
            callerDescription,
            format,
            vfprintfErrorCode,
            vfprintfErrorMessage
        );
        return (unsigned int)-1;
    }

    return (unsigned int)printedCharCount;
}

/**
 * Read a character from the given file. If the operation fails, abort the program with an error message.
 *
 * @param charPtr The location to store the read character.
 * @param file The file to read from.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns Whether the end-of-file was hit.
 */
bool safeFgetc(
    char * const charPtr,
    FILE * const file,
    char const * const callerDescription
) {
    guardNotNull(charPtr, "charPtr", "safeFgetc");
    guardNotNull(file, "file", "safeFgetc");
    guardNotNull(callerDescription, "callerDescription", "safeFgetc");

    int const fgetcResult = fgetc(file);
    if (fgetcResult == EOF) {
        bool const fgetcError = ferror(file);
        if (fgetcError) {
            int const fgetcErrorCode = errno;
            char const * const fgetcErrorMessage = strerror(fgetcErrorCode);

            abortWithErrorFmt(
                "%s: Failed to read char from file using fgetc (error code: %d; error message: \"%s\")",
                callerDescription,
                fgetcErrorCode,
                fgetcErrorMessage
            );
            return false;
        }

        // EOF
        return false;
    }

    *charPtr = (char)fgetcResult;
    return true;
}

/**
 * Read characters from the given file into the given buffer. Stop as soon as one of the following conditions has been
 * met: (A) `bufferLength - 1` characters have been read, (B) a newline is encountered, or (C) the end of the file is
 * reached. The string read into the buffer will end with a terminating character. If the operation fails, abort the
 * program with an error message.
 *
 * @param buffer The buffer into which to read the string.
 * @param bufferLength The length of the buffer.
 * @param file The file to read from.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns Whether unread characters remain.
 */
bool safeFgets(
    char * const buffer,
    size_t const bufferLength,
    FILE * const file,
    char const * const callerDescription
) {
    guardNotNull(buffer, "buffer", "safeFgets");
    guardNotNull(file, "file", "safeFgets");
    guardNotNull(callerDescription, "callerDescription", "safeFgets");

    char * const fgetsResult = fgets(buffer, (int)bufferLength, file);
    bool const fgetsError = ferror(file);
    if (fgetsError) {
        int const fgetsErrorCode = errno;
        char const * const fgetsErrorMessage = strerror(fgetsErrorCode);

        abortWithErrorFmt(
            "%s: Failed to read %zu chars from file using fgets (error code: %d; error message: \"%s\")",
            callerDescription,
            bufferLength,
            fgetsErrorCode,
            fgetsErrorMessage
        );
        return false;
    }

    if (fgetsResult == NULL || feof(file)) {
        return false;
    }

    return true;
}

/**
 * Read a line from the file. If the current file position is EOF, return null.
 *
 * @param file The file to read from.
 *
 * @returns The line (the caller is responsible for freeing this memory), or null if the current file position is EOF.
 */
char *readFileLine(FILE * const file) {
    guardNotNull(file, "file", "readFileLine");

    StringBuilder const lineBuilder = StringBuilder_create();

    bool atEof = true;
    char currentCharacter;
    while (safeFgetc(&currentCharacter, file, "readFileLine")) {
        atEof = false;

        if (currentCharacter == '\n') {
            break;
        }

        StringBuilder_appendChar(lineBuilder, currentCharacter);
    }

    if (atEof) {
        StringBuilder_destroy(lineBuilder);
        return NULL;
    }

    char * const line = StringBuilder_toStringAndDestroy(lineBuilder);
    return line;
}

/**
 * Open a text file, read all the text in the file into a string, and then close the file.
 *
 * @param filePath The path to the file.
 *
 * @returns A string containing all text in the file. The caller is responsible for freeing this memory.
 */
char *readAllFileText(char const * const filePath) {
    guardNotNull(filePath, "filePath", "readAllFileText");

    StringBuilder const fileTextBuilder = StringBuilder_create();

    FILE * const file = safeFopen(filePath, "r", "readAllFileText");
    char fgetsBuffer[100];
    while (safeFgets(fgetsBuffer, 100, file, "readAllFileText")) {
        StringBuilder_append(fileTextBuilder, fgetsBuffer);
    }
    fclose(file);

    char * const fileText = StringBuilder_toStringAndDestroy(fileTextBuilder);

    return fileText;
}

/**
 * Read values from the given file using the given format. Values are stored in the locations pointed to by formatArgs.
 * If the operation fails, abort the program with an error message.
 *
 * @param file The file.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param format The format (scanf).
 * @param ... The format arguments (scanf).
 *
 * @returns The number of input items successfully matched and assigned, which can be fewer than provided for, or even
 *          zero in the event of an early matching failure. EOF is returned if the end of input is reached before either
 *          the first successful conversion or a matching failure occurs.
 */
int safeFscanf(
    FILE * const file,
    char const * const callerDescription,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    int const matchCount = safeVfscanf(file, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return matchCount;
}

/**
 * Read values from the given file using the given format. Values are stored in the locations pointed to by formatArgs.
 * If the operation fails, abort the program with an error message.
 *
 * @param file The file.
 * @param format The format (scanf).
 * @param formatArgs The format arguments (scanf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of input items successfully matched and assigned, which can be fewer than provided for, or even
 *          zero in the event of an early matching failure. EOF is returned if the end of input is reached before either
 *          the first successful conversion or a matching failure occurs.
 */
int safeVfscanf(
    FILE * const file,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(file, "file", "safeVfscanf");
    guardNotNull(format, "format", "safeVfscanf");
    guardNotNull(callerDescription, "callerDescription", "safeVfscanf");

    int const matchCount = vfscanf(file, format, formatArgs);
    bool const vfscanfError = ferror(file);
    if (vfscanfError) {
        int const vfscanfErrorCode = errno;
        char const * const vfscanfErrorMessage = strerror(vfscanfErrorCode);

        abortWithErrorFmt(
            "%s: Failed to read format \"%s\" from file using vfscanf (error code: %d; error message: \"%s\")",
            callerDescription,
            format,
            vfscanfErrorCode,
            vfscanfErrorMessage
        );
        return -1;
    }

    return matchCount;
}

/**
 * Read values from the given file using the given format. Values are stored in the locations pointed to by formatArgs.
 * If the number of matched items does not match the expected count or if the operation fails, abort the program with an
 * error message.
 *
 * @param file The file.
 * @param expectedMatchCount The number of items in the format expected to be matched.
 * @param format The format (scanf).
 * @param ... The format arguments (scanf).
 *
 * @returns True if the format was scanned, or false if the end of the file was met.
 */
bool scanFileExact(
    FILE * const file,
    unsigned int const expectedMatchCount,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    bool const scanned = scanFileExactVA(file, expectedMatchCount, format, formatArgs);
    va_end(formatArgs);
    return scanned;
}

/**
 * Read values from the given file using the given format. Values are stored in the locations pointed to by formatArgs.
 * If the number of matched items does not match the expected count or if the operation fails, abort the program with an
 * error message.
 *
 * @param file The file.
 * @param expectedMatchCount The number of items in the format expected to be matched.
 * @param format The format (scanf).
 * @param formatArgs The format arguments (scanf).
 *
 * @returns True if the format was scanned, or false if the end of the file was met.
 */
bool scanFileExactVA(
    FILE * const file,
    unsigned int const expectedMatchCount,
    char const * const format,
    va_list formatArgs
) {
    guardNotNull(file, "file", "scanFileExactVA");
    guardNotNull(format, "format", "scanFileExactVA");

    int const matchCount = safeVfscanf(file, format, formatArgs, "scanFileExactVA");
    if (matchCount == EOF) {
        return false;
    }

    if ((unsigned int)matchCount != expectedMatchCount) {
        abortWithErrorFmt(
            "scanFileExactVA: Failed to parse exact format \"%s\" from file"
            " (expected match count: %u; actual match count: %d)",
            format,
            expectedMatchCount,
            matchCount
        );
        return false;
    }

    return true;
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessage The error message.
 */
void guard(bool const expression, char const * const errorMessage) {
    if (errorMessage == NULL) {
        abortWithError("guard: errorMessage must not be null");
        return;
    }

    if (expression) {
        return;
    }

    abortWithError(errorMessage);
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessageFormat The error message format (printf).
 * @param ... The error message format arguments (printf).
 */
void guardFmt(bool const expression, char const * const errorMessageFormat, ...) {
    va_list errorMessageFormatArgs;
    va_start(errorMessageFormatArgs, errorMessageFormat);
    guardFmtVA(expression, errorMessageFormat, errorMessageFormatArgs);
    va_end(errorMessageFormatArgs);
}

/**
 * Ensure that the given expression involving a parameter is true. If it is false, abort the program with an error
 * message.
 *
 * @param expression The expression to verify is true.
 * @param errorMessageFormat The error message format (printf).
 * @param errorMessageFormatArgs The error message format arguments (printf).
 */
void guardFmtVA(bool const expression, char const * const errorMessageFormat, va_list errorMessageFormatArgs) {
    if (errorMessageFormat == NULL) {
        abortWithError("guardFmtVA: errorMessageFormat must not be null");
        return;
    }

    if (expression) {
        return;
    }

    abortWithErrorFmtVA(errorMessageFormat, errorMessageFormatArgs);
}

/**
 * Ensure that the given object supplied by a parameter is not null. If it is null, abort the program with an error
 * message.
 *
 * @param object The object to verify is not null.
 * @param paramName The name of the parameter supplying the object.
 * @param callerName The name of the calling function.
 */
void guardNotNull(void const * const object, char const * const paramName, char const * const callerName) {
    guard(paramName != NULL, "guardNotNull: paramName must not be null");
    guard(callerName != NULL, "guardNotNull: callerName must not be null");

    guardFmt(object != NULL, "%s: %s must not be null", callerName, paramName);
}

DEFINE_LIST(CharList, char)
DEFINE_LIST(StringList, char *)

/**
 * Allocate memory of the given size using malloc. If the allocation fails, abort the program with an error message.
 *
 * @param size The size of the memory, in bytes.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The allocated memory.
 */
void *safeMalloc(size_t const size, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeMalloc");

    void * const memory = malloc(size);
    if (memory == NULL) {
        int const mallocErrorCode = errno;
        char const * const mallocErrorMessage = strerror(mallocErrorCode);

        abortWithErrorFmt(
            "%s: Failed to allocate %zu bytes of memory using malloc (error code: %d; error message: \"%s\")",
            callerDescription,
            size,
            mallocErrorCode,
            mallocErrorMessage
        );
        return NULL;
    }

    return memory;
}

/**
 * Resize the given memory using realloc. If the reallocation fails, abort the program with an error message.
 *
 * @param memory The existing memory, or null.
 * @param newSize The new size of the memory, in bytes.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The reallocated memory.
 */
void *safeRealloc(void * const memory, size_t const newSize, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeRealloc");

    void * const newMemory = realloc(memory, newSize);
    if (newMemory == NULL) {
        int const reallocErrorCode = errno;
        char const * const reallocErrorMessage = strerror(reallocErrorCode);

        abortWithErrorFmt(
            "%s: Failed to reallocate memory to %zu bytes using realloc (error code: %d; error message: \"%s\")",
            callerDescription,
            newSize,
            reallocErrorCode,
            reallocErrorMessage
        );
        return NULL;
    }

    return newMemory;
}

/**
 * Create a pipe. If the operation fails, abort the program with an error message.
 *
 * @param outReadFileDescriptor The address of where to store the file descriptor of the read end of the new pipe.
 * @param outWriteFileDescriptor The address of where to store the file descriptor of the write end of the new pipe.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeCreatePipe(
    int * const outReadFileDescriptor,
    int * const outWriteFileDescriptor,
    char const * const callerDescription
) {
    guardNotNull(outReadFileDescriptor, "outReadFileDescriptor", "safeCreatePipe");
    guardNotNull(outWriteFileDescriptor, "outWriteFileDescriptor", "safeCreatePipe");
    guardNotNull(callerDescription, "callerDescription", "safeCreatePipe");

    int fileDescriptors[2];
    int const pipeResult = pipe(fileDescriptors);
    if (pipeResult != 0) {
        int const pipeErrorCode = errno;
        char const * const pipeErrorMessage = strerror(pipeErrorCode);

        abortWithErrorFmt(
            "%s: Failed to create pipe using pipe (error code: %d; error message: \"%s\")",
            callerDescription,
            pipeErrorCode,
            pipeErrorMessage
        );
        return;
    }

    *outReadFileDescriptor = fileDescriptors[0];
    *outWriteFileDescriptor = fileDescriptors[1];
}

/**
 * Clone the calling process, creating an exact copy. If the operation fails, abort the program with an error message.
 *
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The child process ID to the parent process and 0 to the child process.
 */
pid_t safeFork(char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeFork");

    pid_t const forkResult = fork();
    if (forkResult == -1) {
        int const forkErrorCode = errno;
        char const * const forkErrorMessage = strerror(forkErrorCode);

        abortWithErrorFmt(
            "%s: Failed to fork process using fork (error code: %d; error message: \"%s\")",
            callerDescription,
            forkErrorCode,
            forkErrorMessage
        );
        return -1;
    }

    return forkResult;
}

/**
 * Replace the current process image with a new process image.
 *
 * @param filePath The path to the program file to execute. If this does not include a slash character, PATH will be
 *                 searched.
 * @param argv A null-terminated array of null-terminated strings that represents the argument list available to the new
 *             program. The first argument, by convention, should point to the filename associated with the file being
 *             executed.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeExecvp(
    char const * const filePath,
    char * const * const argv,
    char const * const callerDescription
) {
    guardNotNull(filePath, "filePath", "safeExecvp");
    guardNotNull(argv, "argv", "safeExecvp");
    guardNotNull(callerDescription, "callerDescription", "safeExecvp");

    int const execResult = execvp(filePath, argv);
    if (execResult == -1) {
        int const execErrorCode = errno;
        char const * const execErrorMessage = strerror(execErrorCode);

        abortWithErrorFmt(
            "%s: Failed to replace process with \"%s\" using execvp (error code: %d; error message: \"%s\")",
            callerDescription,
            filePath,
            execErrorCode,
            execErrorMessage
        );
    }
}

/**
 * Wait for the process specified by the given ID.
 *
 * @param processId The process ID.
 * @param outStatusPtr If not null and this function returns because the status of a child process is available, that
 *                     status will be stored here.
 * @param options waitpid options.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The process ID.
 */
pid_t safeWaitpid(
    pid_t const processId,
    int * const outStatusPtr,
    int const options,
    char const * const callerDescription
) {
    guardNotNull(callerDescription, "callerDescription", "safeWaitPid");

    pid_t const waitpidResult = waitpid(processId, outStatusPtr, options);
    if (waitpidResult == -1) {
        int const waitpidErrorCode = errno;
        char const * const waitpidErrorMessage = strerror(waitpidErrorCode);

        abortWithErrorFmt(
            "%s: Failed to wait for process using waitpid (error code: %d; error message: \"%s\")",
            callerDescription,
            waitpidErrorCode,
            waitpidErrorMessage
        );
        return -1;
    }

    return waitpidResult;
}

static bool randomInitialized = false;

static void ensureRandomInitialized(void);

/**
 * Initialize the random number generator using the given seed value. If this is never called, the random number
 * generator will automatically be initialized with the time it is first used.
 *
 * @param seed A number used to calculate a starting value for the pseudo-random number sequence.
 */
void initializeRandom(unsigned int const seed) {
    srand(seed);
    randomInitialized = true;
}

/**
 * Generate the next random integer from within the given range.
 *
 * @param minInclusive The inclusive lower bound of the random number returned.
 * @param maxExclusive The exclusive upper bound of the random number returned. maxExclusive must be greater than
 *                     minInclusive.
 *
 * @returns The random integer.
 */
int randomInt(int const minInclusive, int const maxExclusive) {
    guardFmt(
        maxExclusive > minInclusive,
        "randomInt: maxExclusive (%d) must be greater than minInclusive (%d)",
        maxExclusive,
        minInclusive
    );

    ensureRandomInitialized();
    return rand() % (maxExclusive - minInclusive) + minInclusive;
}

static void ensureRandomInitialized(void) {
    if (randomInitialized) {
        return;
    }

    initializeRandom((unsigned int)safeTime("ensureRandomInitialized"));
    randomInitialized = true;
}

/**
 * Open an existing semaphore with the given name. If the operation fails, abort the program with an error message.
 *
 * @param name The name of the existing semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The address of the existing semaphore.
 */
sem_t *safeSemOpen(char const * const name, char const * const callerDescription) {
    guardNotNull(name, "name", "safeSemOpen");
    guardNotNull(callerDescription, "callerDescription", "safeSemOpen");

    sem_t * const semaphore = sem_open(name, 0);
    if (semaphore == SEM_FAILED) {
        int const semOpenErrorCode = errno;
        char const * const semOpenErrorMessage = strerror(semOpenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open semaphore named %s using sem_open (error code: %d; error message: \"%s\")",
            callerDescription,
            name,
            semOpenErrorCode,
            semOpenErrorMessage
        );
        return NULL;
    }

    return semaphore;
}

/**
 * Create a new semaphore or open an existing semaphore with the given name. If the operation fails, abort the program
 * with an error message.
 *
 * @param name The name of the new or existing semaphore.
 * @param permissions The permissions to be placed on the new semaphore.
 * @param initialValue The initial value for the new semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The address of the new or existing semaphore.
 */
sem_t *safeSemOpenOrCreate(
    char const * const name,
    mode_t const permissions,
    unsigned int const initialValue,
    char const * const callerDescription
) {
    guardNotNull(name, "name", "safeSemOpenOrCreate");
    guardNotNull(callerDescription, "callerDescription", "safeSemOpenOrCreate");

    sem_t * const semaphore = sem_open(name, O_CREAT, permissions, initialValue);
    if (semaphore == SEM_FAILED) {
        int const semOpenErrorCode = errno;
        char const * const semOpenErrorMessage = strerror(semOpenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open or create semaphore named %s using sem_open (error code: %d; error message: \"%s\")",
            callerDescription,
            name,
            semOpenErrorCode,
            semOpenErrorMessage
        );
        return NULL;
    }

    return semaphore;
}

/**
 * Create a new semaphore with the given name. If the operation fails, abort the program with an error message.
 *
 * @param name The name of the new semaphore.
 * @param permissions The permissions to be placed on the new semaphore.
 * @param initialValue The initial value for the new semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The address of the new semaphore.
 */
sem_t *safeSemCreate(
    char const * const name,
    mode_t const permissions,
    unsigned int const initialValue,
    char const * const callerDescription
) {
    guardNotNull(name, "name", "safeSemCreate");
    guardNotNull(callerDescription, "callerDescription", "safeSemCreate");

    sem_t * const semaphore = sem_open(name, O_CREAT | O_EXCL, permissions, initialValue);
    if (semaphore == SEM_FAILED) {
        int const semOpenErrorCode = errno;
        char const * const semOpenErrorMessage = strerror(semOpenErrorCode);

        abortWithErrorFmt(
            "%s: Failed to create semaphore named %s using sem_open (error code: %d; error message: \"%s\")",
            callerDescription,
            name,
            semOpenErrorCode,
            semOpenErrorMessage
        );
        return NULL;
    }

    return semaphore;
}

/**
 * Create a new semaphore with the given name. If a semaphore with this name already exists, return null. If the
 * operation otherwise fails, abort the program with an error message.
 *
 * @param name The name of the new semaphore.
 * @param permissions The permissions to be placed on the new semaphore.
 * @param initialValue The initial value for the new semaphore.
 *
 * @returns The address of the new semaphore, or null if a semaphore with the given name already exists.
 */
sem_t *tryCreateSemaphore(char const * const name, mode_t const permissions, unsigned int const initialValue) {
    guardNotNull(name, "name", "tryCreateSemaphore");

    sem_t * const semaphore = sem_open(name, O_CREAT | O_EXCL, permissions, initialValue);
    if (semaphore == SEM_FAILED) {
        int const semOpenErrorCode = errno;
        if (semOpenErrorCode == EEXIST) {
            return NULL;
        }

        char const * const semOpenErrorMessage = strerror(semOpenErrorCode);

        abortWithErrorFmt(
            "tryCreateSemaphore: Failed to create semaphore named %s using sem_open (error code: %d; error message: \"%s\")",
            name,
            semOpenErrorCode,
            semOpenErrorMessage
        );
        return NULL;
    }

    return semaphore;
}

/**
 * Close the given named semaphore. If the operation fails, abort the program with an error message.
 *
 * @param semaphore The semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeSemClose(sem_t * const semaphore, char const * const callerDescription) {
    guardNotNull(semaphore, "semaphore", "tryCreateSemaphore");
    guardNotNull(callerDescription, "callerDescription", "tryCreateSemaphore");

    int const semCloseResult = sem_close(semaphore);
    if (semCloseResult != 0) {
        int const semCloseErrorCode = errno;
        char const * const semCloseErrorMessage = strerror(semCloseErrorCode);

        abortWithErrorFmt(
            "%s: Failed to close semaphore using sem_close (error code: %d; error message: \"%s\")",
            callerDescription,
            semCloseErrorCode,
            semCloseErrorMessage
        );
    }
}

/**
 * Decrement (lock) the given semaphore. If the semaphore's value is greater than zero, then the decrement proceeds, and
 * the function returns immediately. If the semaphore currently has the value zero, then the call blocks until either it
 * becomes possible to perform the decrement (i.e., the semaphore value rises above zero), or a signal handler
 * interrupts the call. If the operation fails, abort the program with an error message.
 *
 * @param semaphore The semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeSemWait(sem_t * const semaphore, char const * const callerDescription) {
    guardNotNull(semaphore, "semaphore", "safeSemWait");
    guardNotNull(callerDescription, "callerDescription", "safeSemWait");

    int const semWaitResult = sem_wait(semaphore);
    if (semWaitResult != 0) {
        int const semWaitErrorCode = errno;
        char const * const semWaitErrorMessage = strerror(semWaitErrorCode);

        abortWithErrorFmt(
            "%s: Failed to lock semaphore using sem_wait (error code: %d; error message: \"%s\")",
            callerDescription,
            semWaitErrorCode,
            semWaitErrorMessage
        );
    }
}

/**
 * Increment (unlock) the given semaphore. If the semaphore's value consequently becomes greater than zero, then another
 * process or thread blocked in a sem_wait call will be woken up and proceed to lock the semaphore. If the operation
 * fails, abort the program with an error message.
 *
 * @param semaphore The semaphore.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeSemPost(sem_t * const semaphore, char const * const callerDescription) {
    guardNotNull(semaphore, "semaphore", "safeSemPost");
    guardNotNull(callerDescription, "callerDescription", "safeSemPost");

    int const semPostResult = sem_post(semaphore);
    if (semPostResult != 0) {
        int const semPostErrorCode = errno;
        char const * const semPostErrorMessage = strerror(semPostErrorCode);

        abortWithErrorFmt(
            "%s: Failed to unlock semaphore using sem_post (error code: %d; error message: \"%s\")",
            callerDescription,
            semPostErrorCode,
            semPostErrorMessage
        );
    }
}

/**
 * Attempt to create shared memory with the given key and size. If the operation fails, return -1.
 *
 * @param key The key.
 * @param size The size.
 * @param permissions The permissions to be placed on the new shared memory.
 *
 * @returns The ID of the newly created shared memory, or -1 if the operation failed.
 */
int tryCreateSharedMemory(key_t const key, size_t const size, mode_t const permissions) {
    return shmget(key, size, IPC_CREAT | IPC_EXCL | (int)permissions);
}

/**
 * Open the shared memory specified by the given key. If the operation fails, abort the program with an error message.
 *
 * @param key The key.
 * @param size The size.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The ID of the shared memory.
 */
int safeOpenSharedMemory(key_t const key, size_t const size, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeOpenSharedMemory");

    int const shmgetResult = shmget(key, size, 0);
    if (shmgetResult == -1) {
        int const shmgetErrorCode = errno;
        char const * const shmgetErrorMessage = strerror(shmgetErrorCode);

        abortWithErrorFmt(
            "%s: Failed to open shared memory using shmget (error code: %d; error message: \"%s\")",
            callerDescription,
            shmgetErrorCode,
            shmgetErrorMessage
        );
        return -1;
    }

    return shmgetResult;
}

/**
 * Destroy the shared memory specified by the given ID. If the operation fails, abort the program with an error message.
 *
 * @param id The ID.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeDestroySharedMemory(int const id, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeDestroySharedMemory");

    int const shmctlResult = shmctl(id, IPC_RMID, NULL);
    if (shmctlResult == -1) {
        int const shmctlErrorCode = errno;
        char const * const shmctlErrorMessage = strerror(shmctlErrorCode);

        abortWithErrorFmt(
            "%s: Failed to destroy shared memory using shmctl (error code: %d; error message: \"%s\")",
            callerDescription,
            shmctlErrorCode,
            shmctlErrorMessage
        );
    }
}

/**
 * Attach the shared memory specified by the given ID. If the operation fails, abort the program with an error message.
 *
 * @param id The ID.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The address of the shared memory.
 */
void *safeAttachSharedMemory(int const id, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeAttachSharedMemory");

    void * const memory = shmat(id, NULL, 0);
    if (memory == (void *)-1) {
        int const shmatErrorCode = errno;
        char const * const shmatErrorMessage = strerror(shmatErrorCode);

        abortWithErrorFmt(
            "%s: Failed to attach shared memory using shmat (error code: %d; error message: \"%s\")",
            callerDescription,
            shmatErrorCode,
            shmatErrorMessage
        );
        return NULL;
    }

    return memory;
}

/**
 * Detach the shared memory specified by the given address. If the operation fails, abort the program with an error
 * message.
 *
 * @param memory The address.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 */
void safeDetatchSharedMemory(void const * const memory, char const * const callerDescription) {
    guardNotNull(callerDescription, "callerDescription", "safeDetatchSharedMemory");

    int const shmdtResult = shmdt(memory);
    if (shmdtResult == -1) {
        int const shmdtErrorCode = errno;
        char const * const shmdtErrorMessage = strerror(shmdtErrorCode);

        abortWithErrorFmt(
            "%s: Failed to detach shared memory using shmdt (error code: %d; error message: \"%s\")",
            callerDescription,
            shmdtErrorCode,
            shmdtErrorMessage
        );
    }
}

/**
 * If the given buffer is non-null, format the string into the buffer. If the buffer is null, simply calculate the
 * number of characters that would have been written if the buffer had been sufficiently large. If the operation fails,
 * abort the program with an error message.
 *
 * @param buffer The buffer into which to write, or null if only the formatted string length is desired.
 * @param bufferLength The length of the buffer, or 0 if the buffer is null.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param format The string format (printf).
 * @param ... The string format arguments (printf).
 *
 * @returns The number of characters that would have been written if the buffer had been sufficiently large, not
 *          counting the terminating null character.
 */
size_t safeSnprintf(
    char * const buffer,
    size_t const bufferLength,
    char const * const callerDescription,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    size_t const length = safeVsnprintf(buffer, bufferLength, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return length;
}

/**
 * If the given buffer is non-null, format the string into the buffer. If the buffer is null, simply calculate the
 * number of characters that would have been written if the buffer had been sufficiently large. If the operation fails,
 * abort the program with an error message.
 *
 * @param buffer The buffer into which to write, or null if only the formatted string length is desired.
 * @param bufferLength The length of the buffer, or 0 if the buffer is null.
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of characters that would have been written if the buffer had been sufficiently large, not
 *          counting the terminating null character.
 */
size_t safeVsnprintf(
    char * const buffer,
    size_t const bufferLength,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(format, "format", "safeVsnprintf");
    guardNotNull(callerDescription, "callerDescription", "safeVsnprintf");

    int const vsnprintfResult = vsnprintf(buffer, bufferLength, format, formatArgs);
    if (vsnprintfResult < 0) {
        abortWithErrorFmt(
            "%s: Failed to format string using vsnprintf (format: \"%s\"; result: %d)",
            callerDescription,
            format,
            vsnprintfResult
        );
        return (size_t)-1;
    }

    return (size_t)vsnprintfResult;
}

/**
 * Format the string into the buffer. If the operation fails, abort the program with an error message.
 *
 * @param buffer The buffer into which to write.
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 * @param format The string format (printf).
 * @param ... The string format arguments (printf).
 *
 * @returns The number of characters written.
 */
size_t safeSprintf(
    char * const buffer,
    char const * const callerDescription,
    char const * const format,
    ...
) {
    va_list formatArgs;
    va_start(formatArgs, format);
    size_t const length = safeVsprintf(buffer, format, formatArgs, callerDescription);
    va_end(formatArgs);
    return length;
}

/**
 * Format the string into the buffer. If the operation fails, abort the program with an error message.
 *
 * @param buffer The buffer into which to write.
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The number of characters written.
 */
size_t safeVsprintf(
    char * const buffer,
    char const * const format,
    va_list formatArgs,
    char const * const callerDescription
) {
    guardNotNull(buffer, "buffer", "safeVsprintf");
    guardNotNull(format, "format", "safeVsprintf");
    guardNotNull(callerDescription, "callerDescription", "safeVsprintf");

    int const vsprintfResult = vsprintf(buffer, format, formatArgs);
    if (vsprintfResult < 0) {
        abortWithErrorFmt(
            "%s: Failed to format string using vsprintf (format: \"%s\"; result: %d)",
            callerDescription,
            format,
            vsprintfResult
        );
        return (size_t)-1;
    }

    return (size_t)vsprintfResult;
}

/**
 * Create a string using the specified format and format args.
 *
 * @param format The string format (printf).
 * @param ... The string format arguments (printf).
 *
 * @returns The formatted string. The caller is responsible for freeing the memory.
 */
char *formatString(char const * const format, ...) {
    va_list formatArgs;
    va_start(formatArgs, format);
    char * const formattedString = formatStringVA(format, formatArgs);
    va_end(formatArgs);
    return formattedString;
}

/**
 * Create a string using the specified format and format args.
 *
 * @param format The string format (printf).
 * @param formatArgs The string format arguments (printf).
 *
 * @returns The formatted string. The caller is responsible for freeing the memory.
 */
char *formatStringVA(char const * const format, va_list formatArgs) {
    guardNotNull(format, "format", "formatStringVA");

    va_list formatArgsForVsprintf;
    va_copy(formatArgsForVsprintf, formatArgs);

    size_t const formattedStringLength = safeVsnprintf(NULL, 0, format, formatArgs, "formatStringVA");
    char * const formattedString = safeMalloc(sizeof *formattedString * (formattedStringLength + 1), "formatStringVA");

    safeVsprintf(formattedString, format, formatArgsForVsprintf, "formatStringVA");
    va_end(formatArgsForVsprintf);

    return formattedString;
}

/**
 * Represents a mutable string of characters with convenience methods for string manipulation.
 */
struct StringBuilder {
    CharList chars;
};

/**
 * Create an empty StringBuilder.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_create(void) {
    StringBuilder const builder = safeMalloc(sizeof *builder, "StringBuilder_create");
    builder->chars = CharList_create();
    return builder;
}

/**
 * Create a StringBuilder initialized with the given characters.
 *
 * @param value The characters.
 * @param count The number of characters.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_fromChars(char const * const value, size_t const count) {
    guardNotNull(value, "value", "StringBuilder_fromChars");

    StringBuilder const builder = safeMalloc(sizeof *builder, "StringBuilder_fromChars");
    builder->chars = CharList_fromItems(value, count);
    return builder;
}

/**
 * Create a StringBuilder initialized with the given string.
 *
 * @param value The string.
 *
 * @returns The newly allocated StringBuilder. The caller is responsible for freeing this memory.
 */
StringBuilder StringBuilder_fromString(char const * const value) {
    guardNotNull(value, "value", "StringBuilder_fromString");
    return StringBuilder_fromChars(value, strlen(value));
}

/**
 * Free the memory associated with the StringBuilder.
 *
 * @param builder The StringBuilder instance.
 */
void StringBuilder_destroy(StringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_destroy");

    CharList_destroy(builder->chars);
    free(builder);
}

/**
 * Get the characters that compose the current value.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns The current value as a character array. This array is not null-terminated.
 */
char const *StringBuilder_chars(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_chars");
    return CharList_items(builder->chars);
}

/**
 * Get the length of the current value.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns The string length of the current value.
 */
size_t StringBuilder_length(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_length");
    return CharList_count(builder->chars);
}

/**
 * Append the given character to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The character.
 */
void StringBuilder_appendChar(StringBuilder const builder, char const value) {
    guardNotNull(builder, "builder", "StringBuilder_appendChar");
    CharList_add(builder->chars, value);
}

/**
 * Append the given characters to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The characters.
 * @param count The number of characters.
 */
void StringBuilder_appendChars(StringBuilder const builder, char const * const value, size_t const count) {
    guardNotNull(builder, "builder", "StringBuilder_appendChars");
    guardNotNull(value, "value", "StringBuilder_appendChars");

    CharList_addMany(builder->chars, value, count);
}

/**
 * Append the given string to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The string.
 */
void StringBuilder_append(StringBuilder const builder, char const * const value) {
    guardNotNull(value, "value", "StringBuilder_append");
    StringBuilder_appendChars(builder, value, strlen(value));
}

/**
 * Append the string specified by the given format and format args to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_appendFmt(StringBuilder const builder, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_appendFmtVA(builder, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Append the string specified by the given format and format args to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_appendFmtVA(StringBuilder const builder, char const * const valueFormat, va_list valueFormatArgs) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_appendFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_append(builder, value);
    free(value);
}

/**
 * Append the given string, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param value The string.
 */
void StringBuilder_appendLine(StringBuilder const builder, char const * const value) {
    guardNotNull(builder, "builder", "StringBuilder_appendLine");
    guardNotNull(value, "value", "StringBuilder_appendLine");

    CharList_addMany(builder->chars, value, strlen(value));
    CharList_add(builder->chars, '\n');
}

/**
 * Append the string specified by the given format and format args, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_appendLineFmt(StringBuilder const builder, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_appendLineFmtVA(builder, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Append the string specified by the given format and format args, followed by a newline, to the current value.
 *
 * @param builder The StringBuilder instance.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_appendLineFmtVA(
    StringBuilder const builder,
    char const * const valueFormat,
    va_list valueFormatArgs
) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_appendLineFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_appendLine(builder, value);
    free(value);
}

/**
 * Insert the given character into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The character.
 */
void StringBuilder_insertChar(StringBuilder const builder, size_t const index, char const value) {
    guardNotNull(builder, "builder", "StringBuilder_insertChar");
    CharList_insert(builder->chars, index, value);
}

/**
 * Insert the given characters into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The characters.
 * @param count The number of characters.
 */
void StringBuilder_insertChars(
    StringBuilder const builder,
    size_t const index,
    char const * const value,
    size_t const count
) {
    guardNotNull(builder, "builder", "StringBuilder_insertChars");
    guardNotNull(value, "value", "StringBuilder_insertChars");

    CharList_insertMany(builder->chars, index, value, count);
}

/**
 * Insert the given string into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param value The string.
 */
void StringBuilder_insert(StringBuilder const builder, size_t const index, char const * const value) {
    guardNotNull(value, "value", "StringBuilder_insert");
    StringBuilder_insertChars(builder, index, value, strlen(value));
}

/**
 * Insert the string specified by the given format and format args into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param valueFormat The string format (printf).
 * @param ... The string format arguments (printf).
 */
void StringBuilder_insertFmt(StringBuilder const builder, size_t const index, char const * const valueFormat, ...) {
    va_list valueFormatArgs;
    va_start(valueFormatArgs, valueFormat);
    StringBuilder_insertFmtVA(builder, index, valueFormat, valueFormatArgs);
    va_end(valueFormatArgs);
}

/**
 * Insert the string specified by the given format and format args into the current value at the given index.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 * @param valueFormat The string format (printf).
 * @param valueFormatArgs The string format arguments (printf).
 */
void StringBuilder_insertFmtVA(
    StringBuilder const builder,
    size_t const index,
    char const * const valueFormat,
    va_list valueFormatArgs
) {
    guardNotNull(valueFormat, "valueFormat", "StringBuilder_insertFmtVA");

    char * const value = formatStringVA(valueFormat, valueFormatArgs);
    StringBuilder_insert(builder, index, value);
    free(value);
}

/**
 * Remove the character at the given index from the current value.
 *
 * @param builder The StringBuilder instance.
 * @param index The index.
 */
void StringBuilder_removeAt(StringBuilder const builder, size_t const index) {
    guardNotNull(builder, "builder", "StringBuilder_removeAt");
    CharList_removeAt(builder->chars, index);
}

/**
 * Remove a series of characters starting at the given index from the current value.
 *
 * @param builder The StringBuilder instance.
 * @param startIndex The index at which to begin removal.
 * @param count The number of characters to remove.
 */
void StringBuilder_removeManyAt(StringBuilder const builder, size_t const startIndex, size_t const count) {
    guardNotNull(builder, "builder", "StringBuilder_removeManyAt");
    CharList_removeManyAt(builder->chars, startIndex, count);
}

/**
 * Convert the current value to a string.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns A newly allocated string containing the value. The caller is responsible for freeing this memory.
 */
char *StringBuilder_toString(ConstStringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_toString");

    size_t const length = CharList_count(builder->chars);
    char * const value = safeMalloc(sizeof *value * (length + 1), "StringBuilder_toString");
    CharList_fillArray(builder->chars, value, 0, length);
    value[length] = '\0';
    return value;
}

/**
 * Convert the current value to a string, then destroy the StringBuilder.
 *
 * @param builder The StringBuilder instance.
 *
 * @returns A newly allocated string containing the value. The caller is responsible for freeing this memory.
 */
char *StringBuilder_toStringAndDestroy(StringBuilder const builder) {
    guardNotNull(builder, "builder", "StringBuilder_toStringAndDestroy");

    char * const valueString = StringBuilder_toString(builder);
    StringBuilder_destroy(builder);
    return valueString;
}

/**
 * Get the current time. If the operation fails, abort the program with an error message.
 *
 * @param callerDescription A description of the caller to be included in the error message. This could be the name of
 *                          the calling function, plus extra information if useful.
 *
 * @returns The current time.
 */
time_t safeTime(char const * const callerDescription) {
    time_t const timeResult = time(NULL);
    if (timeResult == -1) {
        int const timeErrorCode = errno;
        char const * const timeErrorMessage = strerror(timeErrorCode);

        abortWithErrorFmt(
            "%s: Failed to get current time using time (error code: %d; error message: \"%s\")",
            callerDescription,
            timeErrorCode,
            timeErrorMessage
        );
        return -1;
    }

    return timeResult;
}

/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW11 program 2
 */

int main(int const argc, char ** const argv) {
    if (argc != 6) {
        fprintf(stderr, "Error: expected 5 arguments, got %d\n", argc - 1);
        return EXIT_FAILURE;
    }

    char * const pipe1ReadFileDescriptorString = argv[1];
    char * const pipe2WriteFileDescriptorString = argv[2];
    char * const semaphore1Name = argv[3];
    char * const semaphore2Name = argv[4];
    char * const sharedMemoryKeyString = argv[5];

    int pipe1ReadFileDescriptor = (int)strtol(pipe1ReadFileDescriptorString, NULL, 10);
    int pipe2WriteFileDescriptor = (int)strtol(pipe2WriteFileDescriptorString, NULL, 10);
    key_t sharedMemoryKey = (int)strtol(sharedMemoryKeyString, NULL, 10);

    hw11Program2(
        pipe1ReadFileDescriptor,
        pipe2WriteFileDescriptor,
        semaphore1Name,
        semaphore2Name,
        sharedMemoryKey
    );
    return EXIT_SUCCESS;
}

