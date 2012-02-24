#include "DBQueue.h"
#include "string.h"

DBQueue::DBQueue():
    m_seek(0),m_isOpen(false)
{
}

bool DBQueue::open(const std::string& path)
{
    if (path.at(path.size()-1)=='/') {
        m_queuePath = path; m_queuePath+="queue";
        m_indexPath = path; m_indexPath+="index";
    } else {
        m_queuePath = path; m_queuePath+="/queue";
        m_indexPath = path; m_indexPath+="/index";
    }

    m_indexFile.open(m_indexPath.c_str(),std::ios_base::out | std::ios_base::app);
    m_file.open(m_queuePath.c_str(),std::ios_base::out | std::ios_base::app);

    if (!m_indexFile.is_open() || !m_file.is_open())
        return false;

    m_indexFile.close();
    m_file.close();

    m_indexFile.open(m_indexPath.c_str(),std::ios_base::in);
    m_indexFile >> m_seek;

    if (m_indexFile.bad()) {
        m_seek = 0;
        m_indexFile.clear();
    }

    m_indexFile.close();

    m_file.open(m_queuePath.c_str(),std::ios_base::in);

    m_file.seekg(0,std::ios_base::end);
    int t = m_file.tellg();
    if (t == m_seek) {
        m_empty = true;
        reopenQueue(std::ios_base::out | std::ios_base::trunc);
    } else {
        m_empty = false;
        reopenQueue(std::ios_base::out | std::ios_base::app);
        if (m_seek > 0)
            m_file.seekg(m_seek);
        else
            m_file.seekg(0,std::ios_base::beg);
    }

    m_isOpen = true;
    return true;
}

void DBQueue::close() {
    m_isOpen = false;
    m_file.close();
    m_file.clear();
    m_indexFile.close();
    m_indexFile.clear();
}

bool DBQueue::push(const std::string &line)
{
    if (m_lastOpenMode & std::ios_base::in) {
        if (!reopenQueue(std::ios_base::out | std::ios_base::app))
            return false;
    }
    m_file << line << std::endl;
    m_file.flush();
    m_empty = false;

    return !m_file.bad();
}

bool DBQueue::pop()
{
    if (m_lastOpenMode & std::ios_base::out) {
        if (!reopenQueue(std::ios_base::in))
            return false;

        if (m_seek >0)
            m_file.seekg(m_seek);
        else
            m_file.seekg(0,std::ios_base::beg);
    }

    std::string s;
    std::getline(m_file,s);

    if (m_file.bad() || !setSeek(m_file.tellg()))
        return false;

    m_file.seekg(0,std::ios_base::end);
    int t = m_file.tellg();
    if (t == m_seek) {
        m_empty = true;
        if (!setSeek(0) || !reopenQueue(std::ios_base::out | std::ios_base::trunc))
            return false;
    } else {
        m_empty = false;
        m_file.seekg(m_seek);
    }
    return true;
}

std::string DBQueue::front()
{
    if (m_lastOpenMode & std::ios_base::out) {
        if (!reopenQueue(std::ios_base::in))
            return std::string();

        if (m_seek >0)
            m_file.seekg(m_seek);
        else
            m_file.seekg(0,std::ios_base::beg);
    }


    int t = m_file.tellg();
    std::string result;
    std::getline(m_file,result);
    m_file.seekg(t);

    if (m_file.bad())
        return std::string();
    return result;
}

bool DBQueue::setSeek(int seek)
{
    m_indexFile.open(m_indexPath.c_str(),std::ios_base::out | std::ios_base::trunc);
    if (!m_indexFile) return false;
    m_indexFile << seek;
    if (m_indexFile.bad()) return false;
    m_indexFile.close();
    m_seek = seek;
    return true;
}

bool DBQueue::reopenQueue(std::ios_base::openmode mode)
{
    m_file.close();
    m_file.open(m_queuePath.c_str(),mode);
    m_lastOpenMode = mode;

    return m_file.is_open();
}
