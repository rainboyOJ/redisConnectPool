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
#include <atomic>

#include "redisResult.hpp"
#include "redisConnection.hpp"

class redisConnectPool {

public:
    explicit redisConnectPool(std::string_view ip,int port,int connNum);
    ~redisConnectPool();

    std::weak_ptr<redisConnection> get_conn(int &id) ;
    void put_conn(int id) ;

    template<typename... Args>
    auto command(Args... args){
        int id = -1;
        redisResult res;
        auto w_ptr = get_conn(id);
        if( auto s_ptr = w_ptr.lock() ){
            res.Init( 
                    static_cast<redisReply*>(
                        redisCommand(s_ptr->GetCtx(), std::forward<Args>(args)...)
                    )
                    );


        }
        if( id != -1) put_conn(id);
        return res;
    }

private:
    //保活
    void keepAlive();
    
    int               current_conn{0};
    std::mutex        mtx;
    //std::thread       keepAlive_thread{nullptr}; //保活的线程
    std::atomic<bool> isClosed{false}; //是否关闭
    std::vector< std::shared_ptr<redisConnection> > connectPool;
    std::vector<bool> is_conn_used;
};

redisConnectPool::redisConnectPool(std::string_view ip,int port=6379,int connNum=4){
    for(int i=1;i<=connNum;++i){
        connectPool.push_back( std::make_shared<redisConnection>(ip,port) );
        if( connectPool.back()->redisConnect() == false){
            std::cerr << __FILE__ << " " << "connect redisServer failed!" << std::endl;
            break;
        }
        is_conn_used.push_back(false);
    }
    isClosed.store(true);
}

redisConnectPool::~redisConnectPool(){
    isClosed.store(false);
    std::lock_guard<std::mutex> lock(mtx);
    for(int i = 0 ;i< connectPool.size(); ++i){
        is_conn_used[i] = false;
    }
}



std::weak_ptr<redisConnection> redisConnectPool::get_conn(int &id) {
    std::lock_guard<std::mutex> lock(mtx);
    if( connectPool.size() != 0) {

        int i = (current_conn+1 ) % connectPool.size();
        for( ; i < connectPool.size() ; i = ( i +1 ) % connectPool.size() ){
            if( is_conn_used[i] == false ){
                is_conn_used[i] = true;
                current_conn = i;
                id = i;
                return connectPool[i];
            }
        }

    }
    return std::weak_ptr<redisConnection>(); // Maby Bug
}

void redisConnectPool::put_conn(int id) {
    std::lock_guard<std::mutex> lock(mtx);
    is_conn_used[id] = false;
}
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
