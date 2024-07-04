/*
 * (C) 2023 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_FORMATTING_H
#define __BEDROCK_FORMATTING_H

#include <mercury.h>
#include <string>
#ifdef ENABLE_FLOCK
#include <flock/flock-common.h>
#endif

namespace std {

static inline auto to_string(hg_return_t ret) -> string {
#define X(__name__) case __name__: return #__name__;
    switch(ret) {
        HG_RETURN_VALUES
        default:
            return "<unknown>";
    }
#undef X
}

#ifdef ENABLE_FLOCK
static inline auto to_string(flock_return_t ret) -> string {
#define X(__name__, __description__) case __name__: return __description__;
    switch(ret) {
        FLOCK_RETURN_VALUES
        default:
            return "<unknown>";
    }
#undef X
}
#endif

}

#endif
