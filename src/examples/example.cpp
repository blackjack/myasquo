#include "../Myasquo.h"
#include <iostream>

class MyClient: public Myasquo
{
public:
    MyClient(const std::string& hostname, int port, const std::string& username, const std::string& password, const std::string& database, const std::string queuePath):
        Myasquo(hostname,port,username,password,database,queuePath)
    {}
    void onLogMessage(const std::string &message) {
        std::cout << message << std::endl;
    }
};

int main() {

    MyClient client("localhost", 3306, "root", "homeofus", "timeseries", "/tmp");
    while (true) {
        client.query("SELECT 1");
        sleep(1);
    }

    return 0;
}
