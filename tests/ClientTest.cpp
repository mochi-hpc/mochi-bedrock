/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <cppunit/extensions/HelperMacros.h>
#include <bedrock/Client.hpp>
#include <bedrock/Admin.hpp>

extern thallium::engine engine;
extern std::string Service_type;

class ClientTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( ClientTest );
    CPPUNIT_TEST( testOpenService );
    CPPUNIT_TEST_SUITE_END();

    static constexpr const char* Service_config = "{ \"path\" : \"mydb\" }";
    UUID Service_id;

    public:

    void setUp() {
        bedrock::Admin admin(engine);
        std::string addr = engine.self();
        Service_id = admin.createService(addr, 0, Service_type, Service_config);
    }

    void tearDown() {
        bedrock::Admin admin(engine);
        std::string addr = engine.self();
        admin.destroyService(addr, 0, Service_id);
    }

    void testOpenService() {
        bedrock::Client client(engine);
        std::string addr = engine.self();
        
        Service my_Service = client.open(addr, 0, Service_id);
        CPPUNIT_ASSERT_MESSAGE(
                "Service should be valid",
                static_cast<bool>(my_Service));

        auto bad_id = UUID::generate();
        CPPUNIT_ASSERT_THROW_MESSAGE(
                "client.open should fail on non-existing Service",
                client.open(addr, 0, bad_id);
    }
};
CPPUNIT_TEST_SUITE_REGISTRATION( ClientTest );
