/**
 * 连接池
 */

#pragma once

#include <vector>
#include <mutex>
#include <string_view>
#include <memory>
#include <vector>
#include <thread>

#include "redisConnection.hpp"

class redisConnectPool {

public:
    explicit redisConnectPool(std::string_view ip,int port,int connNum);
    ~redisConnectPool();

    redisContext *get_conn(int &id) ;
    void put_conn(int id) ;

private:
    //保活
    void keepAlive();
    
    int               current_conn;
    std::mutex        mtx;
    std::thread       keepAlive_thread{nullptr}; //保活的线程
    std::atomic<bool> isClosed{false}; //是否关闭
    std::vector< std::shared_ptr<redisConnection> > connectPool;
};




void redisConnectPool::keepAlive(){
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& conn : connectPool) {
        if( conn->ping() == false ){
            if( conn->redisConnect() == false){
                //DEBUG 重连接失败
                std::cout << "Redis 重连接 失败" << std::endl;
            }
        }
    }
}
