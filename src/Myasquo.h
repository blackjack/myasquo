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
    virtual void onLogMessage(const std::string& message) {}
    virtual void onError() {}
protected:
    boost::asio::io_service& ioService() { return m_ioService; }
    void doConnect();
    void doOpenQueue();
    void doQuery(const std::string &msg);
    void doPing();
    void executeQuery();
    void handleError();
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
    boost::asio::io_service::work m_work;
    boost::asio::deadline_timer m_reconnectTimer;
    boost::asio::deadline_timer m_reopenTimer;
    bool m_reconnectTimerActive;
    bool m_reopenTimerActive;
    bool m_startReopenTimer;
    bool m_startReconnectTimer;

    std::deque<std::string> m_writeBuffer;
    DBQueue m_dbQueue;
};

#endif // MYSQLASYNC_H
