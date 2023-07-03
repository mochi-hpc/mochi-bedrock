/*
 * (C) 2023 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_FORMATTING_H
#define __BEDROCK_FORMATTING_H

#include <mercury.h>
#include <string>

namespace std {

static inline auto to_string(hg_return_t ret) -> string {
#define X(__name__) case __name__: return #__name__;
    switch(ret) {
        HG_RETURN_VALUES
        default:
            return "<unknown>";
    }
}

}

#endif
