/*
 * (C) 2022 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_SERVICE_HANDLE_H
#define __BEDROCK_SERVICE_HANDLE_H

#include <bedrock/client.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* bedrock_service_t;

/**
 * @brief Create a service handle pointing to a specific
 * Bedrock process.
 *
 * @param client Client
 * @param addr Address of the Bedrock process
 * @param provider_id Provider id of the Bedrock process
 * @param sh Resulting service handle
 *
 * @return 0 on success
 */
int bedrock_service_handle_create(
        bedrock_client_t client,
        const char* addr,
        uint16_t provider_id,
        bedrock_service_t* sh);

/**
 * @brief Destroys a service handle.
 *
 * @param sh Service handle
 *
 * @return 0 on success
 */
int bedrock_service_handle_destroy(
        bedrock_service_t sh);

/**
 * @brief Request the execution of a JX9 script on
 * the remote service, to query its configuration.
 *
 * IMPORTANT: the caller is responsible for calling
 * free() on the returned string.
 *
 * @param script JX9 script
 *
 * @return 0 on success
 */
char* bedrock_service_query_config(
        bedrock_service_t sh,
        const char* script);

#ifdef __cplusplus
}
#endif

#endif
