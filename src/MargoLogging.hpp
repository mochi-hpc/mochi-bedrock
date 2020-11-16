/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_MARGO_LOGGING_H
#define __BEDROCK_MARGO_LOGGING_H

#include <margo.h>

namespace bedrock {

void setupMargoLogging();

void setupMargoLoggingForInstance(margo_instance_id mid);

} // namespace bedrock

#endif
