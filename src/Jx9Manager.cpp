/*
 * (C) 2021 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Jx9Manager.hpp"
#include "Exception.hpp"
#include "Jx9ManagerImpl.hpp"
#include <nlohmann/json.hpp>
#include <map>
#ifdef ENABLE_MPI
#include <mpi.h>
#endif

namespace bedrock {

using nlohmann::json;

// LCOV_EXCL_START

Jx9Manager::Jx9Manager() : self(std::make_shared<Jx9ManagerImpl>()) {}

Jx9Manager::Jx9Manager(const Jx9Manager&) = default;

Jx9Manager::Jx9Manager(Jx9Manager&&) = default;

Jx9Manager& Jx9Manager::operator=(const Jx9Manager&) = default;

Jx9Manager& Jx9Manager::operator=(Jx9Manager&&) = default;

Jx9Manager::~Jx9Manager() = default;

Jx9Manager::operator bool() const { return static_cast<bool>(self); }

// LCOV_EXCL_STOP

void Jx9Manager::setVariable(const std::string& name,
                             const std::string& value) {
    std::lock_guard<tl::mutex> lock(self->m_mtx);
    self->m_global_variables[name] = value;
}

void Jx9Manager::unsetVariable(const std::string& name) {
    std::lock_guard<tl::mutex> lock(self->m_mtx);
    self->m_global_variables.erase(name);
}

static jx9_value* jx9ValueFromJson(const json& object, jx9_vm* vm);

std::string Jx9Manager::executeQuery(
    const std::string&                                  script,
    const std::unordered_map<std::string, std::string>& variables) const {
    if (!self) throw DETAILED_EXCEPTION("Calling executeQuery on invalid Jx9Manager");
    std::lock_guard<tl::mutex> lock(self->m_mtx);
    int                        ret;

    spdlog::trace("Jx9Manager about to execute the following program:\n{}", script);

    // compile script into a VM
    jx9_vm* vm;
    ret = jx9_compile(self->m_engine, script.c_str(), script.size(), &vm);
    if (ret != JX9_OK) {
        char* errLog;
        int   errLogLength;
        jx9_config(self->m_engine, JX9_CONFIG_ERR_LOG, &errLog, &errLogLength);
        auto err = std::string(errLog, errLogLength);
        if(err[errLogLength-1] == '\n') err.resize(errLogLength-1);
        throw DETAILED_EXCEPTION("Jx9 script failed to compile: {}", err);
    }

    // installing MPI_COMM_WORLD
    json comm_world = nullptr;
#ifdef ENABLE_MPI
    int mpi_rank, mpi_size;
    int mpi_initialized;
    MPI_Initialized(&mpi_initialized);
    if(mpi_initialized) {
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        comm_world = json::object();
        comm_world["rank"] = mpi_rank;
        comm_world["size"] = mpi_size;
    }
#endif
    jx9_value* jx9_comm_world = nullptr;
    try {
        jx9_comm_world = jx9ValueFromJson(comm_world, vm);
    } catch (...) {
        jx9_vm_release(vm);
        throw DETAILED_EXCEPTION("Could not create Jx9 value for MPI_COMM_WORLD");
    }
    ret = jx9_vm_config(vm, JX9_VM_CONFIG_CREATE_VAR, "MPI_COMM_WORLD",
                        jx9_comm_world);
    if (ret != JX9_OK) {
        jx9_release_value(vm, jx9_comm_world);
        jx9_vm_release(vm);
        throw DETAILED_EXCEPTION("Could not install variable \"MPI_COMM_WORLD\" in Jx9 VM");
    }
    jx9_release_value(vm, jx9_comm_world);

    // helper lambda to install variables in the VM
    auto set_variable = [&](const std::string& varname, const std::string& value) {
        jx9_value* jx9v    = nullptr;
        try {
            jx9v = jx9ValueFromJson(json::parse(value), vm);
        } catch (...) {
            jx9_vm_release(vm);
            throw DETAILED_EXCEPTION("Could not create Jx9 value from variable \"{}\"",
                            varname);
        }
        ret = jx9_vm_config(vm, JX9_VM_CONFIG_CREATE_VAR, varname.c_str(),
                            jx9v);
        if (ret != JX9_OK) {
            jx9_release_value(vm, jx9v);
            jx9_vm_release(vm);
            throw DETAILED_EXCEPTION("Could not install variable \"{}\" in Jx9 VM",
                            varname);
        }
        jx9_release_value(vm, jx9v);
    };

    // installing VM variables from Jx9Manager
    for (auto& p : self->m_global_variables) {
        if(variables.find(p.first) != variables.end())
            continue;
        set_variable(p.first, p.second);
    }

    // installing VM variables
    for (auto& p : variables) {
        set_variable(p.first, p.second);
    }

    // redirect VM output to stdout
    jx9_vm_config(vm, JX9_VM_CONFIG_OUTPUT,
                  static_cast<int (*)(const void*, unsigned, void*)>(
                      [](const void* pOutput, unsigned int nLen, void*) -> int {
                          auto s = std::string((const char*)pOutput, nLen);
                          spdlog::info("[jx9] {}", s);
                          return JX9_OK;
                      }),
                  NULL);

    // make errors appear in output
    jx9_vm_config(vm, JX9_VM_CONFIG_ERR_REPORT);

    // execute the VM
    int exit_status;
    ret = jx9_vm_exec(vm, &exit_status);
    if (ret != JX9_OK) {
        jx9_vm_release(vm);
        throw DETAILED_EXCEPTION("Jx9 VM execution failed with error code {}", ret);
    }

    // extract VM return value
    jx9_value* ret_value;
    ret = jx9_vm_config(vm, JX9_VM_CONFIG_EXEC_VALUE, &ret_value);
    if (ret != JX9_OK) {
        jx9_vm_release(vm);
        throw DETAILED_EXCEPTION("Could not extract return value from Jx9 VM");
    }

    // serialize ret_value into a string
    int         ret_string_len = 0;
    const char* ret_string = jx9_value_to_string(ret_value, &ret_string_len);
    auto        result     = std::string(ret_string, ret_string_len);
    jx9_vm_release(vm);

    spdlog::trace("Jx9 program returned the following value: {}", result);

    return result;
}

bool Jx9Manager::evaluateCondition(
        const std::string& condition,
        const std::unordered_map<std::string, std::string>& variables) const {
    std::string query = "return (";
    query += condition;
    query +=  ") == true;";
    auto result = executeQuery(query, variables);
    return result == "true";
}

static jx9_value* jx9ValueFromJson(const json& object, jx9_vm* vm) {
    jx9_value* v = nullptr;
    switch (object.type()) {
    case json::value_t::null:
        v = jx9_new_scalar(vm);
        jx9_value_null(v);
        break;
    case json::value_t::boolean:
        v = jx9_new_scalar(vm);
        jx9_value_bool(v, object.get<bool>());
        break;
    case json::value_t::string:
        v = jx9_new_scalar(vm);
        {
            auto s = object.get<std::string>();
            jx9_value_string(v, s.c_str(), s.size());
        }
        break;
    case json::value_t::number_integer:
        v = jx9_new_scalar(vm);
        jx9_value_int64(v, object.get<int64_t>());
        break;
    case json::value_t::number_unsigned:
        v = jx9_new_scalar(vm);
        jx9_value_int64(v, object.get<uint64_t>());
        break;
    case json::value_t::number_float:
        v = jx9_new_scalar(vm);
        jx9_value_double(v, object.get<double>());
        break;
    case json::value_t::object:
        v = jx9_new_array(vm);
        for (auto& entry : object.items()) {
            auto& key      = entry.key();
            auto& val      = entry.value();
            auto  jx9_elem = jx9ValueFromJson(val, vm);
            jx9_array_add_strkey_elem(v, key.c_str(), jx9_elem);
            jx9_release_value(vm, jx9_elem);
        }
        break;
    case json::value_t::array:
        v = jx9_new_array(vm);
        for (auto& entry : object) {
            auto jx9_elem = jx9ValueFromJson(entry, vm);
            jx9_array_add_elem(v, NULL, jx9_elem);
            jx9_release_value(vm, jx9_elem);
        }
        break;
    case json::value_t::binary:
        v = jx9_new_scalar(vm);
        jx9_value_null(v);
        spdlog::warn("Cannot convert binary value to Jx9");
        break;
    case json::value_t::discarded:
        v = jx9_new_scalar(vm);
        jx9_value_null(v);
        spdlog::warn("Cannot convert discarded value to Jx9");
        break;
    }
    return v;
}

} // namespace bedrock
