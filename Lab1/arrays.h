//
// Created by Илья Белозеров on 05.02.2024.
//

#ifndef SYSLABS_ARRAYS_H
#define SYSLABS_ARRAYS_H

#pragma once

struct array_t {
    int size;
    int* array;
};

struct filenames_t {
    int count;
    int index;
    char** file_names;
};

#endif //SYSLABS_ARRAYS_H
