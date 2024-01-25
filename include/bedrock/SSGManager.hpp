/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SSG_MANAGER_HPP
#define __BEDROCK_SSG_MANAGER_HPP

#include <bedrock/MargoManager.hpp>
#include <string>
#include <memory>

typedef uint64_t ssg_group_id_t;
typedef uint64_t ssg_member_id_t;
typedef struct ssg_group_config ssg_group_config_t;

namespace bedrock {

class Server;
class ServerImpl;
class DependencyFinder;
class SSGManagerImpl;
class SSGUpdateHandler;

/**
 * @brief The SSGManager class encapsulates an ssg_group_id_t.
 */
class SSGManager {

    friend class Server;
    friend class ServerImpl;
    friend class DependencyFinder;
    friend class SSGUpdateHandler;

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
     * @brief Create a group from a full configuration.
     *
     * @param config JSON-formatted configuration.
     *
     * @return The created group id.
     */
    std::shared_ptr<NamedDependency>
        createGroupFromConfig(const std::string& config);

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
    std::shared_ptr<NamedDependency>
        createGroup(const std::string&        name,
                    const ssg_group_config_t* config,
                    const std::shared_ptr<NamedDependency>& pool,
                    const std::string& bootstrap_method,
                    const std::string& group_file = "");

    /**
     * @brief Get the internal ssg_group_id_t corresponding to a group.
     *
     * @return the internal ssg_group_id_t.
     */
    std::shared_ptr<NamedDependency> getGroup(const std::string& group_name) const;

    /**
     * @brief Get the internal ssg_group_id_t corresponding to a group index.
     *
     * @return the internal ssg_group_id_t.
     */
    std::shared_ptr<NamedDependency> getGroup(uint32_t index) const;

    /**
     * @brief Get the number of groups.
     */
    size_t getNumGroups() const;

    /**
     * @brief Resolve an address starting with ssg://
     * These address may be:
     * - ssg://<group-name>/members/<member-id>
     * - ssg://<group-name>/ranks/<rank>
     * This function will throw an exception if the address
     * could not be resolved. The caller is NOT supposed to
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
};

} // namespace bedrock

#endif
