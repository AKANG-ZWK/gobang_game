#ifndef __M_UTIL_H__
#define __M_UTIL_H__
#include "logger.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <memory> // 智能指针头文件
#include <vector>
#include <cstdint>
#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>


#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
typedef websocketpp::server<websocketpp::config::asio> wsserver_t;

class mysql_util
{
public:
    static MYSQL *mysql_create(const std::string &host,
                               const std::string &username,
                               const std::string &password,
                               const std::string &dbname,
                               uint16_t port = 3306)
    {
        // 1. 初始化
        MYSQL *mysql = mysql_init(NULL); // 传入空指针，自动返回一个MySQL对象
        if (mysql == NULL)
        {
            ELOG("mysql init failed!");
            return NULL;
        }

        // 2. 连接服务器
        if (mysql_real_connect(mysql,
                               host.c_str(),
                               username.c_str(),
                               password.c_str(),
                               dbname.c_str(), port, NULL, 0) == NULL)
        {
            ELOG("connect mysql server failed : %s", mysql_error(mysql));
            mysql_close(mysql);
            return NULL;
        }

        // 3. 设置客户端字符集
        if (mysql_set_character_set(mysql, "utf8") != 0)
        {
            ELOG("set client character failed : %s", mysql_error(mysql));
            mysql_close(mysql);
            return NULL;
        }

        return mysql;
    }

    // 执行查询MySQL语句
    static bool mysql_exec(MYSQL *mysql, const std::string &sql)
    {
        int ret = mysql_query(mysql, sql.c_str());
        if (ret != 0)
        {
            ELOG("%s\n", sql.c_str());
            ELOG("mysql query failed : %s\n", mysql_error(mysql));
            return false;
        }
        return true;
    }

    static void mysql_destroy(MYSQL *mysql)
    {
        if (mysql != NULL)
        {
            mysql_close(mysql);
        }
        return;
    }
};

class json_util
{
public:
    static bool serialize(const Json::Value &root, std::string &str)
    {
        Json::StreamWriterBuilder swb;
        std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
        std::stringstream ss;
        int ret = sw->write(root, &ss);
        if (ret != 0)
        {
            ELOG("json serialize failed!!");
            return false;
        }
        str = ss.str();

        return true;
    }
    static bool unserialize(const std::string &str, Json::Value &root)
    {
        Json::CharReaderBuilder crb;
        std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
        std::string err;
        bool ret = cr->parse(str.c_str(), str.c_str() + str.size(), &root, &err);
        if (ret == false)
        {
            ELOG("json unserialize failed: %s", err.c_str());
            return false;
        }
        return true;
    }
};

class string_util
{
public:
    static int split(const std::string &src, const std::string &sep, std::vector<std::string> &res)
    {
        // src:需要分割的源字符串 sep: 分隔符 res: 用于存储分割好的字符串
        // 55,33232,2323,,23
        size_t pos, idx = 0;

        while (idx < src.size())
        {
            pos = src.find(sep, idx);
            if (pos == std::string::npos)
            {
                // 没有找到
                res.push_back(src.substr(idx)); // 结尾没有分隔符也需要插入
                break;
            }

            if (pos == idx) // 避免有两个连续的分隔符从而传入了空字符串
            {
                idx += sep.size();
                continue;
            }
            res.push_back(src.substr(idx, pos - idx));
            idx = pos + sep.size(); // 更新索引
        }

        return res.size(); // 返回分割之后的字符串个数
    }
};

class file_util
{
public:
    static bool read(const std::string &filename, std::string &body)
    {
        // 打开文件
        std::ifstream ifs(filename, std::ios::binary); // 以二进制形式打开数据，因为我们并不确定数据是什么类型，所以要用最基本的形式打开
        if (ifs.is_open() == false)
        {
            ELOG("%s file open filed!!", filename.c_str());
            return false;
        }
        // 获取文件大小
        size_t fsize = 0;
        ifs.seekg(0, std::ios::end); // 将读取位置设置为end位置偏移量为0位置处
        fsize = ifs.tellg(); // 返回当前读取指针的位置，此处也即文件大小
        ifs.seekg(0, std::ios::beg); //  将读取位置设置为beg位置，也就是开始位置
        body.resize(fsize); // 设置读取对象的大小等于文件中数据的大小
        // 将文件数据读取出来
        ifs.read(&body[0], fsize);
        if (ifs.good() == false) // good()：如果流是好的返回true否者返回false
        {
            ELOG("read %s file content failed!", filename.c_str());
            ifs.close();
            return false;
        }
        // 关闭文件
        ifs.close();
        return true;
    }
};

#endif