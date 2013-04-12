#include "../Myasquo.h"
#include <iostream>

class MyClient: public Myasquo
{
public:
    MyClient(const std::string& hostname, int port, const std::string& username, const std::string& password, const std::string& database, const std::string queuePath):
        Myasquo(hostname,port,username,password,database,queuePath)
    {}
    void onLogMessage(const std::string &message,int logLevel) {
        std::cout << message << std::endl;
    }

    bool onQueryResultRow(char **row, unsigned int fieldsNum, unsigned long *lengths) {
        for (unsigned i = 0; i<fieldsNum; ++i) {
            std::cout << row[i] << " ";
        }
        std::cout << std::endl;
        return true;
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
