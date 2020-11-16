/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include "DummyBackend.hpp"
#include <iostream>

BEDROCK_REGISTER_BACKEND(dummy, DummyService);

void DummyService::sayHello() {
    std::cout << "Hello World" << std::endl;
}

bedrock::RequestResult<int32_t> DummyService::computeSum(int32_t x, int32_t y) {
    bedrock::RequestResult<int32_t> result;
    result.value() = x + y;
    return result;
}

bedrock::RequestResult<bool> DummyService::destroy() {
    bedrock::RequestResult<bool> result;
    result.value() = true;
    // or result.success() = true
    return result;
}

std::unique_ptr<bedrock::Backend> DummyService::create(const thallium::engine& engine, const json& config) {
    (void)engine;
    return std::unique_ptr<bedrock::Backend>(new DummyService(config));
}

std::unique_ptr<bedrock::Backend> DummyService::open(const thallium::engine& engine, const json& config) {
    (void)engine;
    return std::unique_ptr<bedrock::Backend>(new DummyService(config));
}
