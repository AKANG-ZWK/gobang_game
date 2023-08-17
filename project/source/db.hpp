#ifndef __M_DB_H__
#define __M_DB_H__
#include "util.hpp"
#include <mutex>
#include <cassert>

class user_table
{
private:
    MYSQL *_mysql;     // mysql操作句柄
    std::mutex _mutex; // 互斥锁保护数据库的访问操作
public:
    user_table(const std::string &host,
               const std::string &username,
               const std::string &password,
               const std::string &dbname,
               uint16_t port = 3306)
    {
        _mysql = mysql_util::mysql_create(host, username, password, dbname, port);
        assert(_mysql != NULL);
    }
    ~user_table()
    {
        mysql_util::mysql_destroy(_mysql);
        _mysql = NULL;
    }

    // 注册时新增用户
    bool insert(Json::Value &user)
    {
#define INSERT_USER "insert user values(null, '%s', password('%s'), 1000, 0, 0);"
        // sprintf(void* buf, char* foemat, ...)
        if (user["password"].isNull() || user["username"].isNull())
        {
            DLOG("INPUT PASSWORD OR USERNAME");
            return false;
        }
        char sql[4096] = {0};
        // 将需要插入的语句和变量格式化成一个字符串存在sql里面
        sprintf(sql, INSERT_USER, user["username"].asCString(), user["password"].asCString());
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("insert user info failed!!\n");
            return false;
        }
        return true;
    }

    // 登陆时验证用户账号密码，并返回详细信息
    bool login(Json::Value &user)
    {
// 以用户名和密码共同作为查询过滤条件，查询到数据则表示用户名密码一致，没有信息则用户名密码错误
#define LOGIN_USER "select id, score, total_count, win_count from user where username='%s' and password=password('%s');"
        char sql[4096] = {0};
        sprintf(sql, LOGIN_USER, user["username"].asCString(), user["password"].asCString());

        MYSQL_RES *res = NULL;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("user login failed!!\n");
                return false;
            }
            // 要么有数据，要么没有数据，就算有数据也只能有一条数据
            res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                DLOG("have no login user info!!");
                return false;
            }
        }
        int row_num = mysql_num_rows(res);
        if (row_num != 1)
        {
            DLOG("the user information queried is not unique!!");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = (Json::UInt64)std::stol(row[0]);
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);

        return true;
    }

    // 通过用户名获取用户信息
    bool select_by_name(const std::string &name, Json::Value &user)
    {
#define USER_BY_NAME "select id, score, total_count, win_count from user where username='%s';"
        char sql[4096] = {0};
        sprintf(sql, USER_BY_NAME, name.c_str());
        MYSQL_RES *res = NULL;
        { // 使用局部作用域控制锁，lock对象用来管理锁，除了作用域lock被释放，随之解锁
          // 此处加锁是为了保证查询结果和保存结果这两步的原子性，以防多线程操作时结果还未保存到本地就被修改
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("get user by name failed!!\n");
                return false;
            }
            // 要么有数据，要么没有数据，有数据也只能有一条数据
            res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                DLOG("have no user info!!");
                return false;
            }
        }
        int row_num = mysql_num_rows(res);
        if (row_num != 1)
        {
            DLOG("the user information queried is not unique!!");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        user["id"] = (Json::UInt64)std::stol(row[0]);
        user["username"] = name;
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }

    // 通过id获取用户信息
    bool select_by_id(uint64_t id, Json::Value &user)
    {
        #define USER_BY_ID "select username, score, total_count, win_count from user where id=%d;"
        char sql[4096] = {0};
        sprintf(sql, USER_BY_ID, id);
        MYSQL_RES *res = NULL;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            bool ret = mysql_util::mysql_exec(_mysql, sql);
            if (ret == false)
            {
                DLOG("get user by id failed!!\n");
                return false;
            }
            // 要么有数据，要么没有数据，有数据也只能有一条数据
            res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                DLOG("have no user info!!");
                return false;
            }
        }
        int row_num = mysql_num_rows(res);
        if (row_num != 1)
        {
            DLOG("the user information queried is not unique!!");
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res); 
        user["id"] = (Json::UInt64)id;
        user["username"] = row[0];
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);

        return true;
    }

    // 胜利时天梯分数增加，战斗场次增加，胜利场次增加
    bool win(uint64_t id)
    {
#define USER_WIN "update user set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=%d;"
        char sql[4096];
        sprintf(sql, USER_WIN, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("update win user info failed!!\n");
            return false;
        }
        return true;
    }

    // 失败时天梯分数减小，战斗场次增加，其它不变
    bool lose(uint64_t id)
    {
#define USER_LOSE "update user set score=score-30, total_count=total_count+1 where id=%d;"
        char sql[4096];
        sprintf(sql, USER_LOSE, id);
        bool ret = mysql_util::mysql_exec(_mysql, sql);
        if (ret == false)
        {
            DLOG("update win user info failed!!\n");
            return false;
        }
        return true;
    }
};
#endif