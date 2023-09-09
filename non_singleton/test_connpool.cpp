
#include <vector>
#include <string.h>
#include "connpool_non_singleton.hpp"




double UseConnpoolMultiThread(int numThread, int connCnt) {

    int connCntPerThread = connCnt / numThread;
    std::vector<std::thread> threads;
    std::vector<double> timePerThreadExec(numThread, 0);

    for (int i = 0; i < numThread; ++i) {
        threads.emplace_back([&](int kth) {
            
            clock_t start = clock();

            for (int i = 0; i < connCntPerThread; ++i) {
                Connection conn;
                char sql[1024] = {0};
                sprintf(sql, "insert into stu(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");
                conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
                conn.update(sql);
            }

            clock_t end = clock();
            double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
            double milliseconds = duration * 1000;
            // std::cout << "not use connpool: " << milliseconds << "ms" << std::endl;

            timePerThreadExec[kth] = milliseconds;
        }, i);
    }

    for (int i = 0; i < numThread; ++i) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }

    double result = 0;
    for (auto t : timePerThreadExec) result += t;

    return result;
}





double notUseConnpoolMultiThread(int numThread, int connCnt) {

    int connCntPerThread = connCnt / numThread;
    std::vector<std::thread> threads;
    std::vector<double> timePerThreadExec(numThread, 0);
    std::cout << connCntPerThread << std::endl;

    {
        Connection conn;
        char sql[1024] = {0};
        sprintf(sql, "delete from stu;");
        conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
        conn.update(sql);
    }


    auto threadFunc = [&](int kth) {
            
        clock_t start = clock();
        for (int i = 0; i < connCntPerThread; ++i) {
            Connection conn;
            char sql[1024] = {0};
            sprintf(sql, "insert into stu(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");
            conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
            conn.update(sql);
        }

        clock_t end = clock();
        double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
        double milliseconds = duration * 1000;

        timePerThreadExec[kth] = milliseconds;
        // printf("kth=%d, spend time=%10.2f\n", kth, milliseconds);

    };


    for (int i = 0; i < numThread; ++i) {
        threads.emplace_back(threadFunc, i);
    }


    for (int i = 0; i < numThread; ++i) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }

    double result = 0;
    for (auto& t : timePerThreadExec) result += t;

    return result;
}


// g++ -o test_connpool connpool_non_singleton.hpp test_connpool.cpp -lmysqlclient

int main() {

#if 1


    {
        Connection conn;
        conn.connect("127.0.0.1", 3306, "root", "123456", "chat");

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


    std::cout << "\n\n --- not use connpool --- \n" << std::endl; 
    std::vector<std::vector<double>> resultNotUsePool(maxNumThread, std::vector<double>(stepNumConn, 0));
    for (int i = 1; i <= maxNumThread; i++) {
        for (int j = 1; j <= stepNumConn; j++) {
            resultNotUsePool[i - 1][j - 1] = notUseConnpoolMultiThread(2, 8000);
            printf("threadNum: %3d, connNum: %6d, spend time: %10.2f\n", 2, 8000, resultNotUsePool[i - 1][j - 1]);
        }
    }


#if 0

    std::cout << "\n\n --- use connpool --- \n" << std::endl; 
    std::vector<std::vector<double>> resultUsePool(maxNumThread, std::vector<double>(stepNumConn, 0));
    for (int i = 1; i <= maxNumThread; i++) {
        for (int j = 1; j <= stepNumConn; j++) {
            resultNotUsePool[i - 1][j - 1] = UseConnpoolMultiThread(i, j * stepLenConn);
        }
    }

#endif

    printf("\n\n\n");


#endif

	// Connection conn;
	// conn.connect("127.0.0.1", 3306, "root", "123456", "chat");

	/*Connection conn;
	char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
		"zhang san", 20, "male");
	conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
	conn.update(sql);*/

	// clock_t begin = clock();
	
	// std::thread t1([]() {
	// 	//ConnectionPool *cp = ConnectionPool::getConnectionPool();
	// 	for (int i = 0; i < 4000; ++i)
	// 	{
	// 		/*char sql[1024] = { 0 };
	// 		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
	// 			"zhang san", 20, "male");
	// 		shared_ptr<Connection> sp = cp->getConnection();
	// 		sp->update(sql);*/
	// 		Connection conn;
	// 		char sql[1024] = { 0 };
	// 		sprintf(sql, "insert into stu(name,age,sex) values('%s',%d,'%s')",
	// 			"zhang san", 20, "male");
	// 		conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
	// 		conn.update(sql);
	// 	}
	// });


	// std::thread t2([]() {
	// 	//ConnectionPool *cp = ConnectionPool::getConnectionPool();
	// 	for (int i = 0; i < 4000; ++i)
	// 	{
	// 		/*char sql[1024] = { 0 };
	// 		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
	// 			"zhang san", 20, "male");
	// 		shared_ptr<Connection> sp = cp->getConnection();
	// 		sp->update(sql);*/
	// 		Connection conn;
	// 		char sql[1024] = { 0 };
	// 		sprintf(sql, "insert into stu(name,age,sex) values('%s',%d,'%s')",
	// 			"zhang san", 20, "male");
	// 		conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
	// 		conn.update(sql);
	// 	}
	// });

#if 0


	std::thread t3([]() {
		//ConnectionPool *cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i)
		{
			/*char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);*/
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
			conn.update(sql);
		}
	});
	std::thread t4([]() {
		//ConnectionPool *cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i)
		{
			/*char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);*/
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
			conn.update(sql);
		}
	});

#endif

	// t1.join();
	// t2.join();

#if 0

	t3.join();
	t4.join();

#endif

	// clock_t end = clock();
	// std::cout << (end - begin) / 1000 << "ms" << std::endl;


#if 0
	for (int i = 0; i < 10000; ++i)
	{
		Connection conn;
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
			"zhang san", 20, "male");
		conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
		conn.update(sql);

		/*shared_ptr<Connection> sp = cp->getConnection();
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
			"zhang san", 20, "male");
		sp->update(sql);*/
	}
#endif














    return 0;
}


















