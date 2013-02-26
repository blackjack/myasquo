#include "../Myasquo.h"
#include <iostream>
#include <mysql/mysql.h>

class MyClient: public Myasquo
{
public:
    MyClient(const std::string& hostname, int port, const std::string& username, const std::string& password, const std::string& database, const std::string queuePath):
        Myasquo(hostname,port,username,password,database,queuePath)
    {}
    void onLogMessage(const std::string &message,int logLevel) {
        std::cout << message << std::endl;
    }

    void onQueryResult(MySQLResult result) {
        std::cout << "Result got: " << std::endl;
        for (unsigned i = 0; i<result.size(); ++i) {
            MySQLResult::row row = result.getRow(i);
            for (unsigned l = 0; l<row.length; ++l ) {
                MySQLResult::column column = row[l];
                std::cout << std::string(column.data,column.length) << " ";
            }
            std::cout << std::endl;
        }
    }
};


int main() {

    std::string user, password;
    std::cout << "Enter user and password separated by space" << std::endl;
    std::cin >> user >> password;
    MyClient client("localhost", 3306, user, password, "timeseries", "/tmp");


    while (true) {
        client.query("SELECT * FROM `1min` LIMIT 0 , 30");
        sleep(1);
    }

    return 0;
}
