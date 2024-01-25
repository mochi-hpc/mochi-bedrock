/*
 * (C) 2018 The University of Chicago
 *
 * See COPYRIGHT in top-level directory.
 */
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <bedrock/Server.hpp>
#include <bedrock/Exception.hpp>
#ifdef ENABLE_SSG
  #include <ssg.h>
#endif

namespace py11 = pybind11;
using namespace pybind11::literals;
using namespace bedrock;

struct margo_instance;
typedef struct margo_instance* margo_instance_id;
typedef struct hg_addr* hg_addr_t;
typedef py11::capsule pymargo_instance_id;
typedef py11::capsule pyhg_addr_t;

#define MID2CAPSULE(__mid)   py11::capsule((void*)(__mid), "margo_instance_id", nullptr)
#define CAPSULE2MID(__caps)  (margo_instance_id)(__caps)

#define ADDR2CAPSULE(__addr)   py11::capsule((void*)(__addr), "hg_addr_t", nullptr)
#define CAPSULE2ADDR(__caps)  (hg_add_t)(__caps)

PYBIND11_MODULE(pybedrock_server, m) {
    m.doc() = "Bedrock server C++ extension";
    py11::register_exception<Exception>(m, "Exception", PyExc_RuntimeError);

    py11::class_<NamedDependency, std::shared_ptr<NamedDependency>> named_dep(m, "NamedDependency");
    named_dep
        .def_property_readonly("name",
            [](std::shared_ptr<NamedDependency> nd) { return nd->getName(); })
        .def_property_readonly("type",
            [](std::shared_ptr<NamedDependency> nd) { return nd->getType(); })
    ;

    py11::class_<ProviderDependency, std::shared_ptr<ProviderDependency>> (m, "ProviderDependency", named_dep)
        .def_property_readonly("provider_id",
            [](std::shared_ptr<ProviderDependency> nd) { return nd->getProviderID(); })
    ;

    py11::class_<Server, std::shared_ptr<Server>> server(m, "Server");

    py11::enum_<ConfigType>(server, "ConfigType")
    .value("JSON", ConfigType::JSON)
    .value("JX9", ConfigType::JX9)
    .export_values();

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
        .def("finalize",
             [](std::shared_ptr<Server> server) {
                server->finalize();
             })
        .def_property_readonly("margo_instance_id",
             [](std::shared_ptr<Server> server) {
                return MID2CAPSULE(server->getMargoManager().getMargoInstance());
             })
        .def_property_readonly("config",
             [](std::shared_ptr<Server> server) {
                return server->getCurrentConfig();
             })
        .def_property_readonly("margo_manager",
             [](std::shared_ptr<Server> server) {
                return server->getMargoManager();
             })
        .def_property_readonly("abtio_manager",
             [](std::shared_ptr<Server> server) {
                return server->getABTioManager();
             })
        .def_property_readonly("provider_manager",
             [](std::shared_ptr<Server> server) {
                return server->getProviderManager();
             })
        .def_property_readonly("ssg_manager",
             [](std::shared_ptr<Server> server) {
                return server->getSSGManager();
             })
    ;

    py11::class_<MargoManager> (m, "MargoManager")
        .def_property_readonly("margo_instance_id", [](const MargoManager& m) {
                return MID2CAPSULE(m.getMargoInstance());
        })
        .def_property_readonly("default_handler_pool", &MargoManager::getDefaultHandlerPool)
        .def_property_readonly("config", &MargoManager::getCurrentConfig)
        .def("get_pool", [](const MargoManager& m, const std::string& pool_name) {
                return m.getPool(pool_name);
        }, "name"_a)
        .def("get_pool", [](const MargoManager& m, uint32_t index) {
                return m.getPool(index);
        }, "index"_a)
        .def("add_pool", &MargoManager::addPool,
             "config"_a)
        .def("remove_pool", [](MargoManager& m, const std::string& pool_name) {
                return m.removePool(pool_name);
        }, "name"_a)
        .def("remove_pool", [](MargoManager& m, uint32_t index) {
                return m.removePool(index);
        }, "index"_a)
        .def_property_readonly("num_pools", &MargoManager::getNumPools)
        .def("get_xstream", [](const MargoManager& m, const std::string& es_name) {
                return m.getXstream(es_name);
        }, "name"_a)
        .def("get_xstream", [](const MargoManager& m, uint32_t index) {
                return m.getXstream(index);
        }, "index"_a)
        .def("add_xstream", &MargoManager::addXstream,
             "config"_a)
        .def("remove_xstream", [](MargoManager& m, const std::string& es_name) {
                return m.removeXstream(es_name);
        }, "name"_a)
        .def("remove_xstream", [](MargoManager& m, uint32_t index) {
                return m.removeXstream(index);
        }, "index"_a)
        .def_property_readonly("num_xstreams", &MargoManager::getNumXstreams)
    ;

    py11::class_<SSGManager> (m, "SSGManager")
        .def_property_readonly("config", &SSGManager::getCurrentConfig)
        .def("get_group", [](const SSGManager& ssg, const std::string& name) {
                return ssg.getGroup(name);
             }, "name_a")
        .def("get_group", [](const SSGManager& ssg, uint32_t index) {
                return ssg.getGroup(index);
             }, "index_a")
        .def("create_group",
             [](SSGManager& ssg, const std::string& config) {
                return ssg.createGroupFromConfig(config);
             }, "config"_a)
        .def("resolve_address", [](const SSGManager& ssg, const std::string& address) {
                return ADDR2CAPSULE(ssg.resolveAddress(address));
            }, "address"_a)
        .def("create_group",
             [](SSGManager& ssg,
                const std::string& name,
                const py11::dict& config,
                const std::shared_ptr<NamedDependency>& pool,
                const std::string& bootstrap_method,
                const std::string& group_file) {
#ifdef ENABLE_SSG
                ssg_group_config_t cfg = SSG_GROUP_CONFIG_INITIALIZER;
#define GET_SSG_FIELD(__field__) do { \
                if(config.contains(#__field__)) \
                    cfg.__field__ = config[#__field__].cast<decltype(cfg.__field__)>(); \
                } while(0)
                GET_SSG_FIELD(swim_period_length_ms);
                GET_SSG_FIELD(swim_suspect_timeout_periods);
                GET_SSG_FIELD(swim_subgroup_member_count);
                GET_SSG_FIELD(swim_disabled);
                GET_SSG_FIELD(ssg_credential);
#undef GET_SSG_FIELD
                return ssg.createGroup(name, &cfg, pool, bootstrap_method, group_file);
#else
                throw Exception{"Bedrock was not compiled with SSG support"};
#endif
             }, "name"_a, "config"_a=py11::dict{},
                "pool"_a=nullptr, "bootstrap_method"_a="init",
                "group_file"_a="")
    ;

    py11::class_<ABTioManager> (m, "ABTioManager")
        .def_property_readonly("config", &ABTioManager::getCurrentConfig)
        .def_property_readonly("num_abtio_instances", &ABTioManager::numABTioInstances)
        .def("get_abtio_instance", [](const ABTioManager& abtio, const std::string& name) {
            return abtio.getABTioInstance(name);
        }, "name"_a)
        .def("get_abtio_instance", [](const ABTioManager& abtio, int index) {
            return abtio.getABTioInstance(index);
        }, "index"_a)
        .def("add_abtio_instance", &ABTioManager::addABTioInstance)
    ;

    py11::class_<ProviderDescriptor> (m, "ProviderDescriptor")
        .def(py11::init<const std::string&, const std::string&, uint16_t>())
        .def_readonly("name", &ProviderDescriptor::name)
        .def_readonly("type", &ProviderDescriptor::type)
        .def_readonly("provider_id", &ProviderDescriptor::provider_id)
    ;

    py11::class_<ProviderManager> (m, "ProviderManager")
        .def_property_readonly("config", &ProviderManager::getCurrentConfig)
        .def("lookup_provider", &ProviderManager::lookupProvider,
             "spec"_a)
        .def_property_readonly("providers", &ProviderManager::listProviders)
        /* // need to expose ResolvedDependencyMap
        .def("register_provider",
             [](ProviderManager& manager,
                const ProviderDescriptor&       descriptor,
                const std::string&              pool_name,
                const std::string&              config,
                const ResolvedDependencyMap&    dependencies,
                const std::vector<std::string>& tags) {
            ...
        })
        */
        .def("deregister_provider",
             &ProviderManager::deregisterProvider,
             "spec"_a)
        .def("add_providers_from_json",
             &ProviderManager::addProviderFromJSON,
             "json_config"_a)
        .def("add_provider_list_from_json",
             &ProviderManager::addProviderListFromJSON,
             "json_configs"_a)
        .def("change_pool",
             &ProviderManager::changeProviderPool,
             "provider"_a, "pool"_a)
        .def("migrate_provider",
             &ProviderManager::migrateProvider,
             "provider"_a, "dest_addr"_a, "dest_provider_id"_a,
             "migration_config"_a, "remove_source"_a)
        .def("snapshot_provider",
             &ProviderManager::snapshotProvider,
             "provider"_a, "dest_path"_a, "snapshot_config"_a, "remove_source"_a)
        .def("restore_provider",
             &ProviderManager::restoreProvider,
             "provider"_a, "src_path"_a, "restore_config"_a)
    ;
}
