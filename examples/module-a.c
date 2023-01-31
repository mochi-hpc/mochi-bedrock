/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/module.h>
#include <stdlib.h>
#include <string.h>

static int module_a_register_provider(
        bedrock_args_t args,
        bedrock_module_provider_t* provider)
{
    margo_instance_id mid = bedrock_args_get_margo_instance(args);
    uint16_t provider_id  = bedrock_args_get_provider_id(args);
    ABT_pool pool         = bedrock_args_get_pool(args);
    const char* config    = bedrock_args_get_config(args);
    const char* name      = bedrock_args_get_name(args);

    *provider = strdup("module_a:provider");
    printf("Registered a provider from module A\n");
    printf(" -> mid         = %p\n", (void*)mid);
    printf(" -> provider id = %d\n", provider_id);
    printf(" -> pool        = %p\n", (void*)pool);
    printf(" -> config      = %s\n", config);
    printf(" -> name        = %s\n", name);
    return BEDROCK_SUCCESS;
}

static int module_a_deregister_provider(
        bedrock_module_provider_t provider)
{
    free(provider);
    printf("Deregistered a provider from module A\n");
    return BEDROCK_SUCCESS;
}

static char* module_a_get_provider_config(
        bedrock_module_provider_t provider) {
    (void)provider;
    return strdup("{}");
}

static int module_a_init_client(
        bedrock_args_t args,
        bedrock_module_client_t* client)
{
    margo_instance_id mid = bedrock_args_get_margo_instance(args);
    *client = strdup("module_a:client");
    printf("Registered a client from module A\n");
    printf(" -> mid = %p\n", (void*)mid);
    return BEDROCK_SUCCESS;
}

static int module_a_finalize_client(
        bedrock_module_client_t client)
{
    free(client);
    printf("Finalized a client from module A\n");
    return BEDROCK_SUCCESS;
}

static char* module_a_get_client_config(
        bedrock_module_provider_t provider) {
    (void)provider;
    return strdup("{}");
}

static int module_a_create_provider_handle(
        bedrock_module_client_t client,
        hg_addr_t address,
        uint16_t provider_id,
        bedrock_module_provider_handle_t* ph)
{
    (void)client;
    (void)address;
    (void)provider_id;
    *ph = strdup("module_a:provider_handle");
    printf("Created provider handle from module A\n");
    return BEDROCK_SUCCESS;
}

static int module_a_destroy_provider_handle(
        bedrock_module_provider_handle_t ph)
{
    free(ph);
    printf("Destroyed provider handle from module A\n");
    return BEDROCK_SUCCESS;
}

static struct bedrock_dependency client_dependencies[] = {
    { "some_group", "ssg", BEDROCK_OPTIONAL },
    BEDROCK_NO_MORE_DEPENDENCIES
};

static struct bedrock_module_v1 module_a = {
    .api_version             = 1,
    .register_provider       = module_a_register_provider,
    .deregister_provider     = module_a_deregister_provider,
    .get_provider_config     = module_a_get_provider_config,
    .init_client             = module_a_init_client,
    .finalize_client         = module_a_finalize_client,
    .get_client_config       = module_a_get_client_config,
    .create_provider_handle  = module_a_create_provider_handle,
    .destroy_provider_handle = module_a_destroy_provider_handle,
    .provider_dependencies   = NULL,
    .client_dependencies     = client_dependencies
};

BEDROCK_REGISTER_MODULE_WITH_VERSION(module_a, module_a, 1)
