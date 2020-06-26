#include "Client.h"

Client *Client::instance_ = nullptr;

Client::Client() {
    if (SDL_Init(0) == -1) {
        printf("SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    if (SDLNet_Init() == -1) {
        printf("SDLNet_Init: %s\n", SDLNet_GetError());
        exit(2);
    }

    printf("Starting client...\n");
    if (SDLNet_ResolveHost(&ip_, "localhost", 9999) == -1) {
        printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        exit(1);
    }

    socket_ = SDLNet_TCP_Open(&ip_);
    if (!socket_) {
        printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
        exit(2);
    }
}

Client::~Client() {
    if (socket_ != nullptr) SDLNet_TCP_Close(socket_);

    SDLNet_Quit();
    SDL_Quit();
}

void Client::initializeThread() {
    auto instance = getInstance();
    instance->resume();

    while (instance->isRunning()) {
        char message[1024];
        if (SDLNet_TCP_Recv(instance->socket_, message, 1024) == 0) {
            instance->stop();
            break;
        }

        printf("Received: %s\n", message);
        const auto length = strlen(message);
        if (length == 0) continue;

        const auto type = static_cast<ClientEventDataType>(message[0]);
        instance->pushEvent({type, nullptr});
    }
}

void Client::run() {
    auto *thread = SDL_CreateThread(reinterpret_cast<SDL_ThreadFunction>(Client::initializeThread), "tcp-worker",
                                    nullptr);
    SDL_DetachThread(thread);
}

int Client::clientPollEvent(Client::client_event_data_t *event) {
    if (SDL_LockMutex(getMutex())) {
        if (events_.empty()) return 0;
        *event = events_.front();
        events_.pop();

        SDL_UnlockMutex(event_mutex_);
        return 1;
    }

    fprintf(stderr, "Couldn't lock mutex: %s", SDL_GetError());
    return 0;
}

SDL_mutex *Client::getMutex() {
    if (event_mutex_ == nullptr) event_mutex_ = SDL_CreateMutex();
    return event_mutex_;
}

void Client::pushEvent(const Client::client_event_data_t &event) {
    if (SDL_LockMutex(getMutex())) {
        events_.push(event);
        SDL_UnlockMutex(event_mutex_);
    } else {
        fprintf(stderr, "Couldn't lock mutex: %s", SDL_GetError());
    }
}

int Client::isRunning() {
    return SDL_AtomicGet(&running_) == 1;
}

void Client::resume() {
    SDL_AtomicSet(&running_, 1);
}

void Client::stop() {
    SDL_AtomicSet(&running_, 0);
}

Client *Client::getInstance() {
    if (instance_ == nullptr) instance_ = new Client();
    return instance_;
}
