//
// Created by Илья Белозеров on 05.02.2024.
//
#include "mergesort.h"
#include <stdlib.h>
#include "assert.h"

void *
my_memcpy(void* dest, const void* src, size_t n)
{
    char* char_dest = (char*)dest;
    const char* char_src = (const char*)src;

    for (size_t i = 0; i < n; i++) {char_dest[i] = char_src[i];}

    return dest;
}


void
merge(void *array, int left, int center, int right, size_t element_size,
           int (*comparator)(const void *, const void *))
{

    int left_size = center - left + 1;
    int right_size = right - center;
    int i = 0, j = 0, k = left;

    void* left_arr = malloc(left_size * element_size);
    void* right_arr = malloc(right_size * element_size);

    assert(left_size);
    assert(right_size);

    my_memcpy(left_arr, array + (left * element_size), left_size * element_size);
    my_memcpy(right_arr, array + ((center + 1) * element_size), right_size * element_size);

    while (i < left_size && j < right_size) {
        if (comparator(left_arr + (i * element_size), right_arr + (j * element_size)) <= 0) {
            my_memcpy(array + (k * element_size), left_arr + (i * element_size), element_size);
            i++;
        } else {
            my_memcpy(array + (k * element_size), right_arr + (j * element_size), element_size);
            j++;
        }
        k++;
    }

    while (i < left_size) {
        my_memcpy(array + (k * element_size), left_arr + (i * element_size), element_size);
        i++;
        k++;
    }

    while (j < right_size) {
        my_memcpy(array + (k * element_size), right_arr + (j * element_size), element_size);
        j++;
        k++;
    }

    free(left_arr);
    free(right_arr);

}


void
recursion(void *array, int left, int right, size_t element_size,
               int (*comparator)(const void *, const void *))
{
    if (left < right) {
        int center = left + (right - left) / 2;

        recursion(array, left, center, element_size, comparator);
        recursion(array, center + 1, right, element_size, comparator);

        merge(array, left, center, right, element_size, comparator);

    }
}


int
mergesort(void *array, size_t elements, size_t element_size,
              int (*comparator)(const void *, const void *))
{
    recursion(array, 0, elements - 1, element_size, comparator);
    return 0;
}

int
int_cmp(const void *a, const void *b)
{
    return *(int*)a - *(int*)b;
}
