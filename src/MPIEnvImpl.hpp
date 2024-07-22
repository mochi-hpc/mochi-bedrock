/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MPI_IMPL_H
#define __BEDROCK_MPI_IMPL_H

#include <iostream>
#ifdef ENABLE_MPI
#include <mpi.h>
#endif
#include <bedrock/MPIEnv.hpp>
#include <bedrock/Exception.hpp>

namespace bedrock {

struct MPIEnvImpl {

#ifdef ENABLE_MPI
    static bool   s_initialized_mpi;
    static size_t s_initiaze_mpi_count;
#endif

    std::vector<std::string> m_addresses;

  public:

    MPIEnvImpl() {
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

    ~MPIEnvImpl() {
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

    void exchangeAddresses(const std::string& myaddress) {
#ifdef ENABLE_MPI
        m_addresses.clear();
        std::string addr = myaddress;
        int size = addr.size()+1;
        int max_size = size;
        int num_procs;
        MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
        MPI_Allreduce(&size, &max_size, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        addr.resize(max_size, '\0');
        std::vector<char> allAddresses(max_size*num_procs);
        MPI_Allgather(addr.data(), addr.size(), MPI_CHAR,
                      allAddresses.data(), addr.size(), MPI_CHAR, MPI_COMM_WORLD);
        m_addresses.reserve(num_procs);
        for(int i = 0; i < num_procs; ++i) {
            m_addresses.emplace_back(allAddresses.data() + i*max_size);
        }
#else
        (void)myaddress;
#endif
    }
};

} // namespace bedrock

#endif
