/*
 * (C) 2021 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include "bedrock/Jx9Manager.hpp"
#include "bedrock/Exception.hpp"
#include "Jx9ManagerImpl.hpp"
#include <map>

namespace bedrock {

Jx9Manager::Jx9Manager()
: self(std::make_shared<Jx9ManagerImpl>()) {}

Jx9Manager::Jx9Manager(const Jx9Manager&) = default;

Jx9Manager::Jx9Manager(Jx9Manager&&) = default;

Jx9Manager& Jx9Manager::operator=(const Jx9Manager&) = default;

Jx9Manager& Jx9Manager::operator=(Jx9Manager&&) = default;

Jx9Manager::~Jx9Manager() = default;

Jx9Manager::operator bool() const { return static_cast<bool>(self); }

std::string Jx9Manager::executeQuery(
        const std::string& script,
        const std::vector<std::string>& args) const {
    if(!self) throw Exception("Calling executeQuery on invalid Jx9Manager");
    std::lock_guard<tl::mutex> lock(self->m_mtx);
    int ret;

    // compile script into a VM
    jx9_vm* vm;
    ret = jx9_compile(self->m_engine, script.c_str(), script.size(), &vm);
    if (ret != JX9_OK) {
        char* errLog;
        int errLogLength;
        jx9_config(self->m_engine, JX9_CONFIG_ERR_LOG, &errLog, &errLogLength);
        throw Exception("Jx9 script failed to compile: {}", std::string(errLog, errLogLength));
    }

    // installing VM arguments
    for(auto& arg : args) {
        ret = jx9_vm_config(vm, JX9_VM_CONFIG_ARGV_ENTRY, arg.c_str());
        if (ret != JX9_OK) {
            jx9_vm_release(vm);
            throw Exception("Failed to install Jx9 VM argument");
        }
    }

    // redirect VM output to stdout
    jx9_vm_config(vm, JX9_VM_CONFIG_OUTPUT,
        static_cast<int(*)(const void*, unsigned, void*)>(
        [](const void* pOutput, unsigned int nLen, void*) -> int {
            auto s = std::string((const char*)pOutput, nLen);
            spdlog::info("[jx9] {}", s);
            return JX9_OK;
        }), NULL);

    // make errors appear in output
    jx9_vm_config(vm, JX9_VM_CONFIG_ERR_REPORT);

    // execute the VM
    int exit_status;
    ret = jx9_vm_exec(vm, &exit_status);
    if (ret != JX9_OK) {
        jx9_vm_release(vm);
        throw Exception("Jx9 VM execution failed with error code {}", ret);
    }

    // extract VM return value
    jx9_value* ret_value;
    ret = jx9_vm_config(vm, JX9_VM_CONFIG_EXEC_VALUE, &ret_value);
    if (ret != JX9_OK) {
        jx9_vm_release(vm);
        throw Exception("Could not extract return value from Jx9 VM");
    }

    // serialize ret_value into a string
    int ret_string_len = 0;
    const char* ret_string = jx9_value_to_string(ret_value, &ret_string_len);
    auto result = std::string(ret_string, ret_string_len);
    jx9_vm_release(vm);

    spdlog::debug("Jx9 sending back {}", result);
    return result;
}

} // namespace bedrock
