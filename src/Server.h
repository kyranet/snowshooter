#pragma once

#include "SDL.h"
#include "SDL_net.h"
#include <vector>
#include <queue>

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

    enum class EventDataType {
        CONNECT,
        DISCONNECT
    };

    typedef struct {
        EventDataType type;
        void *data;
    } event_data_t;

    static Server *instance_;
    static SDL_atomic_t running_;
    static SDL_mutex *mutex_;
    static std::queue<event_data_t> events_;
    std::vector<ServerClient *> clients_{};
    TCPsocket server_ = nullptr;
    IPaddress ip_{};
    bool done_ = false;

    /**
     *  \brief Polls for currently pending events.
     *
     *  \return 1 if there are any pending events, or 0 if there are none available.
     *
     *  \param event If not nullptr, the next event is removed from the queue and
     *               stored in that area.
     */
    static int clientPollEvent(event_data_t *event);

    Server();

public:
    ~Server();

    void run();

    static Server *getInstance();

    static int getRunning();

    static SDL_mutex *getMutex();

    static void pushEvent(const event_data_t &event);
};
