#pragma once

#include "SDL.h"
#include "SDL_net.h"
#include <vector>

class Server {
    enum class ServerClientStatus {
        PENDING,
        RUNNING,
        CLOSED
    };

    class ServerClient {
        ServerClientStatus status_ = ServerClientStatus::PENDING;
        IPaddress *remoteIP_ = nullptr;
        TCPsocket socket_;

        void initialize();

        explicit ServerClient(TCPsocket socket);

    public:
        bool isPending();

        bool isRunning();

        bool isClosed();

        void static create(TCPsocket socket);

        ~ServerClient() = default;
    };

    static Server *instance_;
    static SDL_atomic_t running_;
    std::vector<ServerClient *> clients_{};
    TCPsocket server_ = nullptr;
    IPaddress ip_{};
    bool done_ = false;

    Server();

public:
    ~Server();

    void run();

    static Server *getInstance() {
        if (instance_ == nullptr) {
            instance_ = new Server();
            SDL_AtomicSet(&running_, 1);
        }
        return instance_;
    }

    static int getRunning() {
        return SDL_AtomicGet(&running_);
    }
};
