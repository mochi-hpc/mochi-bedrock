/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_ABTIO_CONTEXT_HPP
#define __BEDROCK_ABTIO_CONTEXT_HPP

#include <abt-io.h>
#include <thallium.hpp>
#include <memory>

namespace bedrock {

namespace tl = thallium;

class Server;
class MargoContext;
class ABTioContextImpl;

/**
 * @brief The ABTioContext class is encapsulates a margo_instance_id.
 */
class ABTioContext {

    friend class Server;

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
    ABTioContext(const MargoContext& margo_ctx,
                 const std::string&  configString);

    /**
     * @brief Constructor from an existing array of pairs
     * <name, abt_io_instance_id>.
     *
     * @param abt_io_ids Vector of pairs of names and abt-io instances.
     */
    ABTioContext(const std::vector<std::pair<std::string, abt_io_instance_id>>&
                     abt_io_ids);

    /**
     * @brief Copy-constructor.
     */
    ABTioContext(const ABTioContext&);

    /**
     * @brief Move-constructor.
     */
    ABTioContext(ABTioContext&&);

    /**
     * @brief Copy-assignment operator.
     */
    ABTioContext& operator=(const ABTioContext&);

    /**
     * @brief Move-assignment operator.
     */
    ABTioContext& operator=(ABTioContext&&);

    /**
     * @brief Destructor.
     */
    ~ABTioContext();

    /**
     * @brief Checks whether the ABTioContext instance is valid.
     */
    operator bool() const;

    /**
     * @brief Get the internal abt_io_instance_id by its name.
     *
     * @return internal abt_io_instance_id or ABT_IO_INSTANCE_NULL.
     */
    abt_io_instance_id getABTioInstance(const std::string& name) const;

    /**
     * @brief Get the internal abt_io_instance_id by its index.
     *
     * @return internal abt_io_instance_id or ABT_IO_INSTANCE_NULL.
     */
    abt_io_instance_id getABTioInstance(int index) const;

    /**
     * @brief Get the name of an abt-io instance from its index.
     * If the index is invalid, the function will return "".
     *
     * @param index Index of the abt-io instance
     */
    const std::string& getABTioInstanceName(int index) const;

    /**
     * @brief Get the index of an abt-io instance from its name.
     * If the name is invalid, the function will return -1.
     *
     * @param name Name of the abt-io instance
     */
    int getABTioInstanceIndex(const std::string& name) const;

    /**
     * @brief Returns the number of abt-io instances stored.
     */
    size_t numABTioInstances() const;

  private:
    std::shared_ptr<ABTioContextImpl> self;

    inline operator std::shared_ptr<ABTioContextImpl>() const { return self; }

    inline ABTioContext(std::shared_ptr<ABTioContextImpl> impl)
    : self(std::move(impl)) {}
};

} // namespace bedrock

#endif
