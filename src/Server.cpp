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

    while (Server::getRunning()) {
        /* read the buffer from client */
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
            break;
        }
    }

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
    SDL_Event e;
    while (!done_) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                done_ = true;
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
