## mysql connection pool

### 1.install mysql

```shell
sudo apt install mysql-server  # ubuntu20 
sudo apt install libmysqlclient-dev
mysql --version
# mysql  Ver 8.0.33-0ubuntu0.22.04.2 for Linux on x86_64 ((Ubuntu))
```

```shell
sudo systemctl status mysql
sudo cat /etc/mysql/debian.cnf 
mysql -u debian-sys-maint -p
```

only local login
```sql
USE mysql;
SELECT User,Host,plugin FROM mysql.user WHERE user='root';
ALTER USER 'root'@'localhost' IDENTIFIED WITH caching_sha2_password BY '123456'; 
flush privileges;
exit;
```

set remote login
```shell
mysql -u root -p   # 123456
```

```sql
UPDATE mysql.user SET Host='%' WHERE User='root' AND Host='localhost';
flush privileges;
ALTER USER root IDENTIFIED WITH caching_sha2_password BY '123456';
flush privileges;
```

see also:
http://blog.chinaunix.net/uid-192452-id-5862571.html


### 2.test connpool.hpp


```shell
mysql -u root -p 123456
``` 

```sql
CREATE DATABASE chat;
USE chat;
CREATE TABLE chat.user (
    name VARCHAR(255),
    age INT,
    sex VARCHAR(10)
);
```

```shell
g++ -o test_connpool test_connpool.cpp -lmysqlclient
```









