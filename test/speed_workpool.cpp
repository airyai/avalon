#include <boost/test/unit_test.hpp>

#include <stdio.h>
#include <time.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/timer.hpp>

#include "../thread/asyncresult.h"
#include "../thread/workpool.h"
#include "../errors.h"

BOOST_AUTO_TEST_SUITE (workpool)

using namespace avalon::thread;
using namespace avalon;

void dummy(AsyncResult&) {}

BOOST_AUTO_TEST_CASE( mutex )
{
    {
        AsyncResult ar(dummy);
        boost::timer timer;
        printf ("Testing AsyncResult::Lock speed in one thread ... ");
        
        int loop = 10000000;
        for (int i=0; i<loop; i++) {
            AsyncResult::Status status = ar.status();
        }
        printf ("%lfs.", timer.elapsed() );
    }
}

BOOST_AUTO_TEST_SUITE_END()
