#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"
#include <limits.h>
#include <time.h>
#include "assert.h"


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


struct
my_context
{
    int i;
    char* name;
    int* total_numbers_count;
    struct filenames_t *filenames_t;
    struct array_t **array_t;
    int sec_start;
    int nsec_start;
    int sec_finish;
    int nsec_finish;
    int sec_total;
    int nsec_total;

};


int
total_numbers_in_file(FILE *file)
{
    int count = 0;
    int number;
    while (fscanf(file, "%d", &number) == 1) {count++;}
    fseek(file, 0, SEEK_SET);
    return count;
}

void
create_array(FILE *file, int *array)
{
    int i = 0;
    while (fscanf(file, "%d", &array[i]) == 1) {i++;}
    fseek(file, 0, SEEK_SET);
}

static struct my_context *
my_context_new(int i, char *name, int *total_numbers_count,
               struct filenames_t *filenames_t,
               struct array_t **array_t)
{
    struct my_context *ctx = malloc(sizeof(*ctx));
    ctx->i = i;
    ctx->name = strdup(name);
    ctx->filenames_t = filenames_t;
    ctx->total_numbers_count = total_numbers_count;
    ctx->array_t = array_t;
    return ctx;
}

static void
timer(struct my_context *ctx, char *command)
{
    struct timespec time;
    if (strcmp("start", command) == 0) {
        clock_gettime(CLOCK_MONOTONIC, &time);
        ctx->sec_start = time.tv_sec;
        ctx->nsec_start = time.tv_nsec;
    } else if (strcmp("stop", command) == 0) {
        clock_gettime(CLOCK_MONOTONIC, &time);
        ctx->sec_finish = time.tv_sec;
        ctx->nsec_finish = time.tv_nsec;
    } else {
        exit(EXIT_FAILURE);
    }
}

static void
calc_time(struct my_context *ctx)
{
    ctx->sec_total += ctx->sec_finish - ctx->sec_start;
    if (ctx->nsec_finish - ctx->nsec_start < 0) {
        ctx->nsec_total += 1000000000 + ctx->nsec_finish - ctx->nsec_start;
        ctx->sec_total--;
    } else {
        ctx->nsec_total += ctx->nsec_finish - ctx->nsec_start;
    }
}

static void
my_context_delete(struct my_context *ctx)
{
    free(ctx->name);
    free(ctx);
}


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
          int (*comparator)(const void *, const void *),
          struct my_context *ctx)
{
    if (left < right) {

        int center = left + (right - left) / 2;

        recursion(array, left, center, element_size, comparator, ctx);
        recursion(array, center + 1, right, element_size, comparator, ctx);

        merge(array, left, center, right, element_size, comparator);

        timer(ctx, "stop");
        calc_time(ctx);
        coro_yield();
        timer(ctx, "start");

    }

}


int
mysort(void *array, size_t elements, size_t element_size,
       int (*comparator)(const void *, const void *),
       struct my_context *ctx)
{
    recursion(array, 0, elements - 1, element_size, comparator, ctx);
    return 0;
}

int
int_cmp(const void *a, const void *b)
{
    return *(int*)a - *(int*)b;
}

static int
coroutine_mergesort_file(void *context)
{

    struct coro *this = coro_this();
    struct my_context *ctx = context;
    int status;

    timer(ctx, "start");

    printf("Started coroutine %s\n", ctx->name);

    while (ctx->filenames_t->index < ctx->filenames_t->count) {

        int index = ctx->filenames_t->index;
        char* file_name = ctx->filenames_t->file_names[index];

        printf("Coroutine %s sorting file %s...\n", ctx->name, file_name);

        FILE* file = fopen(file_name, "r");
        if (!file) {
            my_context_delete(ctx);
            return 0;
        }

        ctx->filenames_t->index += 1;

        int numbers_count = total_numbers_in_file(file);
        *ctx->total_numbers_count += numbers_count;

        ctx->array_t[index] = malloc(sizeof(struct array_t));
        ctx->array_t[index]->array = malloc(numbers_count * sizeof(int));
        ctx->array_t[index]->size = numbers_count;
        create_array(file, ctx->array_t[index]->array);
        fclose(file);

        mysort(ctx->array_t[index]->array, numbers_count, sizeof(int), int_cmp, ctx);


    }

    timer(ctx, "stop");
    calc_time(ctx);

    long long int switch_count = coro_switch_count(this);
    printf("%s: количество переключений - %lld, время выполнения: %d\n", ctx->name, switch_count,
           ctx->sec_total * 1000000  + ctx->nsec_total / 1000);

    status = coro_status(this);

    my_context_delete(ctx);
    return status;
}



int
main(int argc, char **argv)
{
    struct timespec start, end;
    int time;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int n_files = argc - 2;
    int total_numbers_count = 0;
    int N = atoi(argv[1]);

    struct array_t **array_s = malloc(n_files * sizeof(struct array_t*));
    struct filenames_t filenames_s = {n_files, 0, &argv[2]};


    coro_sched_init();
    /* Start several coroutines. */
    for (int i = 0; i < N; i++) {
        char name[16];
        sprintf(name, "coro_%d", i);

        coro_new(coroutine_mergesort_file,
                 my_context_new(i, name, &total_numbers_count, &filenames_s, array_s));

    }

    /* Wait for all the coroutines to end. */
    struct coro *c;
    while ((c = coro_sched_wait()) != NULL) {
        if (c != NULL){
            printf("Finished with status %d\n", coro_status(c));
            coro_delete(c);
        }
    }

    FILE *output_file = fopen("out.txt", "w");
    if (output_file == NULL) { exit(EXIT_FAILURE); }

    int *result = (int *)calloc(n_files, sizeof(int));

    int min = INT_MAX;
    int array_index_min = 0;

    for (int i = 0; i <= total_numbers_count; i++) {
        min = INT_MAX;

        for (int j = 0; j < n_files; j++) {
            if (result[j] < array_s[j]->size){
                int curr_elem = array_s[j]->array[result[j]];
                if (curr_elem < min){
                    min = curr_elem;
                    array_index_min = j;
                }
            }
        }
        fprintf(output_file, "%d ", min);
        result[array_index_min]++;
    }


    for (int i = 0; i < n_files; i++){
        free(array_s[i]->array);
        free(array_s[i]);
    }

    free(result);
    free(array_s);
    fclose(output_file);

    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;

    printf("Время выполнения программы: %d микросекунд\n", time);
    return 0;

}