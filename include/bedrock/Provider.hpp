/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_PROVIDER_HPP
#define __BEDROCK_PROVIDER_HPP

#include <thallium.hpp>
#include <memory>

namespace bedrock {

namespace tl = thallium;

class ProviderImpl;

/**
 * @brief A Provider is an object that can receive RPCs
 * and dispatch them to specific Services.
 */
class Provider {

  public:
    /**
     * @brief Constructor.
     *
     * @param engine Thallium engine to use to receive RPCs.
     * @param provider_id Provider id.
     * @param pool Argobots pool to use to handle RPCs.
     */
    Provider(const tl::engine& engine, uint16_t provider_id = 0,
             const tl::pool& pool = tl::pool());

    /**
     * @brief Constructor.
     *
     * @param mid Margo instance id to use to receive RPCs.
     * @param provider_id Provider id.
     * @param pool Argobots pool to use to handle RPCs.
     */
    Provider(margo_instance_id mid, uint16_t provider_id = 0,
             const tl::pool& pool = tl::pool());

    /**
     * @brief Copy-constructor is deleted.
     */
    Provider(const Provider&) = delete;

    /**
     * @brief Move-constructor.
     */
    Provider(Provider&&);

    /**
     * @brief Copy-assignment operator is deleted.
     */
    Provider& operator=(const Provider&) = delete;

    /**
     * @brief Move-assignment operator is deleted.
     */
    Provider& operator=(Provider&&) = delete;

    /**
     * @brief Destructor.
     */
    ~Provider();

    /**
     * @brief Sets a security string that should be provided
     * by Admin RPCs to accept them.
     *
     * @param token Security token to set.
     */
    void setSecurityToken(const std::string& token);

    /**
     * @brief Checks whether the Provider instance is valid.
     */
    operator bool() const;

  private:
    std::shared_ptr<ProviderImpl> self;
};

} // namespace bedrock

#endif
