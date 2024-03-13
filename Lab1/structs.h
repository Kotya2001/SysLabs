//
// Created by Илья Белозеров on 11.03.2024.
//

#ifndef SYSLABS_UTILS_H
#define SYSLABS_UTILS_H

#pragma once

#include <time.h>

struct
array_t
{
    int size;
    int *array;
};

struct
filenames_t
{
    int count;
    int index;
    char **file_names;
};

//struct
//coroutine_times_t
//{
//    struct timespec start_yield, end_yield;
//    long long time_sleep = 0;
//
//};

#endif //SYSLABS_UTILS_H
