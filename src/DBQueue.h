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
    bool fail() { return m_file.fail();}
    bool is_open() { return m_isOpen; }
private:
    bool setSeek(int seek);
    inline bool reopenQueue(std::ios_base::openmode mode);
private:
    std::fstream m_file;
    std::fstream m_indexFile;
    std::ios_base::openmode m_lastOpenMode;
    int m_seek;
    bool m_empty;
    bool m_isOpen;
    std::string m_queuePath;
    std::string m_indexPath;
    boost::mutex mutex;
};

#endif // DBQUEUE_H
