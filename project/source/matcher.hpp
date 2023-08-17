#ifndef _M_MATCHER__
#define _M_MATCHER__

#include "util.hpp"
#include <list>
#include <condition_variable>
#include "room.hpp"
#include "db.hpp"
#include "online.hpp"

template<class T>
class match_queue
{
private:
    /* 为什么要使用链表而不是queue？ 因为要考虑用户匹配中取消匹配，即删除中间数据，queue没有该功能*/
    std::list<T> _list;
    std::mutex _mutex;
    std::condition_variable _cond; // 条件变量 这个条件变量主要为了阻塞消费者
public:
    // 获取元素个数
    int size()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _list.size();
    }

    // 判断是否为空
    bool empty()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _list.empty();
    }

    // 阻塞线程
    void wait()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock);
    }

    // 入队数据，并唤醒线程
    void push(const T& data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _list.push_back(data);
        _cond.notify_all();
    }

    // 出队数据
    bool pop(T& data) // 这块传入的是变量，不是实数！！！
    {
        std::unique_lock<std::mutex> lock(_mutex);
        // 有可能已经在其它线程中将data出队列了
        if (_list.empty() == true)
        {
            return false;
        }
        data = _list.front(); // data是引用，此处先让出队列但保留数据，后续要使用或者删除再执行对应操作
        _list.pop_front();

        return true;
    }

    // 移除指定的数据
    void remove(T& data)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _list.remove(data);

    }
};

class matcher
{
private:
    // 包括三个匹配队列和三个匹配队列处理线程
    match_queue<uint64_t> _q_normal;
    match_queue<uint64_t> _q_high;
    match_queue<uint64_t> _q_super;
    std::thread _th_normal;
    std::thread _th_high;
    std::thread _th_super;

    room_manager* _rm;
    user_table* _ut;
    online_manager* _om;
private:
    void handle_match(match_queue<uint64_t>& mq)
    {
        while (1)
        {
            // 1. 判断队列人数是否大于2，<2则阻塞等待
            while (mq.size() < 2)
            {
                mq.wait();
            }

            // 2. 走下来代表人数够了，出队两个玩家
            uint64_t uid1, uid2;
            bool ret = mq.pop(uid1); // 如果出队列成功，那么uid1肯定就获得了用户uid
            if (ret == false) // 出队列失败只可能是队列空了，否则不会返回false
            {
                continue;
            }
            ret = mq.pop(uid2);
            if (ret == false)
            {
                this->add(uid1);
                continue;
            }

            // 3. 校验两个玩家是否在线，如果有人掉线，则要吧另一个人重新添加入队列
            wsserver_t::connection_ptr conn1 = _om->get_conn_from_hall(uid1);
            if (conn1.get() == nullptr)
            {
                this->add(uid2);
                continue;
            }
            wsserver_t::connection_ptr conn2 = _om->get_conn_from_hall(uid2);
            if (conn2.get() == nullptr)
            {
                this->add(uid1);
                continue;
            }

            // 4. 为两个玩家创建房间，并将玩家加入房间中
            room_ptr rp = _rm->create_room(uid1, uid2);
            if (rp.get() == nullptr)
            {
                this->add(uid1);
                this->add(uid2);
                continue;
            }

            // 5. 对两个玩家进行响应
            Json::Value resp;
            resp["optype"] = "match_success";
            resp["result"] = true;
            std::string body;
            json_util::serialize(resp, body);
            conn1->send(body);
            conn2->send(body);
        }
    }

    void th_normal_entry() { return handle_match(_q_normal); }

    void th_high_entry() { return handle_match(_q_high); }

    void th_super_entry()  { return handle_match(_q_super); }

public:
    matcher(room_manager* rm, user_table* ut, online_manager* om):
        _rm(rm), _ut(ut), _om(om), 
        _th_normal(std::thread(&matcher::th_normal_entry, this)),
        _th_high(std::thread(&matcher::th_high_entry, this)),
        _th_super(std::thread(&matcher::th_super_entry, this))
    {
        DLOG("游戏匹配模块初始化完毕......");
    }

    bool add(uint64_t uid)
    {
        // 根据玩家的分数来判定玩家的档次，添加到不同的匹配队列
        // 1. 根据用户ID，获取玩家信息
        Json::Value user;
        bool ret = _ut->select_by_id(uid, user);
        if (ret == false)
        {
            DLOG("获取玩家:%d 信息失败！！", uid);
            return false;
        }

        int score = user["score"].asInt();

        // 2. 添加到指定的队列中
        if (score < 2000)
        {
            _q_normal.push(uid);
        }
        else if (score >= 2000 && score < 3000)
        {
            _q_high.push(uid);
        }
        else
        {
            _q_super.push(uid);
        }

        return true;
    }

    bool del(uint64_t uid)
    {
        Json::Value user;
        bool ret = _ut->select_by_id(uid, user);
        if (ret == false)
        {
            DLOG("获取玩家:%d 信息失败！！", uid);
            return false;
        }

        int score = user["score"].asInt();
        
        // 从指定队列中删除
        if (score < 2000)
        {
            _q_normal.remove(uid);
        }
        else if (score >= 2000 && score < 3000)
        {
            _q_high.remove(uid);
        }
        else
        {
            _q_super.remove(uid);
        }

        return true;
    }

};
#endif // !_M_MATCHER__