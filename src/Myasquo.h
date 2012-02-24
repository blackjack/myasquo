#ifndef MYSQLASYNC_H
#define MYSQLASYNC_H

#include <deque>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/thread.hpp>
#include <mysql/mysql.h>
#include "DBQueue.h"

class event_base;

class MysqlAsync
{
public:
    MysqlAsync(const std::string& hostname, int port, const std::string& username, const std::string& password, const std::string& database, const std::string queuePath);
    void query(const std::string &query);
    ~MysqlAsync();
    virtual void onLogMessage(const std::string) {}
    virtual void onError();
private:
    void doConnect();
    void doOpenQueue();
    void doQuery(const std::string &msg);
    void executeQuery();
private:
    std::string m_hostname;
    int m_port;
    std::string m_username;
    std::string m_password;
    std::string m_database;
    std::string m_dbQueuePath;
    bool m_connected;
    bool m_writeInProgress;

    MYSQL* m_conn;
    boost::thread m_thread;
    boost::asio::io_service m_ioService;
    boost::asio::deadline_timer m_reconnectTimer;
    boost::asio::deadline_timer m_reopenTimer;

    std::deque<std::string> m_writeBuffer;
    DBQueue m_dbQueue;
};

#endif // MYSQLASYNC_H
