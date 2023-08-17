#include "logger.hpp"
#include "util.hpp"
#include "db.hpp"
#include "online.hpp"
#include "room.hpp"
#include "matcher.hpp"
#include "server.hpp"

#define HOST "127.0.0.1"
#define PORT 3306 // 数据库服务的默认端口
#define USER "root"
#define PASS "z1013357334"
#define DBNAME "gobang"

void mysql_test()
{
    MYSQL *mysql = mysql_util::mysql_create(HOST, USER, PASS, DBNAME, PORT);
    // const char* sql = "insert stu values(null, '嬴政', 18, 21, 12, 98);";
    const char *sql = "delete from stu where sn=6;";

    bool ret = mysql_util::mysql_exec(mysql, sql);
    if (ret == false)
    {
        return;
    }
    mysql_util::mysql_destroy(mysql);
}

void json_test()
{
    Json::Value root;
    std::string body;
    root["姓名"] = "小明";
    root["年龄"] = 18;
    root["成绩"].append(98);
    root["成绩"].append(88.5);
    root["成绩"].append(78.5);
    json_util::serialize(root, body);
    DLOG("%s", body.c_str()); // 因为body.c_str()是这个字符串指针，所以必须加上格式化输出
    DLOG("nihao");

    Json::Value val;
    json_util::unserialize(body, val);

    std::cout << "姓名: " << val["姓名"].asString() << std::endl;
    std::cout << "年龄: " << val["年龄"].asInt() << std::endl;
    int sz = val["成绩"].size();

    for (int i = 0; i < sz; i++)
    {
        std::cout << "成绩: " << val["成绩"][i].asFloat() << std::endl;
    }
}

void str_test()
{
    std::string str = "123,234,,,3232323";
    std::vector<std::string> array;
    string_util::split(str, ",", array);

    for (auto e : array)
    {
        DLOG("%s", e.c_str());
    }
}

void file_test()
{
    std::string filename = "./Makefile";
    std::string body;
    file_util::read(filename, body);
    
    std::cout << body << std::endl;
}

void db_test()
{
    user_table ut(HOST, USER, PASS, DBNAME, PORT);
    Json::Value user;
    user["username"] = "xiaoming";
    // user["password"] = "123456";
    bool ret = ut.lose(1);

    /*
    bool ret = ut.select_by_id(4, user);
    std::string body;
    json_util::serialize(user, body);
    std::cout << body << std::endl;
    */
    /*
    bool ret = ut.login(user);
    if (ret == false)
    {
        DLOG("LOGIN FAILED!!");
    }
    */
}

void online_test()
{
    online_manager om;
    wsserver_t::connection_ptr conn;
    uint64_t uid = 2;
    om.enter_game_room(uid, conn);
    if (om.is_in_game_room(uid)) 
    {
        DLOG("IN GAME HALL");
    }
    else 
    {
        DLOG("NOT IN GAME HALL");
    }
    om.exit_game_room(uid);
    if (om.is_in_game_room(uid)) 
    {
        DLOG("IN GAME HALL");
    }
    else 
    {
        DLOG("NOT IN GAME HALL");
    }
}

int main()
{
    // json_test();
    // str_test();
    // file_test();
    // db_test();
    // online_test();
    /*
    user_table ut(HOST, USER, PASS, DBNAME, PORT);
    online_manager om;
    room_manager rm(&ut, &om);
    room_ptr rp = rm.create_room(10, 20);
    matcher mc(&rm, &ut, &om);
    */

   gobang_server gs(HOST, USER, PASS, DBNAME, PORT);
   gs.start(8092);


    return 0;
}