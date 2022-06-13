#ifndef WEBSOCKETFACTORY_H__
#define WEBSOCKETFACTORY_H__
#include "WebSocket.hpp"
#include <memory>

class WebSocketFactory
{
private:

    static std::shared_ptr<ILogger> _default_logger, _own_logger;

public:

    static std::string DefaultPort(const std::string& uri)
    {
        std::string prefix, path, uri_port, domen;
        std::tie(prefix, domen, uri_port, path) = ISocket::ParseURI(uri);
        if (prefix == "ws")
        {
            return "80";
        }
        if (prefix == "wss")
        {
            return "443";
        }
        return "";
    }

    static std::shared_ptr<ISocket> GenerateSecure(net::io_context& ioc)
    {
        ssl::context ctx(ssl::context::sslv23);
        if (_default_logger.get())
            return std::make_shared <WebSocketS>(ioc, std::move(ctx), _default_logger);
        else
            return std::make_shared <WebSocketS>(ioc, std::move(ctx), std::make_shared<EmptyLogger>());
    }

    static std::shared_ptr<ISocket> GenerateUnsecure(net::io_context& ioc)
    {
        if (_default_logger.get())
            return std::make_shared <WebSocket>(ioc, _default_logger);
        else
            return std::make_shared <WebSocket>(ioc, std::make_shared<EmptyLogger>());
    }

    static std::shared_ptr<ISocket> GenerateDefault(net::io_context& ioc, const std::string& uri, const std::string& port = "")
    {
        std::string prefix, path, uri_port, domen;
        std::tie(prefix, domen, uri_port, path) = ISocket::ParseURI(uri);

        if (prefix == "ws")
        {
            auto socket = GenerateUnsecure(ioc);
            return socket;
        }
        if (prefix == "wss")
        {
            auto socket = GenerateSecure(ioc);
            return socket;
        }
        else
        {
            if (_own_logger)
                _own_logger->LogError("WebSocketFactory.generate_default", "Error while initializing websocket: unsupported web protocol \"" + prefix + "\"");
            else
                throw socket_except("WebSocketFactory.generate_default: Error while initializing websocket: unsupported web protocol \"" + prefix + "\"");
            return nullptr;
        }
    }

    static void SetDefaultLogger(std::shared_ptr<ILogger> logger)
    {
        _default_logger = logger;
    }

    static void SetOwnLogger(std::shared_ptr<ILogger> logger)
    {
        _own_logger = logger;
    }
};
std::shared_ptr<ILogger> WebSocketFactory::_default_logger = std::shared_ptr<ILogger>(nullptr);
std::shared_ptr<ILogger> WebSocketFactory::_own_logger = std::shared_ptr<ILogger>(nullptr);


#endif //WEBSOCKETFACTORY_H__