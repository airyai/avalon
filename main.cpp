#include <iostream>
#include <boost/asio/io_service.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

void f(boost::shared_ptr<boost::asio::io_service::work> w) {
    boost::this_thread::sleep(boost::posix_time::seconds(2));
    w.reset();
}

int main(int argc, char **argv) {
    std::cout << "Hello, world!" << std::endl;
    
    boost::asio::io_service service;
    boost::shared_ptr<boost::asio::io_service::work> work;
    work.reset(new boost::asio::io_service::work(service));
    service.post(boost::bind(f, work));
    service.run_one();
    
    return 0;
}
