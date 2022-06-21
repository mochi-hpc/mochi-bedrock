/*
 * (C) 2022 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_CLIENT_H
#define __BEDROCK_CLIENT_H

#include <margo.h>
#include <bedrock/common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* bedrock_client_t;

/**
 * @brief Create a Bedrock client handle.
 *
 * @param[out] client Client
 *
 * @return 0 for success
 */
int bedrock_client_init(margo_instance_id mid, bedrock_client_t* client);

/**
 * @brief Finalize a Bedrock client handle.
 *
 * @param[int] client Client
 *
 * @return 0 for success
 */
int bedrock_client_finalize(bedrock_client_t client);

#ifdef __cplusplus
}
#endif

#endif
