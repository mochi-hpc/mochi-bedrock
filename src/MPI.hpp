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
#include <bedrock/Exception.hpp>

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

    bool enabled() const {
#ifdef ENABLE_MPI
        return true;
#else
        return false;
#endif
    }

    int size() const {
#ifdef ENABLE_MPI
        int s;
        MPI_Comm_size(MPI_COMM_WORLD, &s);
        return s;
#else
        throw Exception("Cannot get size of MPI_COMM_WORLD in a non-MPI deployment");
#endif
    }

    int rank() const {
#ifdef ENABLE_MPI
        int r;
        MPI_Comm_rank(MPI_COMM_WORLD, &r);
        return r;
#else
        throw Exception("Cannot get rank of process in a non-MPI deployment");
#endif
    }
};

} // namespace bedrock

#endif
