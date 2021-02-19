/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_DEPENDENCY_RESOLVER_HPP
#define __BEDROCK_DEPENDENCY_RESOLVER_HPP

#include <bedrock/MargoManager.hpp>
#include <bedrock/ABTioManager.hpp>
#include <bedrock/SSGManager.hpp>
#include <bedrock/ProviderManager.hpp>
#include <bedrock/ClientManager.hpp>
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
     * @param cmanager Client manager
     */
    DependencyFinder(const MargoManager& margo, const ABTioManager& abtio,
                     const SSGManager& ssg, const ProviderManager& pmanager,
                     const ClientManager& cmanager);

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
     * @param [in] type Type of dependency.
     * @param [in] spec Specification string.
     * @param [out] Resolved specification.
     *
     * @return handle to dependency
     */
    VoidPtr find(const std::string& type, const std::string& spec,
                 std::string* resolved = nullptr) const;

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
     * @param [in] type Type of provider
     * @param [in] name Provider name
     * @param [out] provider_id Provider id found
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr findProvider(const std::string& type, const std::string& name,
                         uint16_t* provider_id = nullptr) const;

    /**
     * @brief Find client with a given name. The returned
     * handle will remain valid until the program terminates.
     * If the name is empty, this function will try to find
     * a client of the specified type.
     *
     * @param type Type of client.
     * @param name Name of the client.
     * @param create_if_not_found Create the client if not found.
     * @param found_name name of the client found or create.
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr findClient(const std::string& type, const std::string& name,
                       std::string* found_name = nullptr) const;

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
     * @param client_name Name of the client to use (or "" for any client of the
     * right type)
     * @param type Type of service.
     * @param provider_id Provider id
     * @param locator Location (e.g. "local" or mercury or ssg addresses)
     * @param resolved Output resolved specification
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr makeProviderHandle(const std::string& client_name,
                               const std::string& type, uint16_t provider_id,
                               const std::string& locator,
                               std::string*       resolved = nullptr) const;

    /**
     * @brief Make a provider handle to a specified provider.
     * Throws an exception if no provider was found with this
     * provider name at the specified location.
     * The returned VoidPtr object owns the underlying provider
     * handle, so the caller is responsible for copying it if
     * necessary.
     *
     * @param client_name Name of the client to use (or "" for any client of the
     * right type)
     * @param type Type of service.
     * @param name Name of the provider
     * @param locator Location (e.g. "local" or mercury or ssg addresses)
     * @param resolved Output resolved specification
     *
     * @return An abstract pointer to the dependency.
     */
    VoidPtr makeProviderHandle(const std::string& client_name,
                               const std::string& type, const std::string& name,
                               const std::string& locator,
                               std::string*       resolved = nullptr) const;

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
