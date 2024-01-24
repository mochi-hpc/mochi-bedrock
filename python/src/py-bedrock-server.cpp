/*
 * (C) 2018 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <bedrock/Server.hpp>
#include <bedrock/Exception.hpp>

namespace py11 = pybind11;
using namespace pybind11::literals;
using namespace bedrock;

struct margo_instance;
typedef struct margo_instance* margo_instance_id;
typedef py11::capsule pymargo_instance_id;

#define MID2CAPSULE(__mid)   py11::capsule((void*)(__mid), "margo_instance_id", nullptr)
#define CAPSULE2MID(__caps)  (margo_instance_id)(__caps)

PYBIND11_MODULE(pybedrock_server, m) {
    m.doc() = "Bedrock server C++ extension";
    py11::register_exception<Exception>(m, "Exception", PyExc_RuntimeError);
    py11::class_<Server, std::shared_ptr<Server>> server(m, "Server");
        server
        .def(py11::init([](const std::string& address, const std::string& config,
                           ConfigType configType,
                           const Jx9ParamMap& jx9Params) {
            return std::make_shared<Server>(address, config, configType, jx9Params);
        }), "address"_a, "config"_a="{}", "config_type"_a=ConfigType::JSON,
            "jx9_params"_a=Jx9ParamMap{})
        .def("wait_for_finalize",
             [](std::shared_ptr<Server> server) {
                server->waitForFinalize();
             })
    ;
    py11::enum_<ConfigType>(server, "ConfigType")
    .value("JSON", ConfigType::JSON)
    .value("JX9", ConfigType::JX9)
    .export_values();
}
