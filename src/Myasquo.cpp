#include "Myasquo.h"
#include <string>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <boost/asio/placeholders.hpp>

namespace {
std::string path_to_filename(const std::string& path) {
    std::string result = path;
    result.erase(0,path.rfind("/")+1);
    return result;
}

std::string itoa(int i) {
    char buf[20];
    sprintf(buf,"%d",i);
    return std::string(buf);
}
}

#define LOGPREFIX ::path_to_filename(__FILE__)+":"+itoa(__LINE__)+" "

Myasquo::Myasquo(const std::string& hostname,
                 int port,
                 const std::string& username,
                 const std::string& password,
                 const std::string& database,
                 const std::string queuePath):
    m_hostname(hostname),
    m_port(port),
    m_username(username),
    m_password(password),
    m_database(database),
    m_dbQueuePath(queuePath),
    m_connected(false),
    m_ownIoService(true),
    m_ioService(new boost::asio::io_service()),
    m_work(*m_ioService),
    m_reconnectTimer(*m_ioService),
    m_reopenTimer(*m_ioService)
{
    if (!m_dbQueue.open(queuePath)) {
        throw std::runtime_error(m_dbQueue.lastError());
    }

    m_ioService->post(boost::bind(&Myasquo::doConnect,this,boost::system::error_code()));
    if (m_ownIoService)
        m_thread = boost::thread(boost::bind(&boost::asio::io_service::run, m_ioService));
}

Myasquo::Myasquo(const std::string &hostname,
                 int port,
                 const std::string &username,
                 const std::string &password,
                 const std::string &database,
                 const std::string queuePath,
                 boost::asio::io_service &io_service):
    m_hostname(hostname),
    m_port(port),
    m_username(username),
    m_password(password),
    m_database(database),
    m_dbQueuePath(queuePath),
    m_connected(false),
    m_ownIoService(false),
    m_ioService(&io_service),
    m_work(*m_ioService),
    m_reconnectTimer(*m_ioService),
    m_reopenTimer(*m_ioService)
{
    if (!m_dbQueue.open(queuePath)) {
        throw std::runtime_error(m_dbQueue.lastError());
    }

    m_ioService->post(boost::bind(&Myasquo::doConnect,this,boost::system::error_code()));
    if (m_ownIoService)
        m_thread = boost::thread(boost::bind(&boost::asio::io_service::run, m_ioService));
}

Myasquo::~Myasquo()
{
    m_reopenTimer.cancel();
    m_reconnectTimer.cancel();
    m_dbQueue.close();
    if (m_ownIoService)
        delete m_ioService;
    m_thread.join();
}

void Myasquo::handleError()
{
    onError();
    if (!m_dbQueue.is_open()) {
        boost::posix_time::time_duration dur = m_reopenTimer.expires_from_now();
        if (dur.total_nanoseconds() <= 0) {
            m_reopenTimer.expires_from_now(boost::posix_time::seconds(1));
            m_reopenTimer.async_wait(
                        boost::bind(&Myasquo::doOpenQueue, this, boost::asio::placeholders::error)
                        );
        }
    }

    if (!m_connected) {
        boost::posix_time::time_duration dur = m_reconnectTimer.expires_from_now();
        if (dur.total_nanoseconds() <= 0) {
            m_reconnectTimer.expires_from_now(boost::posix_time::seconds(1));
            m_reconnectTimer.async_wait(
                        boost::bind(&Myasquo::doConnect, this, boost::asio::placeholders::error)
                        );
        }
    }
}

void Myasquo::query(const std::string &query)
{
    m_ioService->post(boost::bind(&Myasquo::doQuery, this, query));
}

void Myasquo::ping()
{
    m_ioService->post(boost::bind(&Myasquo::doPing, this));
}


void Myasquo::doConnect(const boost::system::error_code& e)
{
    if (e == boost::asio::error::operation_aborted)
        return;

    m_conn = mysql_init(NULL);
    if (!m_conn) throw std::runtime_error("Not enough memory to allocate MySQL");
    m_conn->free_me = 1;
    m_conn->reconnect = 0;

    if (!mysql_real_connect(m_conn,
                            m_hostname.c_str(),
                            m_username.c_str(),
                            m_password.c_str(),
                            m_database.c_str(),
                            m_port,
                            NULL,
                            CLIENT_INTERACTIVE))
    {
        onLogMessage(LOGPREFIX+"Connection to MySQL failed: "+
                     std::string(mysql_error(m_conn)),LOG_LEVEL_WARNING);
        m_connected = false;
        handleError();
        return;
    }

    onLogMessage(LOGPREFIX+"Successfully connected to MySQL",LOG_LEVEL_INFO);
    m_connected = true;

    if (!m_dbQueue.empty())
        doProcessDBQueue();

    onConnect();
}

void Myasquo::doOpenQueue(const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted)
        return;

    if (!m_dbQueue.open(m_dbQueuePath)) {
        onLogMessage(LOGPREFIX+m_dbQueue.lastError(),LOG_LEVEL_WARNING);
        handleError();
    } else {
        onLogMessage(LOGPREFIX+"DBQueue opened: path: "+
                     m_dbQueuePath+" empty:"+ (m_dbQueue.empty()?"yes":"no"),LOG_LEVEL_INFO);
        if (m_connected && !m_dbQueue.empty())
            doProcessDBQueue();
    }
}

void Myasquo::doQuery(const std::string& msg)
{
    if (m_connected) {
        int err = executeQuery(msg);
        if (err) {
            onLogMessage(LOGPREFIX+"Query execution failed: "+ mysql_error(m_conn) +" Query: "+msg,LOG_LEVEL_WARNING);
            if (err>=CR_MIN_ERROR) {
                onLogMessage(LOGPREFIX+"Critical error, reconnecting",LOG_LEVEL_ALERT);
                if (!doPushToDBQueue(msg))
                    m_dbQueue.close();

                mysql_close(m_conn);
                m_connected = false;
                handleError();
            }
        } else
            onLogMessage(LOGPREFIX+"Query execution succeeded. Query: "+msg,LOG_LEVEL_DEBUG);

    }
    else if (!doPushToDBQueue(msg)) {
        m_dbQueue.close();
        handleError();
    }
}

bool Myasquo::doPushToDBQueue(const std::string& msg)
{
    if (m_dbQueue.is_open()) {
        if (!m_dbQueue.push(msg)) {
            onLogMessage(LOGPREFIX+m_dbQueue.lastError(),LOG_LEVEL_WARNING);
            return false;
        } else
            onLogMessage(LOGPREFIX+"Value pushed to DBQueue: "+msg,LOG_LEVEL_DEBUG);
    } else
        onLogMessage(LOGPREFIX+"DBQueue is not opened. Discarding query: "+msg,LOG_LEVEL_WARNING);
    return true;
}

void Myasquo::doProcessDBQueue()
{
    if (!m_connected || m_dbQueue.empty()) return;

    while (!m_dbQueue.empty()) {

        std::string msg = m_dbQueue.front();
        if (msg.empty()) {
            onLogMessage(LOGPREFIX+m_dbQueue.lastError(),LOG_LEVEL_WARNING);
            m_dbQueue.close();
            handleError();
            return;
        } else {
            onLogMessage(LOGPREFIX+"Value taken from DBQueue: "+msg,LOG_LEVEL_DEBUG);
        }

        int err = executeQuery(msg);
        if (err && err>=CR_MIN_ERROR) {
            mysql_close(m_conn);
            m_connected = false;
            handleError();
            return;
        }

        if (!m_dbQueue.pop()) {
            onLogMessage(LOGPREFIX+m_dbQueue.lastError(),LOG_LEVEL_WARNING);
            m_dbQueue.close();
            handleError();
            return;
        } else
            onLogMessage(LOGPREFIX+"Value poped from DBQueue (empty: "+
                         (m_dbQueue.empty()?"yes)":"no)"),LOG_LEVEL_DEBUG);
    }
}

void Myasquo::doPing()
{
    onLogMessage(LOGPREFIX+"Pinging mysql",LOG_LEVEL_DEBUG);
    if (mysql_ping(m_conn) != 0) {
        onLogMessage(LOGPREFIX+"Connection to MySQL failed: "+
                     std::string(mysql_error(m_conn)),LOG_LEVEL_WARNING);
        m_connected = false;
        handleError();
    }
}

int Myasquo::executeQuery(const std::string& msg)
{
    onLogMessage(LOGPREFIX+"Executing query: "+msg,LOG_LEVEL_DEBUG);

    if (mysql_query(m_conn,msg.c_str()) != 0) {
        onLogMessage(LOGPREFIX+"MySQL Query failed: "+
                     std::string(mysql_error(m_conn)),LOG_LEVEL_WARNING);
        return  mysql_errno(m_conn);
    }

    onQueryResult(MySQLResult(m_conn,msg));

    return 0;
}
