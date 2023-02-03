/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABTIO_MANAGER_HPP
#define __BEDROCK_ABTIO_MANAGER_HPP

#include <thallium.hpp>
#include <memory>
#include <bedrock/NamedDependency.hpp>

/* Forward declaration to avoid including <abt-io.h> */
typedef struct abt_io_instance* abt_io_instance_id;

namespace bedrock {

namespace tl = thallium;

class Server;
class ServerImpl;
class MargoManager;
class DependencyFinder;
class ABTioManagerImpl;

/**
 * @brief The ABTioManager class is encapsulates multiple ABT-IO instances.
 */
class ABTioManager {

    friend class Server;
    friend class ServerImpl;
    friend class DependencyFinder;

  public:
    /**
     * @brief Constructor from a configuration string. The configuration
     * should be a JSON array listing ABT-IO instances in the following
     * format:
     * [ ... { "name" : "some_name", "pool" : "some_pool" }, ... ]
     *
     * @param margo_ctx Margo context.
     * @param configString JSON configuration string.
     */
    ABTioManager(const MargoManager& margo_ctx,
                 const std::string&  configString);

    /**
     * @brief Copy-constructor.
     */
    ABTioManager(const ABTioManager&);

    /**
     * @brief Move-constructor.
     */
    ABTioManager(ABTioManager&&);

    /**
     * @brief Copy-assignment operator.
     */
    ABTioManager& operator=(const ABTioManager&);

    /**
     * @brief Move-assignment operator.
     */
    ABTioManager& operator=(ABTioManager&&);

    /**
     * @brief Destructor.
     */
    ~ABTioManager();

    /**
     * @brief Checks whether the ABTioManager instance is valid.
     */
    operator bool() const;

    /**
     * @brief Get the internal abt_io_instance_id by its name.
     *
     * @return internal abt_io_instance_id or nullptr.
     */
    std::shared_ptr<NamedDependency>
        getABTioInstance(const std::string& name) const;

    /**
     * @brief Get the internal abt_io_instance_id by its index.
     *
     * @return internal abt_io_instance_id or nullptr.
     */
    std::shared_ptr<NamedDependency>
        getABTioInstance(int index) const;

    /**
     * @brief Returns the number of abt-io instances stored.
     */
    size_t numABTioInstances() const;

    /**
     * @brief Adds an ABT-IO instance.
     *
     * @param name Name of the instance.
     * @param pool Name of the pool to use.
     * @param config JSON-formatted configuration.
     */
    std::shared_ptr<NamedDependency>
        addABTioInstance(const std::string& name,
                         const std::string& pool,
                         const std::string& config);

    /**
     * @brief Return the current JSON configuration.
     */
    std::string getCurrentConfig() const;

  private:
    std::shared_ptr<ABTioManagerImpl> self;

    inline operator std::shared_ptr<ABTioManagerImpl>() const { return self; }

    inline ABTioManager(std::shared_ptr<ABTioManagerImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
