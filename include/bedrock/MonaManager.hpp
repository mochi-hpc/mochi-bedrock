/*
 * (C) 2022 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MONA_MANAGER_HPP
#define __BEDROCK_MONA_MANAGER_HPP

#include <thallium.hpp>
#include <memory>
#include <bedrock/NamedDependency.hpp>
#include <nlohmann/json.hpp>

/* Forward declaration to avoid including <mona.h> */
typedef struct mona_instance* mona_instance_t;

namespace bedrock {

namespace tl = thallium;

class Server;
class ServerImpl;
class DependencyFinder;
class MargoManager;
class MonaManagerImpl;
class Jx9Manager;

/**
 * @brief The MonaManager class encapsulates multiple MoNA instances.
 */
class MonaManager {

    friend class Server;
    friend class ServerImpl;
    friend class DependencyFinder;

    using json = nlohmann::json;

  public:
    /**
     * @brief Constructor from a configuration string. The configuration
     * should be a JSON array listing MoNA instances in the following
     * format:
     * [ ... { "name" : "some_name", "pool" : "some_pool", "address" : "some_address" }, ... ]
     *
     * @param margoMgr MargoManager
     * @param jx9 JX9 manager
     * @param config JSON configuration of the manager.
     * @param defaultProtocol default protocol to use if not specified in the JSON.
     */
    MonaManager(const MargoManager& margo,
                const Jx9Manager& jx9,
                const json& config,
                const std::string& defaultProtocol);

    /**
     * @brief Copy-constructor.
     */
    MonaManager(const MonaManager&);

    /**
     * @brief Move-constructor.
     */
    MonaManager(MonaManager&&);

    /**
     * @brief Copy-assignment operator.
     */
    MonaManager& operator=(const MonaManager&);

    /**
     * @brief Move-assignment operator.
     */
    MonaManager& operator=(MonaManager&&);

    /**
     * @brief Destructor.
     */
    ~MonaManager();

    /**
     * @brief Checks whether the MonaManager instance is valid.
     */
    operator bool() const;

    /**
     * @brief Get an internal Mona instance by its name.
     * If not found, this function will throw an Exception.
     * If returned, the shared_ptr is guaranteed not to be null.
     *
     * @return a NamedDependency representing the Mona instance.
     */
    std::shared_ptr<NamedDependency>
        getMonaInstance(const std::string& name) const;

    /**
     * @brief Get the internal mona_instance_t by its index.
     * If not found, this function will throw an Exception.
     * If returned, the shared_ptr is guaranteed not to be null.
     *
     * @return a NamedDependency representing the Mona instance.
     */
    std::shared_ptr<NamedDependency>
        getMonaInstance(size_t index) const;

    /**
     * @brief Returns the number of mona instances stored.
     */
    size_t numMonaInstances() const;

    /**
     * @brief Adds a Mona instance.
     *
     * @param name Name of the instance.
     * @param pool Name of the pool to use.
     * @param address Address to use..
     */
    std::shared_ptr<NamedDependency>
        addMonaInstance(const std::string& name,
                        std::shared_ptr<NamedDependency> pool,
                        const std::string& address);

    /**
     * @brief Adds a Mona instance from a JSON description.
     *
     * @param description JSON description.
     *
     * @return a NamedDependency representing the newly-created Mona instance.
     */
    std::shared_ptr<NamedDependency>
        addMonaInstanceFromJSON(const json& description);

    /**
     * @brief Return the current JSON configuration.
     */
    json getCurrentConfig() const;

  private:
    std::shared_ptr<MonaManagerImpl> self;

    inline operator std::shared_ptr<MonaManagerImpl>() const { return self; }

    inline MonaManager(std::shared_ptr<MonaManagerImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
