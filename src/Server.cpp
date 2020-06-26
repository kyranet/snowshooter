#include "Server.h"

void Server::ServerClient::initialize() {
    /* get the clients IP and port number */
    remoteIP_ = SDLNet_TCP_GetPeerAddress(socket_);
    if (!remoteIP_) {
        printf("SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());
        return;
    }

    /* print out the clients IP and port number */
    uint32_t ipAddress;
    ipAddress = SDL_SwapBE32(remoteIP_->host);
    printf("Accepted a connection from %d.%d.%d.%d port %hu\n", ipAddress >> 24u,
           (ipAddress >> 16u) & 0xFFu, (ipAddress >> 8u) & 0xFFu, ipAddress & 0xFFu,
           remoteIP_->port);

    status_ = ServerClientStatus::RUNNING;
    Server::pushEvent({EventDataType::CONNECT, &ipAddress});

    while (Server::getRunning()) {
        // Read the buffer from the client
        char message[1024];
        int len = SDLNet_TCP_Recv(socket_, message, 1024);
        if (!len) {
            printf("SDLNet_TCP_Recv: %s\n", SDLNet_GetError());
            break;
        }

        // Print the received message
        printf("Received: %.*s\n", len, message);
        if (message[0] == 'q') {
            printf("Disconnecting on a q\n");
            Server::pushEvent({EventDataType::DISCONNECT, this});
            break;
        }
    }

    status_ = ServerClientStatus::CLOSED;
    SDLNet_TCP_Close(socket_);
}

Server::ServerClient::ServerClient(TCPsocket socket) : socket_(socket) {}

bool Server::ServerClient::isPending() { return status_ == ServerClientStatus::PENDING; }

bool Server::ServerClient::isRunning() { return status_ == ServerClientStatus::RUNNING; }

bool Server::ServerClient::isClosed() { return status_ == ServerClientStatus::CLOSED; }

void Server::ServerClient::create(TCPsocket socket) {
    auto *client = new ServerClient(socket);
    client->initialize();
}

Server *Server::instance_ = nullptr;
SDL_mutex *Server::mutex_ = nullptr;
std::queue<Server::event_data_t> Server::events_{};

Server::Server() {
    if (SDL_Init(0) == -1) {
        printf("SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }
    if (SDLNet_Init() == -1) {
        printf("SDLNet_Init: %s\n", SDLNet_GetError());
        exit(2);
    }

    printf("Starting server...\n");
    if (SDLNet_ResolveHost(&ip_, nullptr, 9999) == -1) {
        printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        exit(1);
    }

    server_ = SDLNet_TCP_Open(&ip_);
    if (!server_) {
        printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
        exit(2);
    }
}

Server::~Server() {
    done_ = true;
    SDL_AtomicSet(&running_, 0);
}

void Server::run() {
    while (!done_) {
        // Handle SDL events on queue
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                done_ = true;
            }
        }

        // Handle game events on queue
        event_data_t ed;
        while (clientPollEvent(&ed) != 0) {
            switch (ed.type) {
                case Server::EventDataType::DISCONNECT:
                    printf("Client Disconnected.");
                    break;
                case EventDataType::CONNECT:
                    printf("Client Connected %d!", *reinterpret_cast<uint32_t *>(ed.data));
                    break;
            }
        }

        // Free memory
        for (size_t i = clients_.size(); i >= 0; --i) {
            auto *client = clients_[i];
            if (client->isClosed()) {
                clients_.erase(clients_.begin() + static_cast<long>(i));
                delete client;
            }
        }

        // Try to accept a connection
        auto *client = SDLNet_TCP_Accept(server_);

        // No connection accepted
        if (!client) {
            SDL_Delay(1000 / 30);
            continue;
        }

        auto *thread = SDL_CreateThread(reinterpret_cast<SDL_ThreadFunction>(ServerClient::create), "", client);
        SDL_DetachThread(thread);
    }

    SDLNet_Quit();
    SDL_Quit();
}

Server *Server::getInstance() {
    if (instance_ == nullptr) {
        instance_ = new Server();
        SDL_AtomicSet(&running_, 1);
    }
    return instance_;
}

int Server::getRunning() {
    return SDL_AtomicGet(&running_);
}

SDL_mutex *Server::getMutex() {
    if (mutex_ == nullptr) mutex_ = SDL_CreateMutex();
    return mutex_;
}

void Server::pushEvent(const Server::event_data_t &event) {
    if (SDL_LockMutex(getMutex())) {
        events_.push(event);
        SDL_UnlockMutex(mutex_);
    } else {
        fprintf(stderr, "Couldn't lock mutex: %s", SDL_GetError());
    }
}

int Server::clientPollEvent(Server::event_data_t *event) {
    if (SDL_LockMutex(getMutex())) {
        if (events_.empty()) return 0;
        *event = events_.front();
        events_.pop();

        SDL_UnlockMutex(mutex_);
        return 1;
    }

    fprintf(stderr, "Couldn't lock mutex: %s", SDL_GetError());
    return 0;
}
