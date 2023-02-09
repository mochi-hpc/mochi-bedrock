/*
 * (C) 2020 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#ifndef __BEDROCK_EXCEPTION_HPP
#define __BEDROCK_EXCEPTION_HPP

#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <exception>
#include <string>

namespace bedrock {

class Exception : public std::exception {

    std::string m_error;

  public:
    template <typename... Args>
    Exception(Args&&... args)
    : m_error(fmt::format(std::forward<Args>(args)...)) {}

    virtual const char* what() const noexcept override {
        return m_error.c_str();
    }

    virtual const char* details() const noexcept {
        return "";
    }
};

} // namespace bedrock

#endif
