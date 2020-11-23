/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MARGO_CONTEXT_HPP
#define __BEDROCK_MARGO_CONTEXT_HPP

#include <thallium.hpp>
#include <memory>

namespace bedrock {

namespace tl = thallium;

class Server;
class ProviderManager;
class DependencyFinder;
class MargoContextImpl;

/**
 * @brief The MargoContext class is encapsulates a margo_instance_id.
 */
class MargoContext {

    friend class Server;
    friend class ProviderManager;
    friend class DependencyFinder;

  public:
    /**
     * @brief Constructor from an existing Margo instance.
     *
     * @param mid Margo instance, if already initialized.
     */
    MargoContext(margo_instance_id mid);

    /**
     * @brief Constructor from a JSON configurations string.
     *
     * @param address Address.
     * @param configString Configuration string.
     */
    MargoContext(const std::string& address,
                 const std::string& configString = "");

    /**
     * @brief Copy-constructor.
     */
    MargoContext(const MargoContext&);

    /**
     * @brief Move-constructor.
     */
    MargoContext(MargoContext&&);

    /**
     * @brief Copy-assignment operator.
     */
    MargoContext& operator=(const MargoContext&);

    /**
     * @brief Move-assignment operator.
     */
    MargoContext& operator=(MargoContext&&);

    /**
     * @brief Destructor.
     */
    ~MargoContext();

    /**
     * @brief Checks whether the MargoContext instance is valid.
     */
    operator bool() const;

    /**
     * @brief Get the internal margo_instance_id.
     *
     * @return the internal margo_instance_id.
     */
    margo_instance_id getMargoInstance() const;

    /**
     * @brief Get the pool corresponding to a particular index.
     */
    ABT_pool getPool(int index) const;

    /**
     * @brief Get the pool corresponding to a particular name.
     */
    ABT_pool getPool(const std::string& name) const;

  private:
    std::shared_ptr<MargoContextImpl> self;

    inline operator std::shared_ptr<MargoContextImpl>() const { return self; }

    inline MargoContext(std::shared_ptr<MargoContextImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
