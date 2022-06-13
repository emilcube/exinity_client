#ifndef WEBSOCKET_H__
#define WEBSOCKET_H__

#include "Socket.hpp"

template<typename T> class WebSocketBase :public ISocket
{
protected:
    bool is_connected = false;
    std::unique_ptr<websocket::stream<T>> ws;
    std::string _domen, _port, _path, url;
    net::io_context& ioc;
    beast::flat_buffer _buffer;
    websocket::stream_base::timeout opt;
    std::promise<bool> success_ret;
    std::shared_ptr<ILogger> _logger;

public:

    virtual size_t AvailableBytes() override
    {
        auto ret = beast::get_lowest_layer(*ws.get()).socket().available(_ec);
        if (_ec)
        {
            _err = "Can't count available bytes: " + _ec.message();
            _logger->LogError("WebSocket.AvailableBytes", _err);
            return 0;
        }
        return ret;
    }

    bool Connect(const std::string& address, const std::string& port) override
    {
        std::string prefix, path, uri_port, domen;
        std::tie(prefix, domen, uri_port, path) = ParseURI(address);
        _domen = domen;
        _port = port;
        _path = path;
        url = domen + ":" + port + path;

        return ReConnect();
    }

    virtual bool ReConnect() override
    {
        ioc.restart();
        if (is_connected)
        {
            Close();
        }

        std::promise<bool> prom;
        auto fut = prom.get_future();
        success_ret = std::move(prom);

        tcp::resolver resolver(net::make_strand(ioc));
        _ec.clear();
        auto bnd = beast::bind_front_handler(&WebSocketBase<T>::OnResolve, std::static_pointer_cast<WebSocketBase<T>>(this->shared_from_this()));
        resolver.async_resolve(_domen, _port, std::move(bnd));
        ioc.run();
        if (fut.wait_for(std::chrono::microseconds(0)) != std::future_status::ready)
        {
            _err = "io_context didn't run correctly";
            _logger->LogError("WebSocket.ReConnect", _err);
            return false;
        }

        is_connected = fut.get();
        if (is_connected)
        {
            _logger->LogMessage("WebSocket.connect", "Succesfully connected to " + url);
        }
        return is_connected;
    }

    virtual bool Write(const std::string& data) override
    {
        if (!IsOpen())
        {
            _err = "Trying to write message while not connected";
            _logger->LogError("WebSocket.Write", _err);
            return false;
        }

        ioc.restart();
        std::promise<bool> prom;
        auto fut = prom.get_future();
        success_ret = std::move(prom);

        _ec.clear();
        ws->async_write(boost::asio::buffer(data), beast::bind_front_handler(&WebSocketBase<T>::OnWrite, std::static_pointer_cast<WebSocketBase<T>>(this->shared_from_this())));

        ioc.run();
        if (fut.wait_for(std::chrono::microseconds(0)) != std::future_status::ready)
        {
            _err = "io_context didn't run correctly";
            _logger->LogError("WebSocket.Write", _err);
            return false;
        }
        return fut.get();
    }

    virtual bool Ping(const std::string& data = "") override
    {
        if (!IsOpen())
        {
            _err = "Trying to write message while not connected";
            _logger->LogError("WebSocket.Ping", _err);
            return false;
        }

        ioc.restart();
        std::promise<bool> prom;
        auto fut = prom.get_future();
        success_ret = std::move(prom);

        _ec.clear();
        ws->async_ping(beast::websocket::ping_data(data), beast::bind_front_handler(&WebSocketBase<T>::OnPing, std::static_pointer_cast<WebSocketBase<T>>(this->shared_from_this())));

        ioc.run();
        if (fut.wait_for(std::chrono::microseconds(0)) != std::future_status::ready)
        {
            _err = "io_context didn't run correctly";
            _logger->LogError("WebSocket.Ping", _err);
            return false;
        }
        return fut.get();
    }

    bool Read(std::string& ask) override
    {
        if (!IsOpen())
        {
            _err = "Trying to read message while not connected";
            _logger->LogError("WebSocket.Read", _err);
            ask = "";
            return false;
        }
        ioc.restart();
        std::promise<bool> prom;
        auto fut = prom.get_future();
        success_ret = std::move(prom);

        std::promise<std::string> ret;
        auto futstr = ret.get_future();

        _ec.clear();
        auto bnd = beast::bind_front_handler(&WebSocketBase<T>::OnRead, std::static_pointer_cast<WebSocketBase<T>>(this->shared_from_this()), std::move(ret));
        ws->async_read(_buffer, std::move(bnd));
        ioc.run();

        if (futstr.wait_for(std::chrono::microseconds(0)) != std::future_status::ready)
        {
            _err = "io_context didn't run correctly";
            _logger->LogError("WebSocket.Read", _err);
            ask = "";
            return false;
        }
        ask = futstr.get();
        return fut.get();
    }

    virtual bool IsOpen() override
    {
        return is_connected && ws.get() && ws->is_open() && beast::get_lowest_layer(*ws.get()).socket().is_open();
    }

    virtual bool Close() override
    {
        if (!ws)
        {
            return true;
        }
        ws->close(websocket::close_code::normal, _ec);
        if (_ec)
        {
            _err += "Error while trying to disconnect from " + url + ": " + _ec.message();
            _logger->LogError("WebSocketS.Close", _err);
            return false;
        }
        _ec.clear();
        _logger->LogMessage("WebSocket.Close", "Succesfully closed connection to " + url);
        return true;
    }

    virtual ~WebSocketBase()
    {
        if (this->IsOpen())
        {
            this->Close();
        }
    }

protected:

    WebSocketBase(std::shared_ptr<ILogger> logger, net::io_context& _ioc) : ioc(_ioc), ISocket()
    {
        opt = websocket::stream_base::timeout{ std::chrono::seconds(60), std::chrono::seconds(60), true };
        _logger = logger;
    }

    virtual void OnResolve(beast::error_code ec, tcp::resolver::results_type res)
    {
        if (ec)
        {
            _ec = ec;
            _err = "Error while resolving domen name " + _domen + ": " + ec.message();
            _logger->LogError("WebSocket.OnResolve", _err);
            success_ret.set_value(false);
            ioc.stop();
            return;
        }
        Connect(res);
    }

    virtual void Connect(tcp::resolver::results_type res)
    {
        _ec.clear();
        beast::get_lowest_layer(*ws).expires_after(std::chrono::seconds(60));
        auto bnd = std::bind(&WebSocketBase<T>::OnConnect, std::static_pointer_cast<WebSocketBase<T>>(this->shared_from_this()), std::placeholders::_1);
        beast::get_lowest_layer(*ws.get()).async_connect(res, std::move(bnd));
    }

    virtual void OnConnect(beast::error_code ec)
    {
        if (ec)
        {
            _ec = ec;
            _err = "Error while connecting to " + url + ": " + ec.message();
            _logger->LogError("WebSocketBase.OnConnect", _err);
            success_ret.set_value(false);
            ioc.stop();
            return;
        }
        ec.clear();
        beast::get_lowest_layer(*ws).expires_never();
        auto endp = beast::get_lowest_layer(*ws.get()).socket().remote_endpoint(ec);
        auto endp2 = beast::get_lowest_layer(*ws.get()).socket().local_endpoint(ec);
        if (!ec)
        {
            _logger->LogMessage("WebSocket.OnConnect", "Connected to: " + boost::lexical_cast<std::string>(endp) + ", from: " + boost::lexical_cast<std::string>(endp2), true);
        }
        ws->set_option(opt);
        ws->read_message_max(1ull << 26);
        Handshake(_domen + ":" + _port, _path);
    }

    virtual void OnHandshake(beast::error_code ec)
    {
        if (ec.failed())
        {
            _ec = ec;
            _err = "Error while performing websocket handshake with " + url + ": " + ec.message();
            _logger->LogError("WebSocket.OnHandshake", _err);
            success_ret.set_value(false);
            ioc.stop();
            return;
        }
        ec.clear();
        auto endp = beast::get_lowest_layer(*ws.get()).socket().remote_endpoint(ec);
        auto endp2 = beast::get_lowest_layer(*ws.get()).socket().local_endpoint(ec);
        success_ret.set_value(true);
        ioc.stop();
    }

    virtual void Handshake(std::string header, std::string path)
    {
        auto bnd = beast::bind_front_handler(&WebSocketBase<T>::OnHandshake, std::static_pointer_cast<WebSocketBase<T>>(this->shared_from_this()));
        ws->async_handshake(header, path, std::move(bnd));
    }

    virtual void OnWrite(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
            _ec = ec;
            _err = "Error while writing to " + url + ": " + ec.message();
            _logger->LogError("WebSocketBase.Write", _err);
            success_ret.set_value(false);
            ioc.stop();
            return;
        }
        success_ret.set_value(true);
        ioc.stop();
    }

    virtual void OnPing(boost::system::error_code ec)
    {

        if (ec)
        {
            _ec = ec;
            _err = "Error while writing to " + url + ": " + ec.message();
            _logger->LogError("WebSocketBase.Ping", _err);
            success_ret.set_value(false);
            ioc.stop();
            return;
        }
        success_ret.set_value(true);
        ioc.stop();
    }

    virtual void OnRead(std::promise<std::string> prom, boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
            _ec = ec;
            _err = "Error while reading from " + url + ": " + ec.message();
            _logger->LogError("WebSocketBase.Read", _err);
            prom.set_value("");
            success_ret.set_value(false);
            ioc.stop();
            return;
        }

        prom.set_value(beast::buffers_to_string(_buffer.data()));

        _buffer.consume(_buffer.size());
        success_ret.set_value(true);
        ioc.stop();
    }
};


class WebSocketS : public WebSocketBase<ssl::stream<beast::tcp_stream>>
{
    ssl::context ctx;

public:

    WebSocketS(net::io_context& _ioc, ssl::context _ctx, std::shared_ptr<ILogger> logger) :WebSocketBase(logger, _ioc), ctx(std::move(_ctx))
    {
    }

    virtual ~WebSocketS()
    {
    }

protected:

    virtual void Connect(tcp::resolver::results_type res) override
    {
        this->ws = std::make_unique<websocket::stream<ssl::stream<beast::tcp_stream>>>(ioc, ctx);
        if (!SSL_set_tlsext_host_name(ws->next_layer().native_handle(), _domen.c_str()))
        {
            boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
            _err = "Error while handling ssl connection to " + _domen + ": " + ec.message();
            _logger->LogError("WebSocketS.Connect", _err);
            success_ret.set_value(false);
            return;
        }
        WebSocketBase<ssl::stream<beast::tcp_stream>>::Connect(res);
    }

    virtual void OnSSLHandshake(boost::system::error_code ec)
    {
        if (ec.failed())
        {
            _ec = ec;
            _err = "Error while performing SSL handshake with " + url + ": " + ec.message();
            _logger->LogError("WebSocketS.OnSSLHandshake", _err);
            Close();
            success_ret.set_value(false);
            ioc.stop();
            return;
        }
        auto bnd = beast::bind_front_handler(&WebSocketS::OnHandshake, std::static_pointer_cast<WebSocketS>(this->shared_from_this()));
        ws->async_handshake(_domen + ":" + _port, _path, std::move(bnd));
    }

    virtual void Handshake(std::string header, std::string path) override
    {
        auto bnd = beast::bind_front_handler(&WebSocketS::OnSSLHandshake, std::static_pointer_cast<WebSocketS>(this->shared_from_this()));
        ws->next_layer().async_handshake(ssl::stream_base::client, std::move(bnd));
    }
};


class WebSocket : public WebSocketBase<beast::tcp_stream>
{
public:

    WebSocket(net::io_context& _ioc, std::shared_ptr<ILogger>logger) :WebSocketBase(logger, _ioc)
    {
        //this->ws = std::make_unique<websocket::stream<tcp::socket>>(_ioc);
    }

    virtual ~WebSocket()
    {
    }

protected:

    virtual void Connect(tcp::resolver::results_type res) override
    {
        this->ws = std::make_unique<websocket::stream<beast::tcp_stream>>(ioc);
        WebSocketBase<beast::tcp_stream>::Connect(res);
    }
};

#endif //WEBSOCKET_H__