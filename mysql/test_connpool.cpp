
#include <vector>
#include "src/connpool.hpp"





double useConnpool(int connCnt) {
    ConnectionPool *cp = ConnectionPool::getConnectionPool();
    clock_t start = clock();
    for (int i = 0; i < connCnt; ++i) {
        std::shared_ptr<Connection> pconn = cp->getConnection();
        char sql[1024] = {0};
        sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");
        pconn->update(sql);
    }
    clock_t end = clock();

    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    double milliseconds = duration * 1000;
    std::cout << "use connpool: " << milliseconds << "ms" << std::endl;

    return milliseconds;
}


double useConnpoolMultiThread(int connCnt, int numThread) {

    int connCntPerThread = connCnt / numThread;
    std::vector<std::thread> threads;
    clock_t start = clock();

    for (int i = 0; i < numThread; ++i) {
        threads.emplace_back([&]() {
            ConnectionPool *cp = ConnectionPool::getConnectionPool();
            for (int i = 0; i < connCnt; ++i) {
                std::shared_ptr<Connection> pconn = cp->getConnection();
                char sql[1024] = {0};
                sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");
                pconn->update(sql);
            }
        });
    }

    for (int i = 0; i < numThread; ++i) {
        threads[i].join();
    }

    clock_t end = clock();
    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    double milliseconds = duration * 1000;
    std::cout << "use connpool: " << milliseconds << "ms" << std::endl;

    return milliseconds;
}



double notUseConnpool(int connCnt) {
    clock_t start = clock();
    for (int i = 0; i < connCnt; ++i) {
        Connection conn;
        char sql[1024] = {0};
        sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");
        conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
        conn.update(sql);
    }
    clock_t end = clock();

    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    double milliseconds = duration * 1000;
    std::cout << "not use connpool: " << milliseconds << "ms" << std::endl;

    return milliseconds;
}


double notUseConnpoolMultiThread(int connCnt, int numThread) {

    int connCntPerThread = connCnt / numThread;
    std::vector<std::thread> threads;
    clock_t start = clock();

    for (int i = 0; i < numThread; ++i) {
        threads.emplace_back([&]() {
            for (int i = 0; i < connCntPerThread; ++i) {
                Connection conn;
                char sql[1024] = {0};
                sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");
                conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
                conn.update(sql);
            }
        });
    }

    for (int i = 0; i < numThread; ++i) {
        threads[i].join();
    }

    clock_t end = clock();
    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    double milliseconds = duration * 1000;
    std::cout << "not use connpool: " << milliseconds << "ms" << std::endl;
    return milliseconds;
}



int main(int argc, char *argv[]) {

#if 0    // 单线程+不使用连接池

    double avg = 0;
    for (int i = 0; i < 2; ++i) {
        avg += notUseConnpool(10000);
    }
    avg /= 2;
    std::cout << "not use connpool avg: " << avg << "ms" << std::endl;

#elif 0  // 单线程+使用连接池

    double avg = 0;
    for (int i = 0; i < 2; ++i) {
        avg += useConnpool(10000);
    }
    avg /= 2;
    std::cout << "use connpool avg: " << avg << "ms" << std::endl;

#elif 1  // 3线程+不使用连接池

    double avg = 0;
    for (int i = 0; i < 2; ++i) {
        avg += notUseConnpoolMultiThread(10000, 4);
    }
    avg /= 2;
    std::cout << "n thread not use connpool avg: " << avg << "ms" << std::endl;

#elif 0  // 3线程+使用连接池

    double avg = 0;
    for (int i = 0; i < 2; ++i) {
        avg += useConnpoolMultiThread(10000, 4);
    }
    avg /= 2;
    std::cout << "n thread use connpool avg: " << avg << "ms" << std::endl;

#endif


    return 0;
}




















