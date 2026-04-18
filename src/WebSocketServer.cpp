#include "../include/WebSocketServer.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <mutex>
#include <vector>
#include <memory>

namespace beast     = boost::beast;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
using tcp           = net::ip::tcp;

// ── Internal state ────────────────────────────────────────────────────────────
static net::io_context                                         ioc;
static std::vector<std::shared_ptr<websocket::stream<tcp::socket>>> clients;
static std::mutex                                              clientsMu;

// Handles one connected browser tab
static void handleSession(tcp::socket socket) {
    try {
        auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
        ws->accept();
        {
            std::lock_guard<std::mutex> lk(clientsMu);
            clients.push_back(ws);
        }
        beast::flat_buffer buf;
        // Keep alive — read loop (we don't expect messages from the browser)
        while (true) {
            ws->read(buf);
            buf.consume(buf.size());
        }
    } catch (...) {}
}

// ── Public API ────────────────────────────────────────────────────────────────
WebSocketServer::WebSocketServer(int port) : port_(port) {}

void WebSocketServer::start() {
    thread_ = std::thread([this]() {
        try {
            tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), port_));
            std::cout << "[WS] listening on ws://localhost:" << port_ << std::endl;
            while (true) {
                tcp::socket socket(ioc);
                acceptor.accept(socket);
                std::thread(handleSession, std::move(socket)).detach();
            }
        } catch (const std::exception& e) {
            std::cerr << "[WS] " << e.what() << std::endl;
        }
    });
    thread_.detach();
}

void WebSocketServer::broadcast(const std::string& json) {
    std::lock_guard<std::mutex> lk(clientsMu);
    // Iterate backwards so we can erase dead clients cleanly
    for (int i = (int)clients.size() - 1; i >= 0; i--) {
        try {
            clients[i]->write(net::buffer(json));
        } catch (...) {
            clients.erase(clients.begin() + i);
        }
    }
}

void WebSocketServer::stop() {
    ioc.stop();
}