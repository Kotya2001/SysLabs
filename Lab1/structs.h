//
// Created by Илья Белозеров on 11.03.2024.
//

#ifndef SYSLABS_UTILS_H
#define SYSLABS_UTILS_H

#pragma once

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

#endif //SYSLABS_UTILS_H
