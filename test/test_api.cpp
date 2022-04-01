/**
 * test redis api
 */
#include <iostream>
#include <thread>
#include <chrono>
#include "../redisConnectPool.hpp"

int main(){
    //创建一个池
    redisConnectPool red_pool("127.0.0.1",6379,4);


    red_pool.SETEX("hello", "world", 2);

    auto str_ret = 
        red_pool.GET<std::string>("hello");

    std::cout << "hello" << std::endl;

    auto str_ret2 = 
        red_pool.GET<std::string>("notExist");
    std::cout << "notExist key : " << str_ret2 << std::endl;

    auto exist1 = red_pool.isExitsKey("hello");
    std::cout << std::boolalpha << "key hello before 2 seconds,isExist : " << exist1  << std::endl;

    for(int i=1;i<=2;++i){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "sleep " << i << " second" << std::endl;
    }

    auto hello_exits = red_pool.isExitsKey("hello");
    std::cout << std::boolalpha << "key hello after 2 seconds,isExist : " << hello_exits  << std::endl;


    
    red_pool.APPEND("test_append", "1");
    red_pool.APPEND("test_append", "2",',');
    red_pool.APPEND("test_append", "3",',');
    red_pool.APPEND("test_append", "4",',');

    auto ret2 = red_pool.GET<std::string>("test_append");
    std::cout << "test_append "<< ret2 << std::endl;

    // ===================== test RANGE
    red_pool.RPUSH("range", "{v1}");
    red_pool.RPUSH("range", "{v2}");
    red_pool.RPUSH("range", "{v3}");
    red_pool.EXPIRE("range",3);

    auto ret3 = red_pool.ALL_LRANGE<std::string>("range");
    std::cout << "range : " << std::endl;
    for (const auto& e : ret3) {
        std::cout << e << std::endl;
    }




    return 0;
}
