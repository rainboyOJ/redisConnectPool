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
#include <sstream>

#include "redisResult.hpp"
#include "redisConnection.hpp"

class redisConnectPool {

public:
    explicit redisConnectPool(std::string_view ip,int port,int connNum);
    ~redisConnectPool();

    std::weak_ptr<redisConnection> get_conn(int &id) ;
    void put_conn(int id) ;

    template<typename... Args>
    redisResult command(Args... args){
        int id = -1;
        redisResult res;
        while ( 1 ) {
            auto w_ptr = get_conn(id);
            if( auto s_ptr = w_ptr.lock() ){
                res.Init( 
                        static_cast<redisReply*>(
                            redisCommand(s_ptr->GetCtx(), std::forward<Args>(args)...)
                            )
                        );
                if( id != -1) put_conn(id);
                return  res;
            }
        }
        //if( id != -1) put_conn(id);
        return res;
    }

    // ================= API =================
    /**
     * @desc 是否存在某个值
     */
    bool isExitsKey(std::string_view key);

    void EXPIRE(std::string_view key,std::size_t expire= 60);

    /**
     * @desc 设定key 与 过期时间
     */
    template<typename T>
    void SETEX(std::string_view key,T&& value,std::size_t expire= 60);

    /**
     * @desc 追加字符串
     */
    void APPEND(std::string_view key,std::string_view value,char split=0,std::size_t expire= 60);

    /**
     * @得到一个指定类型的key值
     */
    template<typename T>
    T GET(std::string_view key);

    /**
     * @删除
     */

    void DEL(const char * key);
    
    /**
     * @desc 创建一个列表并添加元素
     */
    template<typename T>
    void RPUSH(std::string_view key,T&& value);


    /**
     * @desc 获取list里的元素
     */
    template<typename T>
    std::vector<T> LRANGE(std::string_view key,int start,int end);

    template<typename T>
    std::vector<T> ALL_LRANGE(std::string_view key){ return LRANGE<T>(key, 0, -1); }

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
#ifdef  REDIS_POOL_DEBUG
    std::cout << "get_conn begin" << std::endl;
#endif
    if( connectPool.size() != 0) {

        int try_get_cnt = 0;
        int i = (current_conn+1 ) % connectPool.size();
        for( ; i < connectPool.size() ; i = ( i +1 ) % connectPool.size() ){
            if(++try_get_cnt > connectPool.size() )  // 如果没有空闲的资源
                break;
            if( is_conn_used[i] == false ){
                is_conn_used[i] = true;
                current_conn = i;
                id = i;
#ifdef  REDIS_POOL_DEBUG
    std::cout << "get_conn end : id = "  << id << std::endl;
#endif
                return connectPool[i];
            }
        }
    }
    //一个空的weak_ptr
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

template<typename T>
void redisConnectPool::SETEX(std::string_view key,T&& value,std::size_t expire){
    std::ostringstream oss;
    oss << "SETEX " << key << " " << expire <<  " " << value ;
    redisResult result = command(oss.str().c_str());
}

template<typename T>
T redisConnectPool::GET(std::string_view key){
    std::ostringstream oss;
    oss << "GET " << key ;
    redisResult result = command(oss.str().c_str());
    if constexpr (std::is_same_v<T, int>){
        if (result.is_integer())
            return  result.integer();
        else if ( result.is_string() )
            return std::stoi(result.str());
        return -1; //unkown
    }

    if constexpr (std::is_same_v<T, std::string>){
        if ( result.is_string() )
            return result.str();
        //else if ( result.is_nil())
            //std::cout << "null" << std::endl ;
        return ""; // null
    }
    throw "unsport type";
}

void redisConnectPool::DEL(const char * key){
    command("DEL %s",key);
}

template<typename T>
void redisConnectPool::RPUSH(std::string_view key,T&& value){
    std::ostringstream oss;
    oss << "RPUSH " << key << " " << value;
    command(oss.str().c_str());
}

template<typename T>
std::vector<T>
redisConnectPool::LRANGE(std::string_view key,int start,int end) {
    std::ostringstream oss;
    oss << "LRANGE " << key << " ";
    oss << start << " "<< end;

    redisResult result = command(oss.str().c_str());

    if ( result.is_nil() )
        return {};

    if constexpr ( std::is_same_v<T, std::string>)
    {
        std::vector<T> ret;
        for(int i =0 ;i< result.elements() ; ++i){
            auto Reply = result.element(i);
            ret.push_back(Reply->str);
        }
        return  ret;
    }

    throw "unsport type";
}

bool redisConnectPool::isExitsKey(std::string_view key){
    std::ostringstream oss;
    oss<< "EXISTS " << key ;
    redisResult result = command(oss.str().c_str());
    return !result.is_nil() && (result.is_integer() && result.integer() == 1) ;
}

void redisConnectPool::APPEND(std::string_view key,std::string_view value,char split,std::size_t expire){
    //先设置 过期时间
    if( expire > 0) {
        std::ostringstream oss;
        oss << "EXPIRE " << key << " " << expire;
        command(oss.str().c_str());
    }

    std::ostringstream oss;
    oss << "APPEND " << key <<" ";
    if( split != 0) oss << split;
    oss << value;
    command(oss.str().c_str());
}

void redisConnectPool::EXPIRE(std::string_view key,std::size_t expire) {
    std::ostringstream oss;
    oss << "EXPIRE " << key << " " << expire;
    command(oss.str().c_str());
}
