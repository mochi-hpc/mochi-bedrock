/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_BACKEND_HPP
#define __BEDROCK_BACKEND_HPP

#include <bedrock/RequestResult.hpp>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>
#include <thallium.hpp>

/**
 * @brief Helper class to register backend types into the backend factory.
 */
template <typename BackendType> class __BedrockBackendRegistration;

namespace bedrock {

/**
 * @brief Interface for Service backends. To build a new backend,
 * implement a class MyBackend that inherits from Backend, and put
 * BEDROCK_REGISTER_BACKEND(mybackend, MyBackend); in a cpp file
 * that includes your backend class' header file.
 *
 * Your backend class should also have two static functions to
 * respectively create and open a Service:
 *
 * std::unique_ptr<Backend> create(const json& config)
 * std::unique_ptr<Backend> attach(const json& config)
 */
class Backend {

  public:
    /**
     * @brief Constructor.
     */
    Backend() = default;

    /**
     * @brief Move-constructor.
     */
    Backend(Backend&&) = default;

    /**
     * @brief Copy-constructor.
     */
    Backend(const Backend&) = default;

    /**
     * @brief Move-assignment operator.
     */
    Backend& operator=(Backend&&) = default;

    /**
     * @brief Copy-assignment operator.
     */
    Backend& operator=(const Backend&) = default;

    /**
     * @brief Destructor.
     */
    virtual ~Backend() = default;

    /**
     * @brief Prints Hello World.
     */
    virtual void sayHello() = 0;

    /**
     * @brief Compute the sum of two integers.
     *
     * @param x first integer
     * @param y second integer
     *
     * @return a RequestResult containing the result.
     */
    virtual RequestResult<int32_t> computeSum(int32_t x, int32_t y) = 0;

    /**
     * @brief Destroys the underlying Service.
     *
     * @return a RequestResult<bool> instance indicating
     * whether the database was successfully destroyed.
     */
    virtual RequestResult<bool> destroy() = 0;
};

/**
 * @brief The ServiceFactory contains functions to create
 * or open Services.
 */
class ServiceFactory {

    template <typename BackendType> friend class ::__BedrockBackendRegistration;

    using json = nlohmann::json;

  public:
    ServiceFactory() = delete;

    /**
     * @brief Creates a Service and returns a unique_ptr to the created
     * instance.
     *
     * @param backend_name Name of the backend to use.
     * @param engine Thallium engine.
     * @param config Configuration object to pass to the backend's create
     * function.
     *
     * @return a unique_ptr to the created Service.
     */
    static std::unique_ptr<Backend>
    createService(const std::string&      backend_name,
                  const thallium::engine& engine, const json& config);

    /**
     * @brief Opens an existing database and returns a unique_ptr to the
     * created backend instance.
     *
     * @param backend_name Name of the backend to use.
     * @param engine Thallium engine.
     * @param config Configuration object to pass to the backend's open
     * function.
     *
     * @return a unique_ptr to the created Backend.
     */
    static std::unique_ptr<Backend> openService(const std::string& backend_name,
                                                const thallium::engine& engine,
                                                const json&             config);

  private:
    static std::unordered_map<std::string,
                              std::function<std::unique_ptr<Backend>(
                                  const thallium::engine&, const json&)>>
        create_fn;

    static std::unordered_map<std::string,
                              std::function<std::unique_ptr<Backend>(
                                  const thallium::engine&, const json&)>>
        open_fn;
};

} // namespace bedrock

#define BEDROCK_REGISTER_BACKEND(__backend_name, __backend_type) \
    static __BedrockBackendRegistration<__backend_type>          \
        __bedrock##__backend_name##_backend(#__backend_name)

template <typename BackendType> class __BedrockBackendRegistration {

    using json = nlohmann::json;

  public:
    __BedrockBackendRegistration(const std::string& backend_name) {
        bedrock::ServiceFactory::create_fn[backend_name]
            = [](const thallium::engine& engine, const json& config) {
                  return BackendType::create(engine, config);
              };
        bedrock::ServiceFactory::open_fn[backend_name]
            = [](const thallium::engine& engine, const json& config) {
                  return BackendType::open(engine, config);
              };
    }
};

#endif
