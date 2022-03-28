/**
 * 测试 连接池
 */

#include <iostream>
#include <thread>
#include "../redisConnectPool.hpp"

int main(){
    //创建一个池
    redisConnectPool red_pool("127.0.0.1",6379,4);

    auto res1 = red_pool.command("set Hello %s","hello");
    auto res2 = red_pool.command("get %s","Hello");
    if(res2.is_string()) {
        std::cout << "yes result is string" << std::endl;
        std::cout << res2.str() << std::endl;
    }
    else {
        std::cout << "NO, result is not string" << std::endl;
    }

    //创建线程
    std::vector<std::thread> vec;
    for(int i = 1 ;i<=10 ;i++){
        vec.emplace_back([i,&red_pool](){
            {
                auto res = red_pool.command("set ID %d",i);
                if(res.is_string()) {
                    std::cout << "yes result is string" << std::endl;
                    std::cout << res.str() << std::endl;
                }
                else if(res.is_integer() ){
                    std::cout << "result is int : " << res.integer() << std::endl;
                }
            }
            std::cout << "thread end" << std::endl;

        });
    }

    for (auto& e : vec) {
        if( e.joinable())
            e.join();
    }

    return 0;
}
