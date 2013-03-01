#include "MySQLResult.h"
#include <mysql/errmsg.h>
#include <stdexcept>

MySQLResult::MySQLResult(MYSQL *mysql, const std::string &originalQuery):
    m_mysql(mysql),m_originalQuery(originalQuery)
{
    m_data = mysql_store_result(mysql);
    if (m_data==NULL && mysql_field_count(mysql) > 0) {
        m_error = mysql_error(mysql);
        m_size = 0;
    }
    else {
        m_error = NULL;
        m_size = mysql_num_rows(m_data);
    }
}

MySQLResult::~MySQLResult()
{
    if (m_data!=NULL)
        mysql_free_result(m_data);
}

const std::string &MySQLResult::originalQuery()
{
    return m_originalQuery;
}

const char *MySQLResult::error()
{
    return m_error;
}

unsigned int MySQLResult::size()
{
    return m_size;
}

MySQLResult::row MySQLResult::getRow(unsigned int pos)
{
    if (pos>=m_size)
        throw std::out_of_range("Index exeeds result array bounds");
    mysql_data_seek(m_data,pos);
    char** result = mysql_fetch_row(m_data);

    if (result == NULL)
        m_error = mysql_error(m_mysql);

    unsigned long *ls = mysql_fetch_lengths(m_data);
    unsigned int l = mysql_num_fields(m_data);
    return row(result,l,ls);
}
