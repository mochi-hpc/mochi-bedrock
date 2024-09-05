/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_RESOLVER_HPP
#define __BEDROCK_DEPENDENCY_RESOLVER_HPP

#include <bedrock/MargoManager.hpp>
#include <bedrock/ProviderManager.hpp>
#include <bedrock/MPIEnv.hpp>
#include <string>
#include <memory>

namespace bedrock {

class Server;
class ServerImpl;
class ProviderManager;
class DependencyFinderImpl;

/**
 * @brief A DependencyFinder is an object that takes a dependency
 * specification and resolves it into a void* handle to that dependency.
 */
class DependencyFinder {

    friend class Server;
    friend class ProviderManager;
    friend class ServerImpl;

  public:
    /**
     * @brief Constructor.
     * @param mpi MPI context
     * @param margo Margo context
     * @param pmanager Provider manager
     */
    DependencyFinder(const MPIEnv& mpi,
                     const MargoManager& margo,
                     const ProviderManager& pmanager);

    /**
     * @brief Copy-constructor.
     */
    DependencyFinder(const DependencyFinder&);

    /**
     * @brief Move-constructor.
     */
    DependencyFinder(DependencyFinder&&);

    /**
     * @brief Copy-assignment operator.
     */
    DependencyFinder& operator=(const DependencyFinder&);

    /**
     * @brief Move-assignment operator.
     */
    DependencyFinder& operator=(DependencyFinder&&);

    /**
     * @brief Destructor.
     */
    ~DependencyFinder();

    /**
     * @brief Checks whether the DependencyFinder instance is valid.
     */
    operator bool() const;

    /**
     * @brief Resolve a specification, returning a void* handle to it.
     * This function throws an exception if the specification could not
     * be resolved. A specification is either a name, or a string follows
     * the following grammar:
     *
     * SPEC       := IDENTIFIER
     *            |  IDENTIFIER '@' LOCATION
     * IDENTIFIER := SPECIFIER
     *            |  NAME '->' SPECIFIER
     * SPECIFIER  := NAME
     *            |  TYPE ':' ID
     * LOCATION   := ADDRESS
     * ADDRESS    := <mercury address>
     * NAME       := <qualified identifier>
     * ID         := <provider id>
     *
     *
     * For instance, "abc" represents the name "abc".
     * "abc:123" represents a provider of type "abc" with
     * provider id 123. "abc@address" represents a provider handle
     * pointing to a provider named "abc" at address "address".
     *
     * @param [in] type Type of dependency.
     * @param [in] spec Specification string.
     * @param [out] Resolved specification.
     *
     * @return handle to dependency
     */
    std::shared_ptr<NamedDependency>
        find(const std::string& type,
             const std::string& spec,
             std::string* resolved) const;

    /**
     * @brief Find a local provider based on a type and provider id.
     * Throws an exception if not found.
     *
     * @param type Provider type
     * @param provider_id Provider id
     *
     * @return An abstract pointer to the dependency.
     */
    std::shared_ptr<NamedDependency> findProvider(
            const std::string& type,
            uint16_t provider_id) const;

    /**
     * @brief Find a local provider based on a name.
     * Throws an exception if not found.
     *
     * @param [in] type Type of provider
     * @param [in] name Provider name
     * @param [out] provider_id Provider id found
     *
     * @return An abstract pointer to the dependency.
     */
    std::shared_ptr<NamedDependency> findProvider(
            const std::string& type,
            const std::string& name,
            uint16_t* provider_id = nullptr) const;

    /**
     * @brief Make a provider handle to a specified provider.
     * Throws an exception if no provider was found with this
     * provider id at the specified location.
     *
     * @param type Type of service.
     * @param provider_id Provider id
     * @param locator Location (e.g. "local" or mercury address)
     * @param resolved Output resolved specification
     *
     * @return An abstract pointer to the dependency.
     */
    std::shared_ptr<NamedDependency>
        makeProviderHandle(const std::string& type,
                           uint16_t provider_id,
                           const std::string& locator,
                           std::string* resolved) const;

    /**
     * @brief Make a provider handle to a specified provider.
     * Throws an exception if no provider was found with this
     * provider name at the specified location.
     *
     * @param type Type of service.
     * @param name Name of the provider
     * @param locator Location (e.g. "local" or mercury addresses)
     * @param resolved Output resolved specification
     *
     * @return An abstract pointer to the dependency.
     */
    std::shared_ptr<NamedDependency> makeProviderHandle(
            const std::string& type,
            const std::string& name,
            const std::string& locator,
            std::string* resolved) const;

  private:
    std::shared_ptr<DependencyFinderImpl> self;

    inline operator std::shared_ptr<DependencyFinderImpl>() const {
        return self;
    }

    inline DependencyFinder(std::shared_ptr<DependencyFinderImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
