/**
 * 对redis的结果Replay进行封装
 */
#pragma once

#include <hiredis/hiredis.h>

//#define REDIS_REPLY_STRING 1 //字符串
//#define REDIS_REPLY_ARRAY 2    //数组，多个reply，通过element数组以及elements数组大小访问
//#define REDIS_REPLY_INTEGER 3    //整型, integer字段
//#define REDIS_REPLY_NIL 4    //空，没有数据
//#define REDIS_REPLY_STATUS 5    //状态，str字符串以及len
//#define REDIS_REPLY_ERROR 6    //错误，同STATUS

class redisResult {
public:
    redisResult() {}
    ~redisResult() {
        if(NULL != Reply)
            freeReplyObject(Reply);
    }

    void Init(redisReply * r) {
        if(NULL != Reply)
            freeReplyObject(Reply);
        Reply = r;
    }
    
    inline int32_t   type()    { return Reply->type;} //结果的类型
    inline long long integer() { return Reply->integer;}
    inline int32_t   len()     { return  Reply->len;}
    inline const char * str()  { return  Reply->str;}
    inline size_t elements()   { return Reply->elements;}
    inline redisReply * element(uint32_t index) { return Reply->element[index];}

    bool is_string()  { return NULL!=Reply && type() == REDIS_REPLY_STRING;  }
    bool is_array()   { return NULL!=Reply && type() == REDIS_REPLY_ARRAY;   }
    bool is_integer() { return NULL!=Reply && type() == REDIS_REPLY_INTEGER; }
    bool is_nil()     { return NULL!=Reply && type() == REDIS_REPLY_NIL;     }
    bool is_error()   { return NULL!=Reply && type() == REDIS_REPLY_ERROR;   }

private:

    redisReply* Reply{NULL};
};
