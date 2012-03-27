#include <boost/test/unit_test.hpp>

#include <stdio.h>
#include <time.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "../thread/asyncresult.h"
#include "../thread/workpool.h"
#include "../errors.h"

BOOST_AUTO_TEST_SUITE (workpool)

using namespace avalon::thread;
using namespace avalon;

const int BIT_1 = 0x1, BIT_2 = 0x2, BIT_3 = 0x4, BIT_4 = 0x8, BIT_5 = 0x10, 
        BIT_6 = 0x20, BIT_7 = 0x40, BIT_8 = 0x80, BIT_9 = 0x100;
        
class IntModify {
public:
    IntModify(int& i, int enter, int leave)
     :  i_(i), enter_(enter), leave_(leave) {
         i_ += enter;
     }
    ~IntModify() {
        i_ += leave_;
    }
protected:
    int &i_;
    int enter_, leave_;
};

void f(AsyncResult& ar)
{
    ar.set_result<int>(new int(BIT_1));
}

void g(AsyncResult& ar) 
{
    AVALON_THROW_INFO(AvalonException, error_number(0xdeadbeef) );
}

void h(AsyncResult& ar)
{
    throw boost::thread_interrupted();
}

void cb(AsyncResult &ar)
{
    boost::shared_ptr<int> i = ar.get_result<int>();
    if (!i) {
        i.reset(new int(0));
        ar.set_result<int>(i);
    }
    
    switch (ar.status())
    {
        case AsyncResult::SUCCESS:
            *i += BIT_2;
            break;
        case AsyncResult::ERROR:
            *i += BIT_3;
            break;
        case AsyncResult::CANCELLED:
            *i += BIT_4;
            break;
        case AsyncResult::INTERRUPTED:
            *i += BIT_5;
            break;
        default:
            *i += BIT_9;
            break;
    }
}

void cb2(AsyncResult &ar, bool& flag)
{
    flag = true;
}

#define MAKE_ASYNC_RESULT(f) \
    AsyncResult ar(f); \
    bool flag = false; \
    ar.add_success(cb); \
    ar.add_error(cb); \
    ar.add_cancel(cb); \
    ar.add_interrupt(cb); \
    ar.add_all(boost::bind(cb2, _1, boost::ref(flag)));
    
#define CHECK_RESULT(ar, v) ( (ar.get_result<int>()) && (*ar.get_result<int>() == (v)))

BOOST_AUTO_TEST_CASE( workpool )
{
}

BOOST_AUTO_TEST_CASE( async_result_data )
{
    int i = 0;
    {
        AsyncResult ar(f);
        ar.set_result<IntModify>(new IntModify(i, BIT_1, BIT_2));
    }
    BOOST_CHECK_MESSAGE( i == (BIT_1 | BIT_2),
                         "boost::shared_ptr<base> cannot destruct child instance!" );
    
}

BOOST_AUTO_TEST_CASE( async_result )
{
    // test basic AsyncResult.
    {
        MAKE_ASYNC_RESULT(f);
        BOOST_CHECK( ar.execute() );
        BOOST_CHECK( ar.wait() );
        
        BOOST_CHECK( ar.status() == AsyncResult::SUCCESS );
        BOOST_CHECK( CHECK_RESULT(ar, BIT_1 + BIT_2) );
        BOOST_CHECK( flag );
    }
    // test exception.
    {
        MAKE_ASYNC_RESULT(g);
        BOOST_CHECK( ar.execute() );
        BOOST_CHECK( ar.wait() );
        
        BOOST_CHECK( ar.status() == AsyncResult::ERROR );
        BOOST_CHECK( CHECK_RESULT(ar, BIT_3) );
        BOOST_CHECK( flag );
                
        BOOST_CHECK( ar.exception() );
        if (ar.exception()) {
            const int* error_no = boost::get_error_info<error_number>(*ar.exception());
            BOOST_CHECK( error_no && *error_no == 0xdeadbeef );
        }
    }
    // test canellation.
    {
        MAKE_ASYNC_RESULT(f);
        ar.cancel();
        BOOST_CHECK( !ar.execute() );
        BOOST_CHECK( ar.wait() );
        
        BOOST_CHECK( ar.status() == AsyncResult::CANCELLED );
        BOOST_CHECK( CHECK_RESULT(ar, BIT_4) );
        BOOST_CHECK( flag );
    }
    // test interrupt.
    {
        MAKE_ASYNC_RESULT(h);
        BOOST_CHECK_THROW( ar.execute(), boost::thread_interrupted );
        BOOST_CHECK( ar.wait() );
        
        BOOST_CHECK( ar.status() == AsyncResult::INTERRUPTED );
        BOOST_CHECK( CHECK_RESULT(ar, BIT_5) );
        BOOST_CHECK( flag );
    }
    // test wait timeout.
    {
        MAKE_ASYNC_RESULT(f);
        BOOST_CHECK( !ar.wait(100) );
        
        BOOST_CHECK( ar.status() == AsyncResult::WAIT );
        BOOST_CHECK( !ar.get_result<int>() );
        BOOST_CHECK( !flag );
    }
    // test add callback later on.
    {
        MAKE_ASYNC_RESULT(f);
        BOOST_CHECK( ar.execute() );
        BOOST_CHECK( ar.wait() );
        
        BOOST_CHECK( ar.status() == AsyncResult::SUCCESS );
        BOOST_CHECK( CHECK_RESULT(ar, BIT_1 + BIT_2) );
        BOOST_CHECK( flag );
        
        flag = false;
        ar.add_all( boost::bind(cb2, _1, boost::ref(flag)) );
        BOOST_CHECK(flag);
    }
}

BOOST_AUTO_TEST_SUITE_END()
