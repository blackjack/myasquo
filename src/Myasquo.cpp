#include "Myasquo.h"
#include <string>
#include "event.h"

MysqlAsync::MysqlAsync(const std::string& hostname, int port, const std::string& username, const std::string& password, const std::string& database, const std::string queuePath):
    m_hostname(hostname),
    m_port(port),
    m_username(username),
    m_password(password),
    m_database(database),
    m_dbQueuePath(queuePath),
    m_connected(false),
    m_writeInProgress(false),
    m_reconnectTimer(m_ioService),
    m_reopenTimer(m_ioService)
{
    m_conn = mysql_init(NULL);
    m_conn->free_me = 1;

    if (!m_dbQueue.open(queuePath))
        throw std::runtime_error("Could not open query queue: "+std::string(strerror(errno)));

    m_thread = boost::thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
    m_ioService.post(boost::bind(&MysqlAsync::doConnect,this));
}

MysqlAsync::~MysqlAsync()
{
    m_reopenTimer.cancel();
    m_reconnectTimer.cancel();
    m_dbQueue.close();
    m_ioService.stop();
    m_thread.join();
}

void MysqlAsync::onError()
{
    if (!m_dbQueue.is_open() && !m_dbQueue.open(m_dbQueuePath)) {
        m_reopenTimer.expires_from_now(boost::posix_time::seconds(1));
        m_reopenTimer.async_wait(boost::bind(&MysqlAsync::doConnect, this));
    }
    if (!m_connected) {
        m_reconnectTimer.expires_from_now(boost::posix_time::seconds(1));
        m_reconnectTimer.async_wait(boost::bind(&MysqlAsync::doConnect, this));
    }
}

void MysqlAsync::query(const std::string &query)
{
    m_ioService.post(boost::bind(&MysqlAsync::doQuery, this, query));
}


void MysqlAsync::doConnect()
{
    if (!mysql_real_connect(m_conn, m_hostname.c_str(), m_username.c_str(), m_password.c_str(), m_database.c_str(), m_port, NULL, 0)) {
        onLogMessage("Connection to MySQL failed: "+std::string(mysql_error(m_conn)));
        m_connected = false;
        onError();
    }
    m_connected = true;
    if (!m_writeBuffer.empty())
        executeQuery();
}

void MysqlAsync::doOpenQueue() {
    if (!m_dbQueue.open(m_dbQueuePath)) {
        onLogMessage("Could not open query queue: "+std::string(strerror(errno)));
        onError();
    } else if (m_connected && !m_writeInProgress)
        executeQuery();
}

void MysqlAsync::doQuery(const std::string& msg)
{
    m_writeBuffer.push_back(msg);
    if (m_connected && !m_writeInProgress) {
        executeQuery();
    }
}

void MysqlAsync::executeQuery()
{
    m_writeInProgress = true;

    std::string msg;
    if (!m_dbQueue.empty()) {
        msg = m_dbQueue.front();
        if (msg.empty()) {
            onLogMessage("Could not get query queue item: "+std::string(strerror(errno)));
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
                onLogMessage("Could not push query queue item: "+std::string(strerror(errno)));
                m_dbQueue.close();
            }
        }
        onLogMessage("MySQL Query failed: "+std::string(mysql_error(m_conn)));
        onError();
        return;
    }

    if (!m_dbQueue.empty()) {
        if (!m_dbQueue.pop()) {
            onLogMessage("Could not pop query queue item: "+std::string(strerror(errno)));
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
    m_ioService.post(boost::bind(&MysqlAsync::executeQuery, this));
}
