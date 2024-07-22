/*
 * (C) 2024 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MPI_H
#define __BEDROCK_MPI_H

#include <bedrock/Exception.hpp>
#include <thallium.hpp>

namespace bedrock {

class DependencyFinder;
class Server;
struct MPIEnvImpl;

class MPIEnv {

    friend class Server;
    friend class DependencyFinder;

  public:

    MPIEnv(const MPIEnv&) = default;
    MPIEnv(MPIEnv&&) = default;
    MPIEnv& operator=(const MPIEnv&) = default;
    MPIEnv& operator=(MPIEnv&&) = default;

    /**
     * @brief The last call to the destructor will finalize MPI if it has been
     * initialized by Bedrock.
     */
    ~MPIEnv();

    /**
     * @brief Returns true if Bedrock was built with MPI support.
     * If it hasn't, all the methods bellow will throw an Exception.
     *
     * @param engine Thallium engine.
     */
    bool isEnabled() const;

    /**
     * @brief Return the size of MPI_COMM_WORLD.
     */
    int globalSize() const;

    /**
     * @brief Return the rank of the current process in MPI_COMM_WORLD.
     */
    int globalRank() const;

    /**
     * @brief Return the Mercury address of the given rank.
     *
     * @param rank Rank of the process.
     */
    const std::string& addressOfRank(int rank) const;

  private:

    /**
     * @brief This constructor will initialize MPI if it hasn't been initialized.
     */
    MPIEnv();

    MPIEnv(std::shared_ptr<MPIEnvImpl> s)
    : self(std::move(s)) {}

    operator std::shared_ptr<MPIEnvImpl>() const {
        return self;
    }

    std::shared_ptr<MPIEnvImpl> self;
};

} // namespace bedrock

#endif
