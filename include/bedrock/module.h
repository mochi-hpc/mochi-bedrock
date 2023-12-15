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

#define BEDROCK_MODULE_API_VERSION 1

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
typedef char* (*bedrock_provider_get_config_fn)(bedrock_module_provider_t);

/**
 * @brief Type of function called to change the pool associated with a provider.
 * The returned value must be 0 in case of success, any other value in case of
 * failure.
 *
 * @param bedrock_module_provider_t Provider.
 * @param ABT_pool new pool.
 *
 * @return 0 in case of success.
 */
typedef int (*bedrock_provider_change_pool_fn)(bedrock_module_provider_t, ABT_pool);

/**
 * @brief Type of function called to get the dependencies of a provider
 * from a given configuration. This function should allocate the bedrock_dependency
 * array using malloc, as well as the name and type fields in the dependencies.
 * The caller will be responsible for calling free on these fields and on the
 * dependency array. The integer parameter should be set to the size of the array.
 */
typedef int (*bedrock_provider_get_dependencies_fn)(
        const char*,
        struct bedrock_dependency**,
        int*);

/**
 * @brief Type of function called to migrate the state of a provider to
 * another provider.
 */
typedef int (*bedrock_provider_migrate_fn)(
        bedrock_module_provider_t,
        const char*, /* destination address */
        uint16_t,    /* destination provider ID */
        const char*, /* JSON options */
        bool);       /* remove source */

/**
 * @brief Type of function called to snapshot the state of a provider to
 * a given path.
 */
typedef int (*bedrock_provider_snapshot_fn)(
        bedrock_module_provider_t,
        const char*, /* destination path */
        const char*, /* JSON options */
        bool);       /* remove source */

/**
 * @brief Type of function called to restore the state of a provider
 * from a given path.
 */
typedef int (*bedrock_provider_restore_fn)(
        bedrock_module_provider_t,
        const char*,  /* source path */
        const char*); /* JSON options */

/**
 * @brief Type of function called to get the configuration of a client.
 * The returned string, if not NULL, should be freed by the caller.
 *
 * @param bedrock_module_client_t Client.
 *
 * @return null-terminated configuration string.
 */
typedef char* (*bedrock_client_get_config_fn)(bedrock_module_client_t);

/**
 * @brief Type of function called to get the dependencies of a provider
 * from a given configuration. This function should allocate the bedrock_dependency
 * array using malloc, as well as the name and type fields in the dependencies.
 * The caller will be responsible for calling free on these fields and on the
 * dependency array. The integer parameter should be set to the size of the array.
 */
typedef int (*bedrock_client_get_dependencies_fn)(
        const char*,
        struct bedrock_dependency**,
        int*);

/**
 * @brief A global instance of a bedrock_module_vX structure must be provided
 * in a shared library to make up a Bedrock module.
 * The provider_dependencies and client_dependencies arrays should be terminated
 * by a BEDROCK_NO_MORE_DEPENDENCIES.
 *
 * @warning The api_version field should always match the version of the
 * bedrock_module_vX structure used!
 */
struct bedrock_module_v1 {
    int api_version; // should always be set to 1
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

struct bedrock_module_v2 {
    /* v1 fields */
    int api_version; // should always be set to 1
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
    /* v2 fields */
    bedrock_provider_change_pool_fn    change_provider_pool;
};

struct bedrock_module_v3 {
    /* v1 fields */
    int api_version; // should always be set to 3
    bedrock_register_provider_fn       register_provider;
    bedrock_deregister_provider_fn     deregister_provider;
    bedrock_provider_get_config_fn     get_provider_config;
    bedrock_init_client_fn             init_client;
    bedrock_finalize_client_fn         finalize_client;
    bedrock_client_get_config_fn       get_client_config;
    bedrock_create_provider_handle_fn  create_provider_handle;
    bedrock_destroy_provider_handle_fn destroy_provider_handle;
    /* note: the bellow fields are going to be the default if
     * bedrock_get_provider/client_dependencies_fn are not provided */
    struct bedrock_dependency*         provider_dependencies;
    struct bedrock_dependency*         client_dependencies;
    /* v2 fields */
    bedrock_provider_change_pool_fn change_provider_pool;
    /* v3 fields */
    bedrock_provider_snapshot_fn         snapshot_provider;
    bedrock_provider_restore_fn          restore_provider;
    bedrock_provider_migrate_fn          migrate_provider;
    bedrock_provider_get_dependencies_fn get_provider_dependencies;
    bedrock_client_get_dependencies_fn   get_client_dependencies;
};

/**
 * @brief The following macro must be placed in a .c file
 * compiled into a shared library along wit. For example:
 *
 * static struct bedrock_module_v1 bake_module {
 *   ...
 * };
 * BEDROCK_REGISTER_MODULE_WITH_VERSION(bake, bake_module, 1);
 *
 * @param __name__ Name of the module.
 * @param __struct__ Name of the structure defining the module.
 *
 * Note: if the macro is called in a C++ file instead of
 * a C file, it should be preceded by extern "C".
 */
#define BEDROCK_REGISTER_MODULE_WITH_VERSION(__name__, __struct__, __v__) \
    void __name__##_bedrock_init(struct bedrock_module_v1** m) { \
        *m = (struct bedrock_module_v1*)(&(__struct__));\
        (*m)->api_version = __v__;\
    }

/**
 * @warning This structure is deprecated.
 * Please use one of the bedrock_module_vX structures.
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
 * @warning this macro is deprecated in favor of BEDROCK_REGISTER_MODULE_WITH_VERSION.
 */
#define BEDROCK_REGISTER_MODULE(__name__, __struct__)                         \
    void __name__##_bedrock_init(struct bedrock_module_v1** m) {              \
        static struct bedrock_module_v1 v1 = {0};                             \
        if(v1.api_version == 0) {                                             \
            v1.api_version = 1;                                               \
            v1.register_provider = (__struct__).register_provider;            \
            v1.deregister_provider = (__struct__).deregister_provider;        \
            v1.get_provider_config = (__struct__).get_provider_config;        \
            v1.init_client = (__struct__).init_client;                        \
            v1.finalize_client = (__struct__).finalize_client;                \
            v1.get_client_config = (__struct__).get_client_config;            \
            v1.create_provider_handle = (__struct__).create_provider_handle;  \
            v1.destroy_provider_handle = (__struct__).destroy_provider_handle;\
            v1.provider_dependencies = (__struct__).provider_dependencies;    \
            v1.client_dependencies = (__struct__).client_dependencies;        \
        }                                                                     \
        *m = &v1;                                                             \
    }

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

/**
 * @brief Returns the number of tags specified in the configuration for this provider.
 *
 * @param args Arguments to the provider registration.
 *
 * @return The number of tags.
 */
size_t bedrock_args_get_num_tags(bedrock_args_t args);

/**
 * @brief Get the tag at the specified index.
 *
 * @param args Arguments to the provider registration.
 * @param index Index of the tag to retrieve.
 *
 * @return Null-terminated string of the tag, or NULL if not found.
 */
const char* bedrock_args_get_tag(bedrock_args_t args, size_t index);

#ifdef __cplusplus
}
#endif

#endif
