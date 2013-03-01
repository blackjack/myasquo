#ifndef MYSQLRESULT_H
#define MYSQLRESULT_H

#include <string>
#include <mysql/mysql.h>

class MySQLResult //lazy class that gets you handy access to MYSQL response
{
    MySQLResult(MYSQL* mysql, const std::string& originalQuery);
public:
    ~MySQLResult();

    const std::string& originalQuery();
    const char* error();

    struct column {
        column(char* d, unsigned int l)
            : data(d),length(l)
        {
        }
        char* data;
        unsigned int length;
    };

    struct row {
        row(char** d, unsigned int l, unsigned long* ls)
            : data(d),length(l),column_lengths(ls)
        {
        }

        char** data;
        unsigned int length;
        unsigned long* column_lengths;

        column operator[] (unsigned int pos) {
            return column(data[pos], column_lengths[pos]);
        }
    };

    unsigned int size();
    row getRow(unsigned int pos);
private:
    MYSQL *m_mysql;
    MYSQL_RES *m_data;
    unsigned int m_size;
    const char* m_error;
    const std::string& m_originalQuery;

    friend class Myasquo;
};

#endif // MYSQLRESULT_H
