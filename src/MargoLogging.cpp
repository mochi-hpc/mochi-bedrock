/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "MargoLogging.hpp"
#include <margo-logging.h>
#include <spdlog/spdlog.h>

namespace bedrock {

static void spdlog_trace(void* uargs, const char* message) {
    (void)uargs;
    spdlog::trace(message);
}

static void spdlog_debug(void* uargs, const char* message) {
    (void)uargs;
    spdlog::debug(message);
}

static void spdlog_info(void* uargs, const char* message) {
    (void)uargs;
    spdlog::info(message);
}

static void spdlog_warning(void* uargs, const char* message) {
    (void)uargs;
    spdlog::warn(message);
}

static void spdlog_error(void* uargs, const char* message) {
    (void)uargs;
    spdlog::error(message);
}

static void spdlog_critical(void* uargs, const char* message) {
    (void)uargs;
    spdlog::critical(message);
}

void setupMargoLogging() {

    struct margo_logger logger;
    logger.uargs    = NULL;
    logger.trace    = spdlog_trace;
    logger.debug    = spdlog_debug;
    logger.info     = spdlog_info;
    logger.warning  = spdlog_warning;
    logger.error    = spdlog_error;
    logger.critical = spdlog_critical;

    margo_set_global_logger(&logger);
    margo_set_global_log_level(MARGO_LOG_EXTERNAL);
}

void setupMargoLoggingForInstance(margo_instance_id mid) {

    struct margo_logger logger;
    logger.uargs    = NULL;
    logger.trace    = spdlog_trace;
    logger.debug    = spdlog_debug;
    logger.info     = spdlog_info;
    logger.warning  = spdlog_warning;
    logger.error    = spdlog_error;
    logger.critical = spdlog_critical;

    margo_set_logger(mid, &logger);
    margo_set_log_level(mid, MARGO_LOG_EXTERNAL);
}

} // namespace bedrock
