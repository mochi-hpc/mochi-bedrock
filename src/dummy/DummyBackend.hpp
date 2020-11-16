/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#ifndef __DUMMY_BACKEND_HPP
#define __DUMMY_BACKEND_HPP

#include <bedrock/Backend.hpp>

using json = nlohmann::json;

/**
 * Dummy implementation of an bedrock Backend.
 */
class DummyService : public bedrock::Backend {
   
    json m_config;

    public:

    /**
     * @brief Constructor.
     */
    DummyService(const json& config)
    : m_config(config) {}

    /**
     * @brief Move-constructor.
     */
    DummyService(DummyService&&) = default;

    /**
     * @brief Copy-constructor.
     */
    DummyService(const DummyService&) = default;

    /**
     * @brief Move-assignment operator.
     */
    DummyService& operator=(DummyService&&) = default;

    /**
     * @brief Copy-assignment operator.
     */
    DummyService& operator=(const DummyService&) = default;

    /**
     * @brief Destructor.
     */
    virtual ~DummyService() = default;

    /**
     * @brief Prints Hello World.
     */
    void sayHello() override;

    /**
     * @brief Compute the sum of two integers.
     *
     * @param x first integer
     * @param y second integer
     *
     * @return a RequestResult containing the result.
     */
    bedrock::RequestResult<int32_t> computeSum(int32_t x, int32_t y) override;

    /**
     * @brief Destroys the underlying Service.
     *
     * @return a RequestResult<bool> instance indicating
     * whether the database was successfully destroyed.
     */
    bedrock::RequestResult<bool> destroy() override;

    /**
     * @brief Static factory function used by the ServiceFactory to
     * create a DummyService.
     *
     * @param engine Thallium engine
     * @param config JSON configuration for the Service
     *
     * @return a unique_ptr to a Service
     */
    static std::unique_ptr<bedrock::Backend> create(const thallium::engine& engine, const json& config);

    /**
     * @brief Static factory function used by the ServiceFactory to
     * open a DummyService.
     *
     * @param engine Thallium engine
     * @param config JSON configuration for the Service
     *
     * @return a unique_ptr to a Service
     */
    static std::unique_ptr<bedrock::Backend> open(const thallium::engine& engine, const json& config);
};

#endif
