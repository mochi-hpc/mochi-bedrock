/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MARGO_MANAGER_HPP
#define __BEDROCK_MARGO_MANAGER_HPP

#include <thallium.hpp>
#include <memory>

namespace bedrock {

namespace tl = thallium;

class Server;
class ProviderManager;
class ClientManager;
class DependencyFinder;
class MargoManagerImpl;
class SSGManager;
class SSGData;
class ABTioManager;
class ABTioEntry;
class MonaManager;
class MonaEntry;
class ProviderEntry;
class ServerImpl;

/**
 * @brief The MargoManager class is encapsulates a margo_instance_id.
 */
class MargoManager {

    friend class Server;
    friend class ProviderManager;
    friend class ClientManager;
    friend class DependencyFinder;
    friend class SSGManager;
    friend class SSGData;
    friend class MonaEntry;
    friend class MonaManager;
    friend class ABTioEntry;
    friend class ABTioManager;
    friend class ProviderEntry;
    friend class ServerImpl;

  public:
    /**
     * @brief Constructor from an existing Margo instance.
     *
     * @param mid Margo instance, if already initialized.
     */
    MargoManager(margo_instance_id mid);

    /**
     * @brief Constructor from a JSON configurations string.
     *
     * @param address Address.
     * @param configString Configuration string.
     */
    MargoManager(const std::string& address,
                 const std::string& configString = "");

    /**
     * @brief Copy-constructor.
     */
    MargoManager(const MargoManager&);

    /**
     * @brief Move-constructor.
     */
    MargoManager(MargoManager&&);

    /**
     * @brief Copy-assignment operator.
     */
    MargoManager& operator=(const MargoManager&);

    /**
     * @brief Move-assignment operator.
     */
    MargoManager& operator=(MargoManager&&);

    /**
     * @brief Destructor.
     */
    ~MargoManager();

    /**
     * @brief Checks whether the MargoManager instance is valid.
     */
    operator bool() const;

    /**
     * @brief Get the internal margo_instance_id.
     *
     * @return the internal margo_instance_id.
     */
    margo_instance_id getMargoInstance() const;

    /**
     * @brief Get the thallium engine associated with the Margo instance.
     *
     * @return reference to the thallium engine.
     */
    const tl::engine& getThalliumEngine() const;

    /**
     * @brief Get the default handle pool from Margo.
     */
    ABT_pool getDefaultHandlerPool() const;

    /**
     * @brief Get the pool corresponding to a particular index.
     */
    ABT_pool getPool(int index) const;

    /**
     * @brief Get the pool corresponding to a particular name.
     */
    ABT_pool getPool(const std::string& name) const;

    /**
     * @brief Get the number of pools.
     */
    size_t getNumPools() const;

    /**
     * @brief Resolve an ABT_pool handle to a <name, index> pair,
     * or <"",-1> if the pool wasn't recognized.
     *
     * @param pool Pool
     */
    std::pair<std::string, int> getPoolInfo(ABT_pool pool) const;

    /**
     * @brief Return the current JSON configuration.
     */
    std::string getCurrentConfig() const;

  private:
    std::shared_ptr<MargoManagerImpl> self;

    inline operator std::shared_ptr<MargoManagerImpl>() const { return self; }

    inline MargoManager(std::shared_ptr<MargoManagerImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
