#ifndef __CONNPOOL_H__
#define __CONNPOOL_H__

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <mysql/mysql.h>

#define LOG(str) \
    std::cout << __FILE__ << ":" << __LINE__ << " " \
              << "[" << __TIMESTAMP__ << "]" << ": " << str << std::endl; 


class Connection {
public:
    Connection() {
        _conn = mysql_init(nullptr);
    }
    ~Connection() {
        if (_conn != nullptr) {
            mysql_close(_conn);
        }
    }
    bool connect(std::string ip, unsigned short port, 
                 std::string user, std::string password, std::string dbname) {
        MYSQL *p = mysql_real_connect(_conn, ip.c_str(), 
            user.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
        return p != nullptr;
    }
    // insert/delete/update
    bool update(std::string sql) {
        if (mysql_query(_conn, sql.c_str())) {
            LOG("update failed:" + sql);
            return false;
        }
        return true;
    }
    // select
    MYSQL_RES* query(std::string sql) {
        if (mysql_query(_conn, sql.c_str())) {
            LOG("select failed:" + sql);
            return nullptr;
        }
        return mysql_use_result(_conn);
    }
private:
    MYSQL *_conn;
};


// 单例模式
class ConnectionPool {
public:
    // 线程安全的懒汉单例函数接口
    static ConnectionPool* getConnectionPool() {
        static ConnectionPool pool;
        return &pool;
    }
private:
    ConnectionPool() {}
    bool loadConfigFile() {
        FILE *pf = fopen("connpool.conf", "r");
        if (pf == nullptr) {
            LOG("connpool.conf file is not exist!");
            return false;
        }



        
        return true;
    }

private:
    std::string _ip;
    unsigned short _port;
    std::string _username;
    std::string _password;

    int _initSize;
    int _maxSize;
    int _maxIdleTime;
    int _connectionTimeout;

    std::queue<Connection*> _connectionQue;
    std::mutex _queueMutex;
};


















#endif