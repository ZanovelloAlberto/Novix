#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef void* type_t;

typedef bool (*criteria)(type_t, type_t);

typedef struct
{
    type_t *array;
    uint32_t size;
    uint32_t max_size;
    criteria criteria_function;
}ordered_array;

bool standard_criteria(type_t a, type_t b);

ordered_array create_static_array(void* addr, uint32_t max_size, criteria function);
ordered_array create_dynamic_array(uint32_t max_size, criteria function);
uint32_t insert_ordered_array(type_t item, ordered_array *array);
type_t lookup_ordered_array(uint32_t index, ordered_array *array);
void remove_ordered_array(uint32_t index, ordered_array *array);
uint32_t getIndex_ordered_array(type_t item, ordered_array *array);
