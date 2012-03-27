#include "Myasquo.h"
#include <string>
#include <mysql/mysql.h>

Myasquo::Myasquo(const std::string& hostname, int port, const std::string& username, const std::string& password, const std::string& database, const std::string queuePath):
    m_hostname(hostname),
    m_port(port),
    m_username(username),
    m_password(password),
    m_database(database),
    m_dbQueuePath(queuePath),
    m_connected(false),
    m_writeInProgress(false),
    m_work(m_ioService),
    m_reconnectTimer(m_ioService),
    m_reopenTimer(m_ioService),
    m_reconnectTimerActive(false),
    m_reopenTimerActive(false),
    m_startReopenTimer(false),
    m_startReconnectTimer(false)
{
    m_conn = mysql_init(NULL);
    if (!m_conn) throw std::runtime_error("Not enough memory to allocate MySQL");
    m_conn->free_me = 1;
    m_conn->reconnect = 0;

    if (!m_dbQueue.open(queuePath))
        throw std::runtime_error(m_dbQueue.lastError());

    m_ioService.post(boost::bind(&Myasquo::doConnect,this));
    m_thread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
}

Myasquo::~Myasquo()
{
    m_reopenTimer.cancel();
    m_reconnectTimer.cancel();
    m_dbQueue.close();
    m_ioService.stop();
    m_thread.join();
}

void Myasquo::onError()
{
    if (!m_dbQueue.is_open()) {
        if (!m_reopenTimerActive) {
            if (!m_startReopenTimer) {
                m_startReopenTimer = true;
                doOpenQueue();
            } else {
                m_reopenTimerActive = true;
                m_reopenTimer.expires_from_now(boost::posix_time::seconds(1));
                m_reopenTimer.async_wait(boost::bind(&Myasquo::doOpenQueue, this));
            }
        }
    } else if (!m_connected) {
        while (!m_writeBuffer.empty()) {
            if (!m_dbQueue.push(m_writeBuffer.front())) {
                m_dbQueue.close();
                m_reopenTimer.async_wait(boost::bind(&Myasquo::onError, this));
                break;
            } else
                m_writeBuffer.pop_front();
        }
    }

    if (!m_connected && !m_reconnectTimerActive) {
        if (!m_startReconnectTimer) {
            m_startReconnectTimer = true;
            doConnect();
        } else {
            m_reconnectTimerActive = true;
            m_reconnectTimer.expires_from_now(boost::posix_time::seconds(1));
            m_reconnectTimer.async_wait(boost::bind(&Myasquo::doConnect, this));
        }
    }
}

void Myasquo::query(const std::string &query)
{
    m_ioService.post(boost::bind(&Myasquo::doQuery, this, query));
}

void Myasquo::ping()
{
    m_ioService.post(boost::bind(&Myasquo::doPing, this));
}


void Myasquo::doConnect()
{
    m_reconnectTimerActive = false;
    if (!mysql_real_connect(m_conn, m_hostname.c_str(), m_username.c_str(), m_password.c_str(), m_database.c_str(), m_port, NULL, CLIENT_INTERACTIVE)) {
        onLogMessage("Connection to MySQL failed: "+std::string(mysql_error(m_conn)));
        m_connected = false;
        onError();
        return;
    }
    m_connected = true;

    m_startReconnectTimer = false;
    if (!m_dbQueue.empty() || !m_writeBuffer.empty())
        executeQuery();

    onConnect();
}

void Myasquo::doOpenQueue() {
    m_reopenTimerActive = false;
    if (!m_dbQueue.open(m_dbQueuePath)) {
        onLogMessage(m_dbQueue.lastError());
        onError();
    } else {
        m_startReopenTimer = false;
        while (!m_writeBuffer.empty()) {
            if (!m_dbQueue.push(m_writeBuffer.front())) {
                onLogMessage(m_dbQueue.lastError());
                m_dbQueue.close();
                onError();
                break;
            } else
                m_writeBuffer.pop_front();
        }
        if (m_connected && !m_writeInProgress)
            executeQuery();
    }
}

void Myasquo::doQuery(const std::string& msg)
{
    if (m_connected)
        m_writeBuffer.push_back(msg);
    else
        if (!m_dbQueue.push(msg)) {
            onLogMessage(m_dbQueue.lastError());
            m_dbQueue.close();
        }
    if (m_connected && !m_writeInProgress) {
        executeQuery();
    }
}

void Myasquo::doPing()
{
    if (mysql_ping(m_conn) != 0) {
        onLogMessage("Connection to MySQL failed: "+std::string(mysql_error(m_conn)));
        m_connected = false;
        onError();
    }
}

void Myasquo::executeQuery()
{
    m_writeInProgress = true;

    std::string msg;
    if (!m_dbQueue.empty()) {
        msg = m_dbQueue.front();
        if (msg.empty()) {
            onLogMessage(m_dbQueue.lastError());
            m_dbQueue.close();
            m_writeInProgress = false;
            onError();
            return;
        }
    } else if (!m_writeBuffer.empty()) {
        msg = m_writeBuffer.front();
    } else {
        m_writeInProgress = false;
        return;
    }

    if (mysql_query(m_conn,msg.c_str()) != 0) {
        m_connected = false;
        m_writeInProgress = false;
        if (m_dbQueue.empty()) {//query taken from m_writeBuffer
            if (!m_dbQueue.push(msg)) {
                onLogMessage(m_dbQueue.lastError());
                m_dbQueue.close();
            }
        }
        onLogMessage("MySQL Query failed: "+std::string(mysql_error(m_conn)));
        onError();
        return;
    }

    MYSQL_RES* result = mysql_store_result(m_conn);
    if (result)
        mysql_free_result(result);

    if (!m_dbQueue.empty()) {
        if (!m_dbQueue.pop()) {
            onLogMessage(m_dbQueue.lastError());
            m_dbQueue.close();
            m_writeInProgress = false;
            onError();
            return;
        } else if (m_dbQueue.empty() && m_writeBuffer.empty()) {
            m_writeInProgress = false;
            return;
        }
    }
    else {
        m_writeBuffer.pop_front();
        if (m_writeBuffer.empty()) {
            m_writeInProgress = false;
            return;
        }
    }
    m_ioService.post(boost::bind(&Myasquo::executeQuery, this));
}
