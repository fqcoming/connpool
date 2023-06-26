
#include "connpool.hpp"

#define ENABLE_CONNPOOL     0







int main(int argc, char *argv[]) {



#if ENABLE_CONNPOOL


#else
    Connection conn;
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, age, sex) values('%s', %d, '%s')", "John", 24, "male");

    conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
    conn.update(sql);
#endif

    return 0;
}




// mysql -u root -p 123456
/****************************
 *  create database chat;
    DROP TABLE chat.user;
    CREATE TABLE chat.user (
        name VARCHAR(255),
        age INT,
        sex VARCHAR(10)
    );
****************************/
// g++ -o test_connpool test_connpool.cpp -lmysqlclient















