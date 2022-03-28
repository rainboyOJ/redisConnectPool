/**
 * test redisConnection
 */
#include <iostream>
#include "../redisConnection.hpp"
#include "../redisResult.hpp"

int main(){
    redisConnection conn("127.0.0.1",6379);
    bool ret = conn.redisConnect();
    std::cout << std::boolalpha << "connect: " << ret << std::endl;
    ret = conn.ping();
    std::cout << std::boolalpha << "ping: " << ret << std::endl;


    redisCommand(conn.GetCtx(),"SET test hello");

    redisResult res;

    auto command_ret = redisCommand(conn.GetCtx(), "GET test");

    res.Init(static_cast<redisReply*>(command_ret));
    std::cout << std::boolalpha << "Get result is Integer : " << res.is_integer()  << std::endl;
    std::cout << std::boolalpha << "Get result is string: " << res.is_string()  << std::endl;
    if( res.is_integer() ){
        std::cout << "ret value : " << res.integer() << std::endl;
    }
    if( res.is_string() ){
        std::cout << "ret value : " << res.str() << std::endl;
    }



    return 0;
}
