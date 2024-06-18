/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MPI_H
#define __BEDROCK_MPI_H

#include <iostream>
#ifdef ENABLE_MPI
#include <mpi.h>
#endif

namespace bedrock {

class MPI {

#ifdef ENABLE_MPI
    static bool   s_initialized_mpi;
    static size_t s_initiaze_mpi_count;
#endif

  public:

    MPI() {
#ifdef ENABLE_MPI
        int mpi_is_initialized;
        MPI_Initialized(&mpi_is_initialized);
        if(!mpi_is_initialized) {
            MPI_Init(nullptr, nullptr);
            s_initialized_mpi = true;
        }
        if(s_initialized_mpi)
            s_initiaze_mpi_count += 1;
#endif
    }

    ~MPI() {
#ifdef ENABLE_MPI
        if(s_initialized_mpi) {
            s_initiaze_mpi_count -= 1;
            if(s_initiaze_mpi_count == 0) {
                //MPI_Finalize();
            }
            s_initialized_mpi = false;
        }
#endif
    }
};

} // namespace bedrock

#endif
