#include <boost/test/unit_test.hpp>

#include <stdio.h>
#include <string>
#include <boost/function.hpp>
#include <boost/bind.hpp>

BOOST_AUTO_TEST_SUITE (pre_condition)

class BoostBindParent
{
public:
    virtual int f() {
        return 1;
    }
};

class BoostBindChild : public BoostBindParent
{
public:
    virtual int f() {
        return 2;
    }
};

BOOST_AUTO_TEST_CASE( boost_bind_virtual_method )
{
    BoostBindChild ch;
    BoostBindParent &pa = ch;
    
    BOOST_REQUIRE( boost::bind(&BoostBindParent::f, &pa)() == 2 );
    BOOST_REQUIRE( boost::bind(&BoostBindParent::f, &ch)() == 2 );
    BOOST_REQUIRE( boost::bind(&BoostBindChild::f, &ch)() == 2 );
}

#define DefineStringValue(x) #x
BOOST_AUTO_TEST_CASE( define_string_value )
{
    BOOST_REQUIRE( DefineStringValue(hello world!) == std::string("hello world!") );
}

BOOST_AUTO_TEST_SUITE_END()
