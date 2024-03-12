

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"
#include "structs.h"
#include "mergesort.h"
#include <limits.h>
#include <time.h>


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


struct my_context {
    int i;
    char* name;
    int* total_numbers_count;
    struct filenames_t *filenames_t;
    struct array_t **array_t;
};

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
my_context_delete(struct my_context *ctx)
{
    free(ctx->name);
    free(ctx);
}

static int
coroutine_mergesort_file(void *context)
{

    struct coro *this = coro_this();
    struct my_context *ctx = context;
    int status;

    struct timespec start, end;
    long long time;
    long long time_sleep = 0;

    printf("Started coroutine %s\n", ctx->name);

    clock_gettime(CLOCK_MONOTONIC, &start);

    while (ctx->filenames_t->index < ctx->filenames_t->count) {

        struct timespec start_yield, end_yield;
        double time_yield;

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

        mergesort(ctx->array_t[index]->array, numbers_count, sizeof(int), int_cmp);

        clock_gettime(CLOCK_MONOTONIC, &start_yield);
        coro_yield();
        clock_gettime(CLOCK_MONOTONIC, &end_yield);

        time_yield = (end_yield.tv_sec - start_yield.tv_sec) * 1000000LL + (end_yield.tv_nsec - start_yield.tv_nsec) / 1e3;
        time_sleep += time_yield;


    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_nsec - start.tv_nsec) / 1e3;

    long long int switch_count = coro_switch_count(this);
    printf("%s: количество переключений - %lld, время выполнения: %lld\n", ctx->name, switch_count, time - time_sleep);

    status = coro_status(this);

    my_context_delete(ctx);
    /* This will be returned from coro_status(). */
    return status;
}



int
main(int argc, char **argv)
{
    struct timespec start, end;
    long long time;
    int N = 5;

    clock_gettime(CLOCK_MONOTONIC, &start);

    int n_files = argc - 1;
    int total_numbers_count = 0;

    struct array_t **array_s = malloc(n_files * sizeof(struct array_t*));
    struct filenames_t filenames_s = {n_files, 0, &argv[1]};


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
    int count_index = 0;

    while (1){
        min = INT_MAX;

        for (int i = 0; i < n_files; ++i) {
            if (result[i] < array_s[i]->size){
                int curr_elem = array_s[i]->array[result[i]];
                if (curr_elem < min){
                    min = curr_elem;
                    array_index_min = i;
                }
            }
        }
        fprintf(output_file, "%d ", min);

        result[array_index_min] += 1;

        count_index++;
        if (count_index == total_numbers_count){
            break;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_nsec - start.tv_nsec) / 1e3;


    for (int i = 0; i < n_files; ++i){
        free(array_s[i]->array);
        free(array_s[i]);
    }

    free(result);
    free(array_s);
    fclose(output_file);

    printf("Время выполнения программы: %lld микросекунд\n", time);

    return 0;

}