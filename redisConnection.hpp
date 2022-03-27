/**
 * 对 redis connection 的封装
 */

#pragma once

#include <strings.h>
#include <hiredis/hiredis.h>

class redisConnection {
public:
    explicit redisConnection(std::string_view ip,uint32_t port);
    ~redisConnection();
    bool redisConnect();
    bool redisReConnect();
    bool ping();

    redisContext* GetCtx()        const { return mCtx;        }
    bool          GetConnstatus() const { return mConnStatus; }
private:
    redisContext* mCtx{NULL};
    std::string   mHost; // redis host
    uint32_t      mPort; // redis sever port
    //std::string   mPass; // redis server password
    uint32_t      mTimeout; // connect timeout second
    uint32_t      mPoolsize; // connect pool size for each redis DB
    bool          mConnStatus; // redis connection status
};


redisConnection::redisConnection(std::string_view ip,uint32_t port)
    :mHost{ip},mPort{port}
{}

bool redisConnection::redisConnect(){
    bool bRet = false;
    if (NULL != mCtx) {
        redisFree(mCtx);
        mCtx = NULL;
    }
    mCtx = ::redisConnect(mHost.c_str(), mPort);

    if (NULL == mCtx) {
        bRet = false;
    }
    mConnStatus = bRet;
    return bRet;
}

bool redisConnection::redisReConnect(){

    if (NULL == mCtx) { //没有连接上
        return false;
    }

    bool bRet = false;
    redisContext* tmp_ctx = ::redisConnect(mHost.c_str(),mPort);
    if (NULL == tmp_ctx) {
        //xredis_warn("RedisReConnect failed dbindex:%u host:%s port:%u passwd:%s poolsize:%u timeout:%u role:%u",
            //mSliceIndex, mHost.c_str(), mPort, mPass.c_str(), mPoolsize, mTimeout, mRole);
        bRet = false;
    } else {
        redisFree(mCtx);
        mCtx = tmp_ctx;
        bRet = true;
    }

    mConnStatus = bRet;
    return bRet;
}

bool redisConnection::ping()
{
    redisReply * reply = static_cast<redisReply*>(redisCommand(mCtx, "PING") );
    bool bRet = (NULL != reply) && (reply->str) && (strcasecmp(reply->str, "PONG") == 0);
    if (bRet) {
        freeReplyObject(reply);
    }
    return bRet;
}
