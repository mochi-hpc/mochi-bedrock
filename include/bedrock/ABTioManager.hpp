/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABTIO_MANAGER_HPP
#define __BEDROCK_ABTIO_MANAGER_HPP

#include <thallium.hpp>
#include <nlohmann/json.hpp>
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
class Jx9Manager;

/**
 * @brief The ABTioManager class is encapsulates multiple ABT-IO instances.
 */
class ABTioManager {

    friend class Server;
    friend class ServerImpl;
    friend class DependencyFinder;

    using json = nlohmann::json;

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
                 const Jx9Manager& jx9,
                 const json& config);

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
     * @brief Get an internal abt-io instance by its name.
     * If not found, this function will throw an Exception.
     * If returned, the shared_ptr is guaranteed not to be null.
     *
     * @return a NamedDependency representing the ABT-IO instance.
     */
    std::shared_ptr<NamedDependency>
        getABTioInstance(const std::string& name) const;

    /**
     * @brief Get an internal abt-io instance by its index.
     * If not found, this function will throw an Exception.
     * If returned, the shared_ptr is guaranteed not to be null.
     *
     * @return a NamedDependency representing the ABT-IO instance.
     */
    std::shared_ptr<NamedDependency>
        getABTioInstance(size_t index) const;

    /**
     * @brief Returns the number of abt-io instances stored.
     */
    size_t numABTioInstances() const;

    /**
     * @brief Adds an ABT-IO instance.
     *
     * @param name Name of the instance.
     * @param pool Name of the pool to use.
     * @param config JSON configuration.
     *
     * @return the NamedDependency handle for the newly-created instance.
     */
    std::shared_ptr<NamedDependency>
        addABTioInstance(const std::string&                      name,
                         const std::shared_ptr<NamedDependency>& pool = {},
                         const json&                             config = {});


    /**
     * @brief Adds an ABT-IO istance as described by the
     * provided JSON object.
     *
     * @param description ABT-IO instance description.
     *
     * @return the NamedDependency handle for the newly-created instance.
     *
     * Example of JSON description:
     *
     * ```json
     * {
     *    "name": "my_abt_io_instance",
     *    "pool": "my_pool",
     *    "config": {}
     * }
     * ```
     */
    std::shared_ptr<NamedDependency>
        addABTioInstanceFromJSON(const json& description);

    /**
     * @brief Return the current JSON configuration.
     */
    json getCurrentConfig() const;

  private:
    std::shared_ptr<ABTioManagerImpl> self;

    inline operator std::shared_ptr<ABTioManagerImpl>() const { return self; }

    inline ABTioManager(std::shared_ptr<ABTioManagerImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
