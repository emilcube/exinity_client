#ifndef SOCKET_H__
#define SOCKET_H__

#include "Logger/Logger.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <string>
#include <algorithm>


namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;


class socket_except : public std::exception
{
private:

    std::string _message;

public:

    socket_except(char* message)
    {
        _message = message;
    }

    socket_except(std::string message)
    {
        _message = message;
    }

    ~socket_except()
    {
        _message.clear();
    }

    virtual const char* what() const throw() override
    {
        return _message.c_str();
    }

};


class ISocket : public std::enable_shared_from_this<ISocket>
{
protected:

    mutable std::string _err;
    boost::system::error_code _ec;

    ISocket() {}

    virtual ~ISocket() {}

public:

    //prefix,URL,port,path
    static std::tuple < std::string, std::string, std::string, std::string > ParseURI(const std::string& address)
    {
        std::string prefix, adr;
        size_t subaddr;
        if ((subaddr = address.find("://")) < address.length())
        {
            prefix = address.substr(0, subaddr);
            adr = address.substr(subaddr + 3);
        }
        else
        {
            prefix = "ws";
            adr = address;
        }
        subaddr = std::min(adr.find('/'), adr.length());
        auto path = adr.substr(subaddr);
        if (path.length() < 1)
            path.push_back('/');
        adr = adr.substr(0, subaddr);

        std::string port;
        if ((subaddr = adr.find(":")) < adr.length())
        {
            port = adr.substr(subaddr + 1);
            adr = adr.substr(0, subaddr);
        }
        else
        {
            if (prefix == "wss")
                port = "443";
            else if (prefix == "ws")
                port = "80";
            else
                port = "";
        }

        return{ prefix, adr, port, path };
    }

    std::string PeekError() const
    {
        return _err;
    }

    std::string ConsumeError() const
    {
        return std::move(_err);
    }

    const boost::system::error_code GetErrorCode() const
    {
        return _ec;
    }

    virtual bool Connect(const std::string& uri, const std::string& port) = 0;
    virtual bool ReConnect() = 0;
    virtual bool Close() = 0;

    virtual bool Write(const std::string& data) = 0;
    virtual bool Ping(const std::string& data) = 0;
    virtual bool Read(std::string& ret) = 0;
    /** Returns `true` if the websocket is open. Can get stale until read or write function is called.
    */
    virtual bool IsOpen() = 0;
    virtual size_t AvailableBytes() = 0;
};

#endif //SOCKET_H__

