/*
 * (C) 2020 The University of Chicago
 * 
 * See COPYRIGHT in top-level directory.
 */
#include <bedrock/Admin.hpp>
#include <cppunit/extensions/HelperMacros.h>

extern thallium::engine engine;
extern std::string Service_type;

class AdminTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( AdminTest );
    CPPUNIT_TEST( testAdminCreateService );
    CPPUNIT_TEST_SUITE_END();

    static constexpr const char* Service_config = "{ \"path\" : \"mydb\" }";

    public:

    void setUp() {}
    void tearDown() {}

    void testAdminCreateService() {
        bedrock::Admin admin(engine);
        std::string addr = engine.self();

        bedrock::UUID Service_id;
        // Create a valid Service
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("admin.createService should return a valid Service",
                Service_id = admin.createService(addr, 0, Service_type, Service_config));

        // Create a Service with a wrong backend type
        bedrock::UUID bad_id;
        CPPUNIT_ASSERT_THROW_MESSAGE("admin.createService should throw an exception (wrong backend)",
                bad_id = admin.createService(addr, 0, "blabla", Service_config),
                bedrock::Exception);

        // Destroy the Service
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("admin.destroyService should not throw on valid Service",
            admin.destroyService(addr, 0, Service_id));

        // Destroy an invalid Service
        CPPUNIT_ASSERT_THROW_MESSAGE("admin.destroyService should throw on invalid Service",
            admin.destroyService(addr, 0, bad_id),
            bedrock::Exception);
    }
};
CPPUNIT_TEST_SUITE_REGISTRATION( AdminTest );
