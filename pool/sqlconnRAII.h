/*
 * @Author: zzzzztw
 * @Date: 2023-02-23 15:13:00
 * @LastEditors: Do not edit
 * @LastEditTime: 2023-02-24 13:30:55
 * @FilePath: /Webserver/pool/sqlconnRAII.h
 */
#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H
#include "sqlconnpool.h"


class SqlconnRAII{

public:
    SqlconnRAII(MYSQL **sql, SqlConnPool * connpool){
        assert(connpool);
        *sql =  connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }
    
    ~SqlconnRAII(){
        if(sql_){
            connpool_->FreeConn(sql_);
        }
    }

private:
    MYSQL* sql_;
    SqlConnPool* connpool_;


};

#endif