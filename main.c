#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

static const long Num_To_Add = 1000000000;
static const double Scale = 10.0 / RAND_MAX;

long add_serial(const char *numbers) {
    long sum = 0;
    for (long i = 0; i < Num_To_Add; i++) {
        sum += numbers[i];
    }
    return sum;
}

long add_parallel(const char *numbers) {
    long sum = 0;

    int num_max_threads = omp_get_max_threads();
    long size_per_threads = Num_To_Add / num_max_threads;
    long *thread_results = malloc(sizeof(long) * num_max_threads);
#pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        long my_sum = 0;
        long my_first_i = thread_id * size_per_threads;
        long my_last_i = my_first_i + size_per_threads;

        for (my_first_i; my_first_i < my_last_i; my_first_i++){
            my_sum += numbers[my_first_i];
        }

        thread_results[thread_id] = my_sum;
#pragma omp barrier
        if (!(num_max_threads & (num_max_threads - 1))){
            int multiplier = 2;

            for(int i = 0; i< log2(num_max_threads); i++){
                for (int j = 0; j < num_max_threads; j += multiplier){
                    if (thread_id == j){
                        thread_results[j] += thread_results[j + (multiplier / 2)];
                    }
                }
                multiplier *= 2;
#pragma omp barrier
            }

            if (!thread_id){
                sum = thread_results[thread_id];
            }
        } else {
#pragma omp atomic
            sum += my_sum;
        }
    }

    return sum;
}
int main() {
    char *numbers = malloc(sizeof(long) * Num_To_Add);

    long chunk_size = Num_To_Add / omp_get_max_threads();
#pragma omp parallel num_threads(omp_get_max_threads())
    {
        int p = omp_get_thread_num();
        unsigned int seed = (unsigned int) time(NULL) + (unsigned int) p;
        long chunk_start = p * chunk_size;
        long chunk_end = chunk_start + chunk_size;
        for (long i = chunk_start; i < chunk_end; i++) {
            numbers[i] = (char) (rand_r(&seed) * Scale);
        }
    }

    struct timeval start, end;

    printf("Timing sequential...\n");
    gettimeofday(&start, NULL);
    long sum_s = add_serial(numbers);
    gettimeofday(&end, NULL);
    printf("Took %f seconds\n\n", end.tv_sec - start.tv_sec + (double) (end.tv_usec - start.tv_usec) / 1000000);

    printf("Timing parallel...\n");
    gettimeofday(&start, NULL);
    long sum_p = add_parallel(numbers);
    gettimeofday(&end, NULL);
    printf("Took %f seconds\n\n", end.tv_sec - start.tv_sec + (double) (end.tv_usec - start.tv_usec) / 1000000);

    printf("Sum serial: %ld\nSum parallel: %ld", sum_s, sum_p);

    free(numbers);
    return 0;
}
