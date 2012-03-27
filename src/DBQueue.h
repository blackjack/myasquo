#ifndef DBQUEUE_H
#define DBQUEUE_H
#include <fstream>
#include <boost/thread/mutex.hpp>
#include <errno.h>

class DBQueue
{
public:
    DBQueue();
    bool open(const std::string& path);
    void close();

    bool push(const std::string& line);
    bool pop();
    std::string front();


    bool empty() { return m_empty; }
    bool is_open() { return m_isOpen; }
    const std::string& lastError() { return m_lastError; }
private:
    void setSeek(int seek);
    void reopenQueue(std::ios_base::openmode mode);

    void setLastError(std::string prefix, std::string filename);
private:
    std::fstream m_queueFile;
    std::fstream m_indexFile;
    std::ios_base::openmode m_lastOpenMode;
    int m_seek;
    bool m_empty;
    bool m_isOpen;
    std::string m_queuePath;
    std::string m_indexPath;
    std::string m_lastError;
};

#endif // DBQUEUE_H
