#ifndef __MYSQL_WRAP_H__
#define __MYSQL_WRAP_H__

#include <mysql.h>
#include <mysqld_error.h>


class MysqlResult
{
    friend class MysqlDB;
public:
    MysqlResult();
    ~MysqlResult();
    
    // 查询列数
    int FieldNum();
    // 通过id查询列信息
    int FieldInfo(int idx, char *field_name, int name_len,
        enum_field_types *field_type);
    // 通过名字查找idx
    int FieldIdx(const char *field_name);
    // 第idx列长度
    int FieldLength(int idx, unsigned long &length);
    // 所有列长度的数组
    int FieldLengthArray(unsigned long *&lengthArray, int &num);
    // effect raw
    int RowNum();
    char **FetchNext();
    char **Fetch(unsigned long long offset);
    void Free();
protected:
    MYSQL_RES *m_pResult;
    char m_errMsg[256];
};

class MysqlDB
{
public:
    MysqlDB();
    ~MysqlDB();
    
    int Connect(const char *host, const char *user, const char *password,
        const char *dbname, int port = 3306, bool keepalive = false,
        const char *sock = NULL);
    int Query(const char *sql, unsigned long sqlLen, MysqlResult *poRst = NULL,
        int *affectRow = NULL);
    void Escape(const char *data, unsigned long dataLen, char *& newBuff, unsigned long &escapeLen);
    void Close();
    inline bool IsConnected()
    {
        return m_connected;
    }
    const char *GetErr();
    // timeout单位s,默认是<0表示不限制
    // 必须在connect之前设置
    inline void SetTimeOut(int timeout)
    {
        m_timeout = timeout;
    }
protected:
    MYSQL m_sqlHandle;
    char m_errMsg[256];
    bool m_keepalive;
    bool m_connected;
    int m_timeout;
};

#endif // __MYSQL_WRAP_H__