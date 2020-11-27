/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SSG_CONTEXT_HPP
#define __BEDROCK_SSG_CONTEXT_HPP

#include <bedrock/MargoContext.hpp>
#include <ssg.h>
#include <string>
#include <memory>

namespace bedrock {

class Server;
class DependencyFinder;
class SSGContextImpl;

/**
 * @brief The SSGContext class encapsulates an ssg_group_id_t.
 */
class SSGContext {

    friend class Server;
    friend class DependencyFinder;

  public:
    /**
     * @brief Constructor from a JSON configurations string.
     *
     * @param margo MargoContext
     * @param configString Configuration string.
     */
    SSGContext(const MargoContext& margo, const std::string& configString = "");

    /**
     * @brief Copy-constructor.
     */
    SSGContext(const SSGContext&);

    /**
     * @brief Move-constructor.
     */
    SSGContext(SSGContext&&);

    /**
     * @brief Copy-assignment operator.
     */
    SSGContext& operator=(const SSGContext&);

    /**
     * @brief Move-assignment operator.
     */
    SSGContext& operator=(SSGContext&&);

    /**
     * @brief Destructor.
     */
    ~SSGContext();

    /**
     * @brief Checks whether the SSGContext instance is valid.
     */
    operator bool() const;

    /**
     * @brief Create a group and add it to the SSG context.
     *
     * @param name Name of the group.
     * @param config Configuration.
     * @param pool Pool.
     * @param bootstrap_method Bootstrap method.
     * @param group_file Group file to create for other processes to join.
     *
     * @return The newly created SSG group.
     */
    ssg_group_id_t createGroup(const std::string&        name,
                               const ssg_group_config_t* config, ABT_pool pool,
                               const std::string& bootstrap_method,
                               const std::string& group_file = "");

    /**
     * @brief Get the internal ssg_group_id_t corresponding to a group.
     *
     * @return the internal ssg_group_id_t.
     */
    ssg_group_id_t getGroup(const std::string& group_name) const;

    /**
     * @brief Resolve an address starting with ssg://
     * These address may be:
     * - ssg://<group-name>/members/<member-id>
     * - ssg://<group-name>/ranks/<rank>
     * This function will throw an exception if the address
     * could not be resolved. The caller is not supposed to
     * destroy the returned hg_addr_t.
     *
     * @param address Address
     *
     * @return the corresponding hg_addr_t.
     */
    hg_addr_t resolveAddress(const std::string& address) const;

    /**
     * @brief Return the current JSON configuration.
     */
    std::string getCurrentConfig() const;

  private:
    std::shared_ptr<SSGContextImpl> self;

    inline operator std::shared_ptr<SSGContextImpl>() const { return self; }

    inline SSGContext(std::shared_ptr<SSGContextImpl> impl)
    : self(std::move(impl)) {}

    static void membershipUpdate(void* group_data, ssg_member_id_t member_id,
                                 ssg_member_update_type_t update_type);
};

} // namespace bedrock

#endif
