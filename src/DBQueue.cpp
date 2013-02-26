#include "DBQueue.h"
#include "string.h"
#include "boost/filesystem.hpp"

DBQueue::DBQueue():
    m_seek(0),m_isOpen(false)
{
    m_indexFile.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    m_queueFile.exceptions(std::ios_base::failbit | std::ios_base::badbit);
}

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

#define TRY(operation,result,filename) try { operation; } catch (const std::fstream::failure& e) { setLastError(::path_to_filename(__FILE__)+":"+itoa(__LINE__)+" "+result,filename); return false; }

bool DBQueue::open(const std::string& path)
{
    try {
        boost::filesystem::create_directories(path);
    } catch (...) {}

    if (path.at(path.size()-1)=='/') {
        m_queuePath = path; m_queuePath+="queue";
        m_indexPath = path; m_indexPath+="index";
    } else {
        m_queuePath = path; m_queuePath+="/queue";
        m_indexPath = path; m_indexPath+="/index";
    }

    TRY(m_indexFile.open(m_indexPath.c_str(),std::ios_base::out | std::ios_base::app),"Error opening index file",m_indexPath);
    TRY(m_queueFile.open(m_queuePath.c_str(),std::ios_base::out | std::ios_base::app),"Error opening queue file",m_queuePath);
    TRY(m_indexFile.close(),"Error closing",m_indexPath);
    TRY(m_queueFile.close(),"Error closing",m_queuePath);

    TRY(m_indexFile.open(m_indexPath.c_str(),std::ios_base::in),"Error opening",m_indexPath);

    try {
        m_indexFile >> m_seek;
    } catch (std::exception e) {
        m_seek = 0;
        m_indexFile.clear();
    }

    TRY(m_indexFile.close(),"Error closing",m_indexPath);

    TRY(m_queueFile.open(m_queuePath.c_str(),std::ios_base::in),"Error opening",m_queuePath);

    TRY(m_queueFile.seekg(0,std::ios_base::end),"Error seeking queue to the end","");

    int t = m_queueFile.tellg();
    if (t <= m_seek) {
        m_empty = true;
        TRY(reopenQueue(std::ios_base::out | std::ios_base::trunc),"Error reopening queue file",m_queuePath);
    } else {
        m_empty = false;
        TRY(reopenQueue(std::ios_base::out | std::ios_base::app),"Error reopening queue file",m_queuePath);
        if (m_seek > 0) {
            TRY(m_queueFile.seekg(m_seek),"Error seeking queue to the "+::itoa(m_seek),"");
        }
        else {
            TRY(m_queueFile.seekg(0,std::ios_base::beg),"Error seeking queue to the beginning","");
        }
    }

    m_isOpen = true;
    return true;
}

void DBQueue::close() {
    m_isOpen = false;
    try { m_queueFile.clear(); } catch (...) {}
    try { m_indexFile.clear(); } catch (...) {}
    try { m_queueFile.close(); } catch (...) {}
    try { m_indexFile.close(); } catch (...) {}
}

bool DBQueue::push(const std::string &line)
{
    if (m_lastOpenMode & std::ios_base::in) {
        TRY(reopenQueue(std::ios_base::out | std::ios_base::app),"Error reopening queue file","");
    }

    TRY(m_queueFile << line << std::endl,"Error pushing line into queue","");
    TRY(m_queueFile.flush(),"Error pushing line into queue","");
    m_empty = false;

    return true;
}

bool DBQueue::pop()
{
    if (m_lastOpenMode & std::ios_base::out) {

        TRY(reopenQueue(std::ios_base::in),"Error reopening queue file","");

        if (m_seek >0) {
            TRY(m_queueFile.seekg(m_seek),"Error seeking queue to the "+::itoa(m_seek),"");
        }
        else {
            TRY(m_queueFile.seekg(0,std::ios_base::beg),"Error seeking queue to the beginning","");
        }
    }

    std::string s;
    try {
        std::getline(m_queueFile,s);
    } catch (const std::fstream::failure& e) {
        setLastError(::path_to_filename(__FILE__)+":"+itoa(__LINE__)+" "+"Error getting line from queue","");
        return false;
    }

    TRY(setSeek(m_queueFile.tellg()),"Error setting seek position","");

    TRY(m_queueFile.seekg(0,std::ios_base::end),"Error seeking queue to the end","");

    int t = m_queueFile.tellg();
    if (t <= m_seek) {
        m_empty = true;
        TRY(setSeek(0),"Error setting seek position","");
        TRY(reopenQueue(std::ios_base::out | std::ios_base::trunc),"Error reopening queue file","");
    } else {
        m_empty = false;
        TRY(m_queueFile.seekg(m_seek),"Error seeking queue to the "+::itoa(m_seek),"");
    }
    return true;
}

#undef TRY
#define TRY(operation,result,filename) try { operation; } catch (const std::fstream::failure& e) { setLastError(::path_to_filename(__FILE__)+":"+itoa(__LINE__)+" "+result,filename); return std::string(); }
std::string DBQueue::front()
{
    if (m_lastOpenMode & std::ios_base::out) {
        TRY(reopenQueue(std::ios_base::in),"Error reopening queue file","");

        if (m_seek >0) {
            TRY(m_queueFile.seekg(m_seek),"Error seeking queue to the "+::itoa(m_seek),"");
        }
        else {
            TRY(m_queueFile.seekg(0,std::ios_base::beg),"Error seeking queue to the beginning","");
        }
    }


    int t = m_queueFile.tellg();
    std::string result;

    try {
        std::getline(m_queueFile,result);
    } catch (const std::fstream::failure& e) {
        setLastError(::path_to_filename(__FILE__)+":"+itoa(__LINE__)+" "+"Error getting line from queue","");
        return std::string();
    }

    TRY(m_queueFile.seekg(t),"Error seeking queue to the "+::itoa(t),"");

    return result;
}
#undef TRY_S

void DBQueue::setSeek(int seek)
{
    m_indexFile.open(m_indexPath.c_str(),std::ios_base::out | std::ios_base::trunc);
    m_indexFile << seek;
    m_indexFile.close();
    m_seek = seek;
}

void DBQueue::reopenQueue(std::ios_base::openmode mode)
{
    m_queueFile.close();
    m_queueFile.open(m_queuePath.c_str(),mode);
    m_lastOpenMode = mode;
}

void DBQueue::setLastError(std::string prefix, std::string filename)
{
    m_lastError = prefix + " " + (filename.empty()?"":filename) + (errno!=0?std::string(": ")+strerror(errno):"");
}

#undef TRY
