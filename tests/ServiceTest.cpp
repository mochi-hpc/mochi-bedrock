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

class ServiceTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( ServiceTest );
    CPPUNIT_TEST( testMakeServiceHandle );
    CPPUNIT_TEST( testSayHello );
    CPPUNIT_TEST( testComputeSum );
    CPPUNIT_TEST_SUITE_END();

    static constexpr const char* Service_config = "{ \"path\" : \"mydb\" }";
    bedrock::UUID Service_id;

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

    void testMakeServiceHandle() {
        bedrock::Client client(engine);
        std::string addr = engine.self();

        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
                "client.makeServiceHandle should not throw for valid id.",
                client.makeServiceHandle(addr, 0, Service_id));

        auto bad_id = bedrock::UUID::generate();
        CPPUNIT_ASSERT_THROW_MESSAGE(
                "client.makeServiceHandle should throw for invalid id.",
                client.makeServiceHandle(addr, 0, bad_id),
                bedrock::Exception);
        
        CPPUNIT_ASSERT_THROW_MESSAGE(
                "client.makeServiceHandle should throw for invalid provider.",
                client.makeServiceHandle(addr, 1, Service_id),
                std::exception);
        
        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
                "client.makeServiceHandle should not throw for invalid id when check=false.",
                client.makeServiceHandle(addr, 0, bad_id, false));

        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
                "client.makeServiceHandle should not throw for invalid provider when check=false.",
                client.makeServiceHandle(addr, 1, Service_id, false));
    }

    void testSayHello() {
        bedrock::Client client(engine);
        std::string addr = engine.self();
        
        bedrock::ServiceHandle my_Service = client.makeServiceHandle(addr, 0, Service_id);

        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
                "my_Service.sayHello() should not throw.",
                my_Service.sayHello());
    }

    void testComputeSum() {
        bedrock::Client client(engine);
        std::string addr = engine.self();
        
        bedrock::ServiceHandle my_Service = client.makeServiceHandle(addr, 0, Service_id);

        int32_t result = 0;
        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
                "my_Service.computeSum() should not throw.",
                my_Service.computeSum(42, 51, &result));

        CPPUNIT_ASSERT_EQUAL_MESSAGE(
                "42 + 51 should be 93",
                93, result);

        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
                "my_Service.computeSum() should not throw when passed NULL.",
                my_Service.computeSum(42, 51, nullptr));

        bedrock::AsyncRequest request;
        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
                "my_Service.computeSum() should not throw when called asynchronously.",
                my_Service.computeSum(42, 51, &result, &request));

        CPPUNIT_ASSERT_NO_THROW_MESSAGE(
                "request.wait() should not throw.",
                request.wait());
    }

};
CPPUNIT_TEST_SUITE_REGISTRATION( ServiceTest );
