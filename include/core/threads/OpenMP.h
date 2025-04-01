#ifndef OPENMP_HELPER_H
#define OPENMP_HELPER_H

#include <omp.h>
#include <iostream>

struct ThreadBudget {
    int ChunkGenerationThreads;
    int MeshGenerationThreads;
    int OtherThreads;
};

void setThreadBudget(ThreadBudget& threadBudget) {

    #ifdef _OPENMP
        fprintf(stderr, "OpenMP is supported -- version = %d\n", _OPENMP);
    #else
        fprintf(stderr, "No OpenMP support!\n");
    #endif
    
        omp_set_dynamic(0);
    
        int numThreads = omp_get_max_threads();
        omp_set_num_threads(numThreads);
    
        if (numThreads >= 8) {
            threadBudget = {6, 2, 0};
        } else if (numThreads >= 6) {
            threadBudget = {4, 2, 0};
        } else if (numThreads >= 4) {
            threadBudget = {2, 1, 1};
        } else {
            threadBudget = {1, 1, 0};
        }
    }

#endif
