/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "MPI.hpp"

namespace bedrock {

#ifdef ENABLE_MPI
bool   MPI::s_initialized_mpi;
size_t MPI::s_initiaze_mpi_count;
#endif

} // namespace bedrock
