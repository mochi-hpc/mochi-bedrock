/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_RESOLVER_HPP
#define __BEDROCK_DEPENDENCY_RESOLVER_HPP

#include <bedrock/MargoContext.hpp>
#include <bedrock/ABTioContext.hpp>
#include <bedrock/SSGContext.hpp>
#include <bedrock/ProviderManager.hpp>
#include <bedrock/VoidPtr.hpp>
#include <string>
#include <memory>

namespace bedrock {

class Server;
class DependencyFinderImpl;

/**
 * @brief A DependencyFinder is an object that takes a dependency
 * specification and resolves it into a void* handle to that dependency.
 */
class DependencyFinder {

    friend class Server;

  public:
    /**
     * @brief Constructor.
     *
     * @param margo Margo context
     * @param abtio ABT-IO context
     * @param ssg SSG context
     * @param pmanager Provider manager
     */
    DependencyFinder(const MargoContext& margo, const ABTioContext& abtio,
                     const SSGContext& ssg, const ProviderManager& pmanager);

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
     * be resolved. A specification string follows the following grammar:
     *
     * SPEC       := IDENTIFIER
     *            |  IDENTIFIER '@' LOCATION
     * IDENTIFIER := NAME
     *            |  TYPE ':' ID
     * LOCATION   := ADDRESS
     *            |  'ssg://' GROUP '/' RANK
     * ADDRESS    := <mercury address>
     * NAME       := <qualified identifier>
     * ID         := <provider id>
     *
     * @param type Type of dependency.
     * @param spec Specification string.
     *
     * @return handle to dependency
     */
    VoidPtr find(const std::string& type, const std::string& spec) const;

    /**
     * @brief Find a local provider based on a type and provider id.
     * Throws an exception if not found.
     *
     * @param type Provider type
     * @param provider_id Provider id
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr findProvider(const std::string& type, uint16_t provider_id) const;

    /**
     * @brief Find a local provider based on a name.
     * Throws an exception if not found.
     *
     * @param type Type of provider
     * @param name Provider name
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr findProvider(const std::string& type,
                         const std::string& name) const;

    /**
     * @brief Get a client of a given type. The returned
     * handle will remain valid until the program terminates.
     *
     * @param type Type of client.
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr getClient(const std::string& type) const;

    /**
     * @brief Get an admin of a given type. The returned
     * handle will remain valid until the program terminates.
     *
     * @param type Type of admin.
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr getAdmin(const std::string& type) const;

    /**
     * @brief Make a provider handle to a specified provider.
     * Throws an exception if no provider was found with this
     * provider id at the specified location.
     * The returned VoidPtr object owns the underlying provider
     * handle, so the caller is responsible for copying it if
     * necessary.
     *
     * @param type Type of service.
     * @param provider_id Provider id
     * @param locator Location (e.g. "local" or mercury or ssg addresses)
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr makeProviderHandle(const std::string& type, uint16_t provider_id,
                               const std::string& locator) const;

    /**
     * @brief Make a provider handle to a specified provider.
     * Throws an exception if no provider was found with this
     * provider name at the specified location.
     * The returned VoidPtr object owns the underlying provider
     * handle, so the caller is responsible for copying it if
     * necessary.
     *
     * @param type Type of service.
     * @param provider_id Provider id
     * @param locator Location (e.g. "local" or mercury or ssg addresses)
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr makeProviderHandle(const std::string& type, const std::string& name,
                               const std::string& locator) const;

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
