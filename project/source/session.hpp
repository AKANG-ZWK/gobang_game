#ifndef __M_SS_H__
#define __M_SS_H__

#include "util.hpp"
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <unordered_map>

typedef enum
{
    UNLOGIN,
    LOGIN
} ss_statu;

class session
{
private:
    uint64_t _ssid;            // 标识符
    uint64_t _uid;             // session对应的用户ID
    ss_statu _statu;           // 用户状态： 登录和未登录
    wsserver_t::timer_ptr _tp; // session关联的定时器
public:
    session(uint64_t ssid) : _ssid(ssid) { DLOG("SESSION %p 被创建！！", this); }

    ~session() { DLOG("SESSION %p 被释放！！", this); }

    void set_user(uint64_t uid) { _uid = uid; }

    void set_statu(ss_statu statu) { _statu = statu; }

    uint64_t ssid() { return _ssid; }
 
    uint64_t get_user() { return _uid; }

    bool is_login() { return _statu == LOGIN; }

    void set_timer(const wsserver_t::timer_ptr &tp) { _tp = tp; }

    wsserver_t::timer_ptr& get_timer() { return _tp; }
};


#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1
using session_ptr = std::shared_ptr<session>;

class session_manager
{
private:
    uint64_t _next_ssid;
    std::mutex _mutex;
    std::unordered_map<uint64_t, session_ptr> _session; // 存储所有的session会话
    wsserver_t* _server;
public:
    session_manager(wsserver_t* srv): _next_ssid(1), _server(srv)
    {
        DLOG("session管理器初始化完毕！");
    }
    ~session_manager() { DLOG("session管理器即将销毁！"); }

    session_ptr create_session(uint64_t uid, ss_statu statu)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        session_ptr ssp(new session(_next_ssid));
        ssp->set_statu(statu);
        ssp->set_user(uid);
        _session.insert(std::make_pair(_next_ssid, ssp));
        _next_ssid++;

        return ssp;
    }

    session_ptr get_session_by_ssid(uint64_t ssid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _session.find(ssid);
        if (it == _session.end())
        {
            return session_ptr();
        }

        return it->second;
    }

    void append_session(const session_ptr &ssp)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _session.insert(std::make_pair(ssp->ssid(), ssp));
    }

    void remove_session(uint64_t ssid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _session.erase(ssid);
    }

    void set_session_expire_time(uint64_t ssid, int ms)
    {
        // 依赖于websocketpp的定时器来完成session生命周期的管理。
        //  登录之后，创建session，session需要在指定时间无通信后删除
        //  但是进入游戏大厅，或者游戏房间，这个session就应该永久存在，因为websocket是长连接
        //  等到退出游戏大厅，或者游戏房间，这个session应该被重新设置为临时，在长时间无通信后被删除
        session_ptr ssp = get_session_by_ssid(ssid); // 先通过ssid查一下_session里面有没有session对象，有的话就返回
        if (ssp.get() == nullptr)
        {
            return;
        }
        wsserver_t::timer_ptr tp = ssp->get_timer(); // 然后通过session获得定时器对象，没有的话返回空指针
        if (tp.get() == nullptr && ms == SESSION_FOREVER)
        {
            // 1. 在session永久存在的情况下，设置永久存在
            return;
        }
        else if (tp.get() == nullptr && ms != SESSION_FOREVER)
        {
            // 当没有设置定时任务时，也就是刚登陆游戏，需要设置定时任务，以保证长时间不响应删除会话
            // 此时会绑定删除session的函数，因此后面使用cancel时，会触发删除函数，导致立马session立马被删除
            // 2. 在session永久存在的情况下，设置指定时间之后被删除的定时任务
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,
                    std::bind(&session_manager::remove_session, this, ssid));
            ssp->set_timer(tmp_tp);
        }
        else if (tp.get() != nullptr && ms == SESSION_FOREVER)
        {
            // 走到这肯定已经设置的定时任务，因此肯定绑定了删除session的函数remove_session,所以后面使用cancel会导致session被删除
            // 3. 在session设置了定时删除的情况下，将session设置为永久存在(进入大厅或房间时)
            tp->cancel(); // 删除定时任务---stready_timer删除定时任务会导致任务直接被执行
           
            // 为什么cancel不是立即删除定时任务？？？ 由服务器管理
            // 因为这个取消定时任务并不是立即取消的，因此重新给session管理器中添加一个session信息的时候
            // 需要使用定时器，而不是立即添加，不然可能会出现还没删除session就重新添加的情况
            ssp->set_timer(wsserver_t::timer_ptr()); // 将session关联的定时器设置为空
            _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp));
        }
        else if (tp.get() != nullptr && ms != SESSION_FOREVER)
        {
            // 4. 在session设置了定时删除的情况下，相当于将session重置删除时间。
            tp->cancel(); // 因为这个取消定时任务并不是立即取消的
            ssp->set_timer(wsserver_t::timer_ptr()); // 将原来的session中的定时对象设置为空的，设置空的timer
            _server->set_timer(0, std::bind(&session_manager::append_session, this, ssp)); // 然后由服务器重新设置定时任务

            // 重新给session添加定时销毁任务
            wsserver_t::timer_ptr tmp_tp = _server->set_timer(ms,
                std::bind(&session_manager::remove_session, this, ssp->ssid()));
            // 重新设置session关联的定时器
            ssp->set_timer(tmp_tp);
        }
    }
};

#endif // !__M_SS_H__