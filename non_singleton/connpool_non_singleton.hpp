
#ifndef __CONNPOOL_H__
#define __CONNPOOL_H__

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
#include <chrono>
#include <ctime>
#include <mysql/mysql.h>


#define LOG(str) \
    std::cout << __FILE__ << ":" << __LINE__ << " " \
              << "[" << __TIMESTAMP__ << "]" << ": " << str << std::endl


class Connection {
public:
    Connection() { _conn = mysql_init(nullptr); }
    ~Connection() {
        if (_conn != nullptr) {
            mysql_close(_conn);
        }
    }

    bool connect(std::string ip, unsigned short port, std::string user, std::string password, std::string dbname) {
        MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
        return p != nullptr;
    }

    // insert/delete/update
    bool update(std::string sql) {
        if (mysql_query(_conn, sql.c_str())) {
            LOG("update failed: " + sql);
            return false;
        }
        return true;
    }

    // select
    MYSQL_RES* query(std::string sql) {

        // If the query is successfully executed, the return value is 0.
        // If the query fails, the return value is non zero, indicating an error code. 
        // You can use mysql_error() that is used to obtain detailed error information.

        if (mysql_query(_conn, sql.c_str())) {
            LOG("select failed: " + sql);
            return nullptr;
        }
        return mysql_use_result(_conn);
    }

    // Refresh the starting idle time point of the connection
	void refreshAliveTime() { _alivetime = clock(); }

    // Return the idle time of the connection
	clock_t getAliveTime() const { return clock() - _alivetime; }
private:
    MYSQL *_conn;
    clock_t _alivetime;   // Record the starting time when the connection enters an idle state.
};



// non-singleton
class ConnectionPool {
public:
    ConnectionPool(std::string ip, unsigned short port, std::string username, std::string password, 
                   std::string dbname, std::string connpoolname = "mysql", 
                   int initSize = 4, int maxSize = 1024, int maxIdleTime = 60, int connTimeout = 100) 
                   : _ip(ip), _port(port), _username(username), _password(password), 
                     _dbname(dbname), _connpoolname(connpoolname), 
                     _initSize(initSize), _maxSize(maxSize), _maxIdleTime(maxIdleTime), _connTimeout(connTimeout) {

        for (int i = 0; i < _initSize; ++i) {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime();
            _connQue.push(p);
            _connCnt++;
        }
        
        std::thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
        produce.detach();

        // start a new timer thread, scan idle connections that have exceeded maxIdleTime, and collect timeout connections.
        std::thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
        scanner.detach();
    }

    std::shared_ptr<Connection> getConnection() {
        std::unique_lock<std::mutex> lock(_queMutex);
        if (_connQue.empty()) {
            if (std::cv_status::timeout == _condVar.wait_for(lock, std::chrono::milliseconds(_connTimeout))) {
                if (_connQue.empty()) {
                    LOG("Obtaining idle connection timed out!");
                    return nullptr;
                }
            }
        }

        // When the shared_ptr smart pointer is destructed, the connection resource will be directly deleted.
        // Equivalent to calling the destructor of a connection, the connection is closed.
        // You need to customize the method of releasing shared_ptr resources by directly returning the connection to the queue.

        std::shared_ptr<Connection> sp(_connQue.front(), 
            [&](Connection *pcon) { 
                std::unique_lock<std::mutex> lock(_queMutex);
                pcon->refreshAliveTime();
                _connQue.push(pcon); 
            });

        _connQue.pop();
        if (_connQue.empty()) {

            // Who consumed the last connection in the queue and is responsible for notifying the producer of production.
            _condVar.notify_all(); 
        }

        return sp;
    }

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

private:

    void produceConnectionTask() {
        for ( ; ; ) {
            std::unique_lock<std::mutex> lock(_queMutex);
            while (!_connQue.empty()) {
                _condVar.wait(lock);  // The queue is not empty, where the production thread enters a waiting state.
            }
            if (_connCnt < _maxSize) {
                Connection *p = new Connection();
                p->connect(_ip, _port, _username, _password, _dbname);
                p->refreshAliveTime();
                _connQue.push(p);
                _connCnt++;
            }
            _condVar.notify_all();
        }
    }

    void scannerConnectionTask() {
        for (;;) {

            // Simulate timing effects through sleep
            std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

            // Scan the entire queue to release excess connections
            std::unique_lock<std::mutex> lock(_queMutex);
            while (_connCnt > _initSize) {
                Connection *p = _connQue.front();
                if (p->getAliveTime() >= (_maxIdleTime * 1000)) {
                    _connQue.pop();
                    _connCnt--;
                    delete p; // call ~Connection()
                } else {
                    break; // The connection of the team leader did not exceed _MaxIdleTime, other connections definitely don't have it.
                }
            }
        }
    }

private:
    std::string    _ip;
    unsigned short _port;
    std::string    _username;
    std::string    _password;
    std::string    _dbname;
    std::string    _connpoolname;   // When creating multiple connection pools, uniquely identify one connection pool

    int _initSize;
    int _maxSize;
    int _maxIdleTime;   // s
    int _connTimeout;   // ms

    std::queue<Connection*> _connQue;
    std::mutex              _queMutex;
    std::atomic_int         _connCnt;
    std::condition_variable _condVar;
};


#endif











































