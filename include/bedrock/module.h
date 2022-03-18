/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MODULE_H
#define __BEDROCK_MODULE_H

#include <margo.h>
#include <bedrock/common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BEDROCK_OPTIONAL 0x0
#define BEDROCK_REQUIRED 0x1
#define BEDROCK_ARRAY    0x2

typedef struct bedrock_args* bedrock_args_t;
#define BEDROCK_ARGS_NULL ((bedrock_args_t)NULL)

/**
 * @brief The bellow types resolve to void* since they are
 * not known to Bedrock at compile-time nor at run-time.
 */
typedef void* bedrock_module_provider_t;
typedef void* bedrock_module_provider_handle_t;
typedef void* bedrock_module_client_t;

/**
 * @brief The bedrock_dependency structure describes
 * a module dependency. The name correspondings to the name
 * of the dependency in the module configuration. The type
 * corresponds to the type of dependency (name of other modules
 * from which the dependency comes from). The flags field
 * allows parameterizing the dependency. It should be an or-ed
 * value from BEDROCK_REQUIRED (this dependency is required)
 * and BEDROCK_ARRAY (this dependency is an array). Note that
 * BEDROCK_REQUIRED | BEDROCK_ARRAY indicates that the array
 * should contain at least 1 entry.
 *
 * For example, the following bedrock_dependency
 *   { "storage", "bake", BEDROCK_REQUIRED | BEDROCK_ARRAY }
 * indicates that a provider for this module requires to be
 * created with a "dependencies" section in its JSON looking
 * like the following:
 *   "dependencies" : {
 *      "storage" : [ "bake:34@na+sm://1234", ... ]
 *   }
 * that is, a "storage" key is expected (name = "storage"),
 * and it will resolve to an array of bake constructs
 * (e.g. providers or provider handles).
 */
struct bedrock_dependency {
    const char* name;
    const char* type;
    int32_t     flags;
};

#define BEDROCK_NO_MORE_DEPENDENCIES \
    { NULL, NULL, 0 }

/**
 * @brief Type of function called to register a provider.
 *
 * @param [in] bedrock_args_t Arguments for the provider.
 * @param [out] bedrock_module_provider_t Resulting provider.
 *
 * @return BEDROCK_SUCCESS or other error code.
 */
typedef int (*bedrock_register_provider_fn)(bedrock_args_t,
                                            bedrock_module_provider_t*);

/**
 * @brief Type of function called to deregister a provider.
 *
 * @param [in] bedrock_module_provider_t Provider.
 *
 * @return BEDROCK_SUCCESS or other error code.
 */
typedef int (*bedrock_deregister_provider_fn)(bedrock_module_provider_t);

/**
 * @brief Type of function called to register a client.
 *
 * @param [in] bedrock_args_t Arguments for the client..
 * @param [out] bedrock_module_client_t Resulting client.
 *
 * @return BEDROCK_SUCCESS or other error code.
 */
typedef int (*bedrock_init_client_fn)(bedrock_args_t args,
                                      bedrock_module_client_t*);

/**
 * @brief Type of function called to destroy a client.
 *
 * @param [in] bedrock_module_client_t Client.
 *
 * @return BEDROCK_SUCCESS or other error code.
 */
typedef int (*bedrock_finalize_client_fn)(bedrock_module_client_t);

/**
 * @brief Type of function called to create a provider handle from a
 * client, an hg_addr_t address, and a provider id.
 *
 * @param [in] bedrock_module_client_t Client.
 * @param [in] hg_addr_t Address.
 * @param [in] uint16_t Provider id.
 * @param [out] bedrock_module_provider_handle_t Resulting provider handle.
 *
 * @return BEDROCK_SUCCESS or other error code.
 */
typedef int (*bedrock_create_provider_handle_fn)(
    bedrock_module_client_t, hg_addr_t, uint16_t,
    bedrock_module_provider_handle_t*);

/**
 * @brief Type of function called to destroy a provider handle.
 *
 * @param [in] bedrock_module_provider_handle_t Provider handle.
 *
 * @return BEDROCK_SUCCESS or other error code.
 */
typedef int (*bedrock_destroy_provider_handle_fn)(
    bedrock_module_provider_handle_t);

/**
 * @brief Type of function called to get the configuration of a provider.
 * The returned string, if not NULL, should be freed by the caller.
 *
 * @param bedrock_module_provider_t Provider.
 *
 * @return null-terminated configuration string.
 */
typedef const char* (*bedrock_provider_get_config_fn)(bedrock_module_provider_t);

/**
 * @brief Type of function called to get the configuration of a client.
 * The returned string, if not NULL, should be freed by the caller.
 *
 * @param bedrock_module_client_t Client.
 *
 * @return null-terminated configuration string.
 */
typedef const char* (*bedrock_client_get_config_fn)(bedrock_module_client_t);

/**
 * @brief A global instance of the bedrock_module structure must be provided
 * in a shared library to make up a Bedrock module.
 * The provider_dependencies and client_dependencies arrays should be terminated
 * by a BEDROCK_NO_MORE_DEPENDENCIES.
 */
struct bedrock_module {
    bedrock_register_provider_fn       register_provider;
    bedrock_deregister_provider_fn     deregister_provider;
    bedrock_provider_get_config_fn     get_provider_config;
    bedrock_init_client_fn             init_client;
    bedrock_finalize_client_fn         finalize_client;
    bedrock_client_get_config_fn       get_client_config;
    bedrock_create_provider_handle_fn  create_provider_handle;
    bedrock_destroy_provider_handle_fn destroy_provider_handle;
    struct bedrock_dependency*         provider_dependencies;
    struct bedrock_dependency*         client_dependencies;
};

/**
 * @brief The following macro must be placed in a .c file
 * compiled into a shared library along wit. For example:
 *
 * static struct bedrock_module bake_module {
 *   ...
 * };
 * DEFINE_BEDROCK_C_MODULE(bake, bake_module);
 *
 * @param __name__ Name of the module.
 * @param __struct__ Name of the structure defining the module.
 *
 * Note: if the macro is called in a C++ file instead of
 * a C file, it should be preceded by extern "C".
 */
#define BEDROCK_REGISTER_MODULE(__name__, __struct__) \
    void __name__##_bedrock_init(struct bedrock_module* m) { *m = __struct__; }

/**
 * @brief Get the margo instance from the bedrock_args_t.
 *
 * @param args Arguments to the provider registration.
 *
 * @return The margo instance or MARGO_INSTANCE_NULL in case of error.
 */
margo_instance_id bedrock_args_get_margo_instance(bedrock_args_t args);

/**
 * @brief Get the pool the provider should be using.
 *
 * @param args Arguments to the provider registration.
 *
 * @return The Argobots pool or ABT_POOL_NULL in case of error.
 */
ABT_pool bedrock_args_get_pool(bedrock_args_t args);

/**
 * @brief Get the provider id that the provider should be registered with.
 *
 * @param args Arguments to the provider registration.
 *
 * @return The provider id.
 */
uint16_t bedrock_args_get_provider_id(bedrock_args_t args);

/**
 * @brief Get the JSON-formated configuration string.
 *
 * @param args Arguments to the provider registration.
 *
 * @return The config string or NULL in case of error.
 */
const char* bedrock_args_get_config(bedrock_args_t args);

/**
 * @brief Get the name by which the provider will be identified.
 *
 * @param args Arguments to the provider registration.
 *
 * @return The name, or NULL in case of error.
 */
const char* bedrock_args_get_name(bedrock_args_t args);

/**
 * @brief Get the number of dependencies of a given name in the argument.
 *
 * @param args Arguments to the provider registration.
 * @param name Name of the dependency.
 *
 * @return Number of dependencies.
 */
size_t bedrock_args_get_num_dependencies(bedrock_args_t args, const char* name);

/**
 * @brief Get the dependency associated with the name. If the dependency
 * is an array, the result can be cast into a void** that is null-terminated.
 *
 * @param args Arguments to the provider registration.
 * @param name Name of the dependency.
 * @param index Index of the dependency (relevant for arrays, otherwise use 0).
 *
 * @return The dependency entry, or NULL if there is none associated with
 * this name.
 */
void* bedrock_args_get_dependency(bedrock_args_t args, const char* name,
                                  size_t index);

#ifdef __cplusplus
}
#endif

#endif
