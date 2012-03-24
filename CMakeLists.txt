cmake_minimum_required(VERSION 2.8)

# project start
project(avalon)

# check libraries
find_library(BOOST_SYSTEM_LIB boost_system)
find_library(BOOST_THREAD_LIB boost_thread)
find_library(BOOST_UNIT_TEST_LIB boost_unit_test_framework)
find_library(PTHREAD_LIB pthread)
find_library(PROTOBUF_LIB protobuf)
find_library(SSL_LIB ssl)

# gather libraries
SET(COMMON_LIB ${BOOST_SYSTEM_LIB} ${BOOST_THREAD_LIB} 
                ${PTHREAD_LIB} ${PROTOBUF_LIB} ${SSL_LIB})
SET(TEST_LIB ${COMMON_LIB} ${BOOST_UNIT_TEST_LIB})

# gather source files
SET(COMMON_SRC errors.cpp)
SET(THREAD_SRC thread/workpool.cpp thread/asyncresult.cpp)
SET(SERVER_SRC servers/channelbase.cpp)

SET(TEST_SRC test/test_pre_condition.cpp test/test_workpool.cpp)
SET(MAIN_SRC ${COMMON_SRC} ${THREAD_SRC} ${SERVER_SRC})

# compile the avalon library
add_library(avalon ${MAIN_SRC})
add_library(avalon_test ${TEST_SRC})

# compile main executable program
add_executable(main main.cpp)
target_link_libraries(main avalon ${COMMON_LIB})

# compile test programs
add_executable(test test/test_main.cpp)
target_link_libraries(test avalon avalon_test ${TEST_LIB})
