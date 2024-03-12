//
// Created by Илья Белозеров on 05.02.2024.
//

#ifndef SYSLABS_MERGESORT_H
#define SYSLABS_MERGESORT_H
#include <stddef.h>
#include "libcoro.h"

int
mergesort(void *array, size_t elements, size_t element_size, int (*comparator)(const void *, const void *));

void
recursion(void *array, int left, int right, size_t element_size, int (*comparator)(const void *, const void *));

int
int_cmp(const void *a, const void *b);

#endif //SYSLABS_MERGESORT_H
