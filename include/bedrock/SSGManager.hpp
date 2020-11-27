/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SSG_MANAGER_HPP
#define __BEDROCK_SSG_MANAGER_HPP

#include <bedrock/MargoManager.hpp>
#include <ssg.h>
#include <string>
#include <memory>

namespace bedrock {

class Server;
class DependencyFinder;
class SSGManagerImpl;

/**
 * @brief The SSGManager class encapsulates an ssg_group_id_t.
 */
class SSGManager {

    friend class Server;
    friend class DependencyFinder;

  public:
    /**
     * @brief Constructor from a JSON configurations string.
     *
     * @param margo MargoManager
     * @param configString Configuration string.
     */
    SSGManager(const MargoManager& margo, const std::string& configString = "");

    /**
     * @brief Copy-constructor.
     */
    SSGManager(const SSGManager&);

    /**
     * @brief Move-constructor.
     */
    SSGManager(SSGManager&&);

    /**
     * @brief Copy-assignment operator.
     */
    SSGManager& operator=(const SSGManager&);

    /**
     * @brief Move-assignment operator.
     */
    SSGManager& operator=(SSGManager&&);

    /**
     * @brief Destructor.
     */
    ~SSGManager();

    /**
     * @brief Checks whether the SSGManager instance is valid.
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
    std::shared_ptr<SSGManagerImpl> self;

    inline operator std::shared_ptr<SSGManagerImpl>() const { return self; }

    inline SSGManager(std::shared_ptr<SSGManagerImpl> impl)
    : self(std::move(impl)) {}

    static void membershipUpdate(void* group_data, ssg_member_id_t member_id,
                                 ssg_member_update_type_t update_type);
};

} // namespace bedrock

#endif
