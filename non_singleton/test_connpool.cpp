
#include <vector>
#include <string.h>
#include "connpool_non_singleton.hpp"


std::string ip = "127.0.0.1";
unsigned short port = 3306;
std::string username = "root";
std::string password = "123456";
std::string dbname = "chat";



double UseConnpoolMultiThread(int numThread, int connCnt) {

    int connCntPerThread = connCnt / numThread;
    std::vector<std::shared_ptr<std::thread>> threads;
    std::vector<double> timePerThreadExec(numThread, 0);

	static std::shared_ptr<ConnectionPool> connpool(new ConnectionPool(ip, port, username, password, dbname));

    {
        std::shared_ptr<Connection> conn = connpool->getConnection();
        char sql[1024] = {0};
        sprintf(sql, "delete from stu;");
        conn->update(sql);
    }
  
    auto threadFunc = [&](int cntConn) {

        for (int i = 0; i < cntConn; ++i) {
            char sql[1024] = {0};
            sprintf(sql, "insert into stu(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");
			std::shared_ptr<Connection> conn = connpool->getConnection();
            conn->update(sql);
        }
    };


	// After testing, the time for creating and destroying threads is negligible.
	clock_t start = clock();

    for (int i = 0; i < numThread; ++i) {
		std::thread* th = new std::thread(threadFunc, connCntPerThread);
        threads.emplace_back(th);
    }

    for (int i = 0; i < numThread; ++i) {
        if (threads[i]->joinable()) {
            threads[i]->join();
        }
    }

	clock_t end = clock();
	double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    return duration * 1000;
}





double notUseConnpoolMultiThread(int numThread, int connCnt) {

    int connCntPerThread = connCnt / numThread;
    std::vector<std::shared_ptr<std::thread>> threads;
    std::vector<double> timePerThreadExec(numThread, 0);

    {
        Connection conn;
        char sql[1024] = {0};
        sprintf(sql, "delete from stu;");
        conn.connect(ip, port, username, password, dbname);
        conn.update(sql);
    }
  
    auto threadFunc = [](int cntConn) {
        for (int i = 0; i < cntConn; ++i) {
            Connection conn;
            char sql[1024] = {0};
            sprintf(sql, "insert into stu(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");
            conn.connect(ip, port, username, password, dbname);
            conn.update(sql);
        }
    };

	// After testing, the time for creating and destroying threads is negligible.
	clock_t start = clock();

    for (int i = 0; i < numThread; ++i) {
		std::thread* th = new std::thread(threadFunc, connCntPerThread);
        threads.emplace_back(th);
    }

    for (int i = 0; i < numThread; ++i) {
        if (threads[i]->joinable()) {
            threads[i]->join();
        }
    }

	clock_t end = clock();
	double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    return duration * 1000;
}


// g++ -o test_connpool connpool_non_singleton.hpp test_connpool.cpp -lmysqlclient

int main() {

	// Each test should first create a table for the data to be inserted.
    {
        Connection conn;
        conn.connect(ip, port, username, password, dbname);

        char sql[1024] = {0};
        sprintf(sql, "drop table chat.stu;");
        conn.update(sql);

        memset(sql, 0, 1024);
        sprintf(sql, "create table chat.stu(name VARCHAR(255), age INT, sex VARCHAR(10));");
        conn.update(sql);
    }


    int maxNumThread = 4;
    int maxNumConn  = 8000;
    int stepLenConn = 2000;  
    int stepNumConn = maxNumConn / stepLenConn;


#if 0

    std::cout << "\n\n --- not use connpool --- \n" << std::endl; 
    for (int i = 1; i <= maxNumThread; i++) {
        for (int j = 1; j <= stepNumConn; j++) {
            double result = notUseConnpoolMultiThread(i, j * stepLenConn);
            printf("threadNum:%2d, connNum:%5d, spend time:%6.0fms\n", i, j * stepLenConn, result);
        }
    }

#endif

#if 1

    std::cout << "\n\n --- use connpool --- \n" << std::endl; 
    for (int i = 1; i <= maxNumThread; i++) {
        for (int j = 1; j <= stepNumConn; j++) {
            double result = UseConnpoolMultiThread(i, j * stepLenConn);
			printf("threadNum:%2d, connNum:%5d, spend time:%6.0fms\n", i, j * stepLenConn, result);
        }
    }

#endif

    printf("\n\n\n");
    return 0;
}


















