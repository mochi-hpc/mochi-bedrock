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
    friend class SSGEntry;
    friend class MonaEntry;
    friend class MonaManager;
    friend class ABTioEntry;
    friend class ABTioManager;
    friend class ProviderEntry;
    friend class ServerImpl;

  public:

    /**
     * @brief Structure holding information about a pool in Margo.
     */
    struct PoolInfo {
        ABT_pool    pool;
        std::string name;
        uint32_t    index;
    };

    /**
     * @brief Structure holding information about an ES in Margo.
     */
    struct XstreamInfo {
        ABT_xstream xstream;
        std::string name;
        uint32_t    index;
    };

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
    PoolInfo getPool(int index) const;

    /**
     * @brief Get the pool corresponding to a particular name.
     */
    PoolInfo getPool(const std::string& name) const;

    /**
     * @brief Get the pool corresponding to a particular handle.
     */
    PoolInfo getPool(ABT_pool pool) const;

    /**
     * @brief Get the number of pools.
     */
    size_t getNumPools() const;

    /**
     * @brief Add a pool from a JSON configuration, returning
     * the corresponding PoolInfo object.
     */
    PoolInfo addPool(const std::string& config);

    /**
     * @brief Remove a pool by its index.
     */
    void removePool(int index);

    /**
     * @brief Remove a pool by its name.
     */
    void removePool(const std::string& name);

    /**
     * @brief Remove a pool by its handle.
     */
    void removePool(ABT_pool pool);

    /**
     * @brief Get the ES corresponding to a particular index.
     */
    XstreamInfo getXstream(int index) const;

    /**
     * @brief Get the xstream corresponding to a particular name.
     */
    XstreamInfo getXstream(const std::string& name) const;

    /**
     * @brief Get the xstream corresponding to a particular handle.
     */
    XstreamInfo getXstream(ABT_xstream es) const;

    /**
     * @brief Get the number of pools.
     */
    size_t getNumXstreams() const;

    /**
     * @brief Add an ES from a JSON configuration, returning
     * the corresponding XstreamInfo object.
     */
    XstreamInfo addXstream(const std::string& config);

    /**
     * @brief Remove an ES by its index.
     */
    void removeXstream(int index);

    /**
     * @brief Remove an xstream by its name.
     */
    void removeXstream(const std::string& name);

    /**
     * @brief Remove an xstream by its handle.
     */
    void removeXstream(ABT_xstream xstream);

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
