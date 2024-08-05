/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/MPIEnv.hpp>
#include "MPIEnvImpl.hpp"

namespace bedrock {

#ifdef ENABLE_MPI
bool   MPIEnvImpl::s_initialized_mpi = false;
size_t MPIEnvImpl::s_initiaze_mpi_count = 0;
#endif

MPIEnv::MPIEnv()
: self{std::make_shared<MPIEnvImpl>()} {}

MPIEnv::~MPIEnv() = default;

bool MPIEnv::isEnabled() const {
#ifdef ENABLE_MPI
    return true;
#else
    return false;
#endif
}

int MPIEnv::globalSize() const {
#ifdef ENABLE_MPI
    int s;
    MPI_Comm_size(MPI_COMM_WORLD, &s);
    return s;
#else
    throw Exception("Cannot get size of MPI_COMM_WORLD in a non-MPI deployment");
#endif
}

int MPIEnv::globalRank() const {
#ifdef ENABLE_MPI
    int r;
    MPI_Comm_rank(MPI_COMM_WORLD, &r);
    return r;
#else
    throw Exception("Cannot get rank of process in a non-MPI deployment");
#endif
}

const std::string& MPIEnv::addressOfRank(int rank) const {
    if(rank < 0 || rank >= globalSize()) {
        throw Exception{"Requesting address of an invalid rank ({})", rank};
    }
    static const std::string uninitialized = "<uninitialized>";
    if(self->m_addresses.empty()) return uninitialized;
    return self->m_addresses[rank];
}


} // namespace bedrock
