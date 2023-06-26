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

    // 刷新一下连接的起始空闲时间点
	void refreshAliveTime() { _alivetime = clock(); }
    // 返回连接的空闲时间
	clock_t getAliveTime() const { return clock() - _alivetime; }
private:
    MYSQL *_conn;
    clock_t _alivetime;   // 记录连接进入空闲状态的起始时间
};


// 单例模式
class ConnectionPool {
public:
    // 线程安全的懒汉单例函数接口
    static ConnectionPool* getConnectionPool() {
        static ConnectionPool pool;
        return &pool;
    }

    std::shared_ptr<Connection> getConnection() {
        std::unique_lock<std::mutex> lock(_queueMutex);
        if (_connectionQue.empty()) {
            if (std::cv_status::timeout == cv.wait_for(lock, 
                std::chrono::milliseconds(_connectionTimeout))) {
                if (_connectionQue.empty()) {
                    LOG("Obtaining idle connection timed out!");
                    return nullptr;
                }
            }
        }

        // shared_ptr智能指针析构时，会把connection资源直接delete掉
        // 相当于调用connection的析构函数，connection就被close掉了
        // 这里需要自定义shared_ptr的资源释放的方式，把connection直接归还到queue当中
        std::shared_ptr<Connection> sp(_connectionQue.front(), 
            [&](Connection *pcon) { 
                std::unique_lock<std::mutex> lock(_queueMutex);
                pcon->refreshAliveTime();
                _connectionQue.push(pcon); 
            });

        _connectionQue.pop();
        if (_connectionQue.empty()) {
            cv.notify_all(); // 谁消费了队列中的最后一个conncection，谁负责通知一下生产者生产
        }

        return sp;
    }

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

private:

    bool loadConfigFile() {
        FILE *pf = fopen("connpool.conf", "r");
        if (pf == nullptr) {
            LOG("connpool.conf file is not exist!");
            return false;
        }
        while (!feof(pf)) {
            char line[1024] = {0};
            fgets(line, 1024, pf);  
            std::string str = line;
            int idx = str.find('=', 0);   // 在字符串str中从索引0开始查找字符'='
            if (idx == -1) continue;

            // password=123456\n
            int endidx = str.find('\n', idx);
            std::string key = str.substr(0, idx);
            std::string value = str.substr(idx + 1, endidx - idx - 1);
            
            if (key == "ip") {
                _ip = value;
            } else if (key == "port") {
                _port = atoi(value.c_str());
            } else if (key == "username") {
                _username = value;
            } else if (key == "password") {
                _password = value;
            } else if (key == "dbname") {
                _dbname = value;
            } else if (key == "initSize") {
                _initSize = atoi(value.c_str());
            } else if (key == "maxSize") {
                _maxSize = atoi(value.c_str());
            } else if (key == "maxIdleTime") {
                _maxIdleTime = atoi(value.c_str());
            } else if (key == "connectionTimeOut") {
                _connectionTimeout = atoi(value.c_str());
            }
        } // while
        return true;
    }

    void produceConnectionTask() {
        for ( ; ; ) {
            std::unique_lock<std::mutex> lock(_queueMutex);
            while (!_connectionQue.empty()) {
                cv.wait(lock);  // 队列不空，此处生产线程进入等待状态
            }
            if (_connectionCnt < _maxSize) {
                Connection *p = new Connection();
                p->connect(_ip, _port, _username, _password, _dbname);
                p->refreshAliveTime();
                _connectionQue.push(p);
                _connectionCnt++;
            }
            cv.notify_all();
        }
    }

    void scannerConnectionTask() {
        for (;;) {
            // 通过sleep模拟定时效果
            std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

            // 扫描整个队列，释放多余的连接
            std::unique_lock<std::mutex> lock(_queueMutex);
            while (_connectionCnt > _initSize) {
                Connection *p = _connectionQue.front();
                if (p->getAliveTime() >= (_maxIdleTime * 1000)) {
                    _connectionQue.pop();
                    _connectionCnt--;
                    delete p; // 调用~Connection()释放连接
                } else {
                    break; // 队头的连接没有超过_maxIdleTime，其它连接肯定没有
                }
            }
        }
    }

    ConnectionPool() {
        if (!loadConfigFile()) return;
        for (int i = 0; i < _initSize; ++i) {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime();
            _connectionQue.push(p);
            _connectionCnt++;
        }
        
        std::thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
        produce.detach();

        // 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行超时连接回收
        std::thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
        scanner.detach();
    }

private:
    std::string _ip;
    unsigned short _port;
    std::string _username;
    std::string _password;
    std::string _dbname;

    int _initSize;
    int _maxSize;
    int _maxIdleTime;
    int _connectionTimeout;

    std::queue<Connection*> _connectionQue;
    std::mutex _queueMutex;
    std::atomic_int _connectionCnt;
    std::condition_variable cv;
};


#endif