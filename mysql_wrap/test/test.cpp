#include "mysql_wrap.h"

#include <stdio.h>
int main()
{
    char sql_buf[2048];
    MysqlDB theDB;
    if(theDB.Connect("localhost", "booljin", "123456", "cpp_test") != 0)
    {
        printf("dbConnect fail %s", theDB.GetErr());
        return 0;
    }
    printf("connect success\n");
    //char *username, *data1, *data2;
    //unsigned long len, len1, len2;
    //theDB.Escape("test123", strlen("test123"), username, len);
    //char *tdata1 = "0123~!@#$%^&*(),. ;'\"abc\\/";
    //theDB.Escape(tdata1, strlen(tdata1) + 1, data1, len1);
    ////insert
    //int sql_len = snprintf(sql_buf, sizeof(sql_buf),
    //    "insert into t_1(user_name, user_data_0) value('%s', '%s');",
    //    username, data1);
    //printf("tdata = %s\n", tdata1);
    //printf("sql = %s\n", sql_buf);
    //int rows;
    //if(theDB.Query(sql_buf, sql_len, NULL, &rows) != 0)
    //{
    //    printf("insert err %s\n", theDB.GetErr());
    //    return 0;
    //}
    //delete[] username;
    //delete[] data1;
    
    // set: update t_1 set xxx = 'xxx', yyy = 'yyy' where user_name = BINARY'name'
    
    // get: select user_data_0, user_data_1 from t_1 where user_name = BINARY'name'
    
    MysqlResult result;
    //char *select_sql = "select user_data_0, user_data_1 from t_1 where user_name = BINARY'test123';";
    //char *select_sql = "select user_data_0, user_data_1 from t_1;";
    char *select_sql = "select * from t_1 where user_name = BINARY'test1223';";
    
    
    if(theDB.Query(select_sql, strlen(select_sql), &result) != 0)
    {
        printf("select err %s\n", theDB.GetErr());
        return 0;
    }
    printf("row = %d\n", result.RowNum());
    int row_num = result.RowNum();
    printf("fileds : ");
    char field_name[30];
    enum_field_types field_type;
    for(int i = 0; i < result.FieldNum(); ++i)
    {
        result.FieldInfo(i, field_name, 30, &field_type);
        printf("%s:%d\t", field_name, field_type);
    }
    printf("\n");
    while(row_num-- > 0)
    {
        char** rcd = result.FetchNext();
        unsigned long *lengthArray;
        int lengthNum;
        result.FieldLengthArray(lengthArray, lengthNum);
        printf("result.FieldLengthArray = %d\n", lengthNum);
        for(int i = 0; i < lengthNum; ++i)
        {
            char *vol = rcd[i];
            if(vol != NULL)
                printf("    %d: %s----%d\n", i, vol, lengthArray[i]);
            else
                printf("    %d: NULL\n", i);
        }
    }
    return 0;
}