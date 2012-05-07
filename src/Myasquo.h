#ifndef MYSQLASYNC_H
#define MYSQLASYNC_H

#include <deque>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread.hpp>
#include "DBQueue.h"

typedef struct st_mysql MYSQL;

class Myasquo
{
public:
    Myasquo(const std::string& hostname, int port, const std::string& username, const std::string& password, const std::string& database, const std::string queuePath);
    ~Myasquo();

    void query(const std::string &query);
    void ping();
    virtual void onConnect() {}
    virtual void onLogMessage(const std::string& message, int logLevel) {}
    virtual void onError() {}
public:
    static const int LOG_LEVEL_EMERG = 0;
    static const int LOG_LEVEL_ALERT = 1;
    static const int LOG_LEVEL_CRIT = 2;
    static const int LOG_LEVEL_ERR = 3;
    static const int LOG_LEVEL_WARNING = 4;
    static const int LOG_LEVEL_NOTICE = 5;
    static const int LOG_LEVEL_INFO = 6;
    static const int LOG_LEVEL_DEBUG = 7;


protected:
    boost::asio::io_service& ioService() { return m_ioService; }
    void doConnect(const boost::system::error_code &e);
    void doOpenQueue(const boost::system::error_code &e);
    void doQuery(const std::string &msg);
    bool doPushToDBQueue(const std::string &msg);
    void doProcessDBQueue();
    void doPing();
    int executeQuery(const std::string &msg);
    void handleError();
private:
    std::string m_hostname;
    int m_port;
    std::string m_username;
    std::string m_password;
    std::string m_database;
    std::string m_dbQueuePath;
    bool m_connected;

    MYSQL* m_conn;
    boost::thread m_thread;
    boost::asio::io_service m_ioService;
    boost::asio::io_service::work m_work;
    boost::asio::deadline_timer m_reconnectTimer;
    boost::asio::deadline_timer m_reopenTimer;

    DBQueue m_dbQueue;
};

#endif // MYSQLASYNC_H
