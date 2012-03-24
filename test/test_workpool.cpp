#include <boost/test/unit_test.hpp>

#include <stdio.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "../thread/asyncresult.h"
#include "../errors.h"

BOOST_AUTO_TEST_SUITE (workpool)

using namespace avalon::thread;
using namespace avalon;

void f(int &i, int j)
{
    i = j;
}

void g(int &i, int j) 
{
    AVALON_THROW(AvalonException);
}

BOOST_AUTO_TEST_CASE( async_result )
{
    {
        int i = 0, j = 0, k = 0, l = 0, m = 0;
        AsyncResult ar(boost::bind(f, boost::ref(i), 0xdeadbeef));
        ar.add_success(boost::bind(f, boost::ref(j), 1));
        ar.add_error(boost::bind(f, boost::ref(k), 1));
        ar.add_cancel(boost::bind(f, boost::ref(l), 1));
        ar.add_all(boost::bind(f, boost::ref(m), 1));
        ar.do_task();
        
        BOOST_REQUIRE( i == 0xdeadbeef );
        BOOST_REQUIRE( j == 1 && k == 0 && l == 0 && m == 1 );
    }
    {
        int i = 0, j = 0, k = 0, l = 0, m = 0;
        AsyncResult ar(boost::bind(g, boost::ref(i), 0xdeadbeef));
        ar.add_success(boost::bind(f, boost::ref(j), 1));
        ar.add_error(boost::bind(f, boost::ref(k), 1));
        ar.add_cancel(boost::bind(f, boost::ref(l), 1));
        ar.add_all(boost::bind(f, boost::ref(m), 1));
        ar.do_task();
        
        BOOST_REQUIRE( i == 0 );
        BOOST_REQUIRE( j == 0 && k == 1 && l == 0 && m == 1 );
    }
    {
        int i = 0, j = 0, k = 0, l = 0, m = 0;
        AsyncResult ar(boost::bind(g, boost::ref(i), 0xdeadbeef));
        ar.add_success(boost::bind(f, boost::ref(j), 1));
        ar.add_error(boost::bind(f, boost::ref(k), 1));
        ar.add_cancel(boost::bind(f, boost::ref(l), 1));
        ar.add_all(boost::bind(f, boost::ref(m), 1));
        ar.cancel();
        ar.do_task();
        
        BOOST_REQUIRE( i == 0 );
        BOOST_REQUIRE( j == 0 && k == 0 && l == 1 && m == 1 );
    }
    {
        int i = 0, j = 0, k = 0, l = 0, m = 0;
        {
            AsyncResult ar(boost::bind(g, boost::ref(i), 0xdeadbeef));
            ar.add_success(boost::bind(f, boost::ref(j), 1));
            ar.add_error(boost::bind(f, boost::ref(k), 1));
            ar.add_cancel(boost::bind(f, boost::ref(l), 1));
            ar.add_all(boost::bind(f, boost::ref(m), 1));
        }
        
        BOOST_REQUIRE( i == 0 );
        BOOST_REQUIRE( j == 0 && k == 0 && l == 1 && m == 1 );
    }
}

BOOST_AUTO_TEST_SUITE_END()
