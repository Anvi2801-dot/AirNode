#pragma once
#include <string>
#include <thread>

class WebSocketServer {
public:
    explicit WebSocketServer(int port);
    void start();
    void broadcast(const std::string& json);
    void stop();
private:
    int          port_;
    std::thread  thread_;
};