#include "Server.h"

#include <string>

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
         (ipAddress >> 16u) & 0xFFu, (ipAddress >> 8u) & 0xFFu,
         ipAddress & 0xFFu, remoteIP_->port);

  status_ = ClientStatus::RUNNING;
  Server::pushEvent({ServerEventDataType::CONNECT, this, &ipAddress});

  while (Server::getRunning()) {
    // Handle game events on queue
    client_event_data_t ed;
    while (clientPollEvent(&ed) != 0) {
      switch (ed.type) {
        case ClientEventDataType::ASK_NAME:
          printf("Asked for the name!");
          char message[1];
          message[0] = ed.type;
          SDLNet_TCP_Send(socket_, message, 1);
          break;
      }
    }

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
      Server::pushEvent({ServerEventDataType::DISCONNECT, this, nullptr});
      break;
    }
  }

  status_ = ClientStatus::CLOSED;
  SDLNet_TCP_Close(socket_);
}

Server::ServerClient::ServerClient(TCPsocket socket) : socket_(socket) {}

bool Server::ServerClient::isPending() {
  return status_ == ClientStatus::PENDING;
}

bool Server::ServerClient::isRunning() {
  return status_ == ClientStatus::RUNNING;
}

bool Server::ServerClient::isClosed() {
  return status_ == ClientStatus::CLOSED;
}

int Server::ServerClient::create(TCPsocket socket) {
  auto* client = new ServerClient(socket);
  client->initialize();
  return 0;
}

SDL_mutex* Server::ServerClient::getMutex() {
  if (event_mutex_ == nullptr) event_mutex_ = SDL_CreateMutex();
  return event_mutex_;
}

void Server::ServerClient::pushEvent(const Server::client_event_data_t& event) {
  if (SDL_LockMutex(getMutex())) {
    events_.push(event);
    SDL_UnlockMutex(event_mutex_);
  } else {
    fprintf(stderr, "Couldn't lock mutex: %s", SDL_GetError());
  }
}

int Server::ServerClient::clientPollEvent(Server::client_event_data_t* event) {
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

Server* Server::instance_ = nullptr;
SDL_atomic_t Server::running_{};
SDL_mutex* Server::event_mutex_ = nullptr;
std::queue<Server::server_event_data_t> Server::events_{};

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
  if (server_ != nullptr) SDLNet_TCP_Close(server_);
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
    server_event_data_t ed;
    while (clientPollEvent(&ed) != 0) {
      switch (ed.type) {
        case ServerEventDataType::DISCONNECT:
          printf("Client Disconnected.");
          break;
        case ServerEventDataType::CONNECT:
          printf("Client Connected %d!", *reinterpret_cast<uint32_t*>(ed.data));
          ed.sender->pushEvent({ClientEventDataType::ASK_NAME, nullptr});
          break;
      }
    }

    // Free memory
    size_t i = 0;
    while (i < clients_.size()) {
      auto* client = clients_[i];
      if (client->isClosed()) {
        clients_.erase(clients_.begin() + static_cast<long>(i));
        delete client;
      } else {
        ++i;
      }
    }

    // Try to accept a connection
    auto* client = SDLNet_TCP_Accept(server_);

    // No connection accepted
    if (!client) {
      SDL_Delay(1000 / 30);
      continue;
    }

    auto* thread = SDL_CreateThread(
        reinterpret_cast<SDL_ThreadFunction>(ServerClient::create), "client",
        client);
    SDL_DetachThread(thread);
  }

  SDLNet_Quit();
  SDL_Quit();
}

Server* Server::getInstance() {
  if (instance_ == nullptr) {
    instance_ = new Server();
    SDL_AtomicSet(&running_, 1);
  }
  return instance_;
}

int Server::getRunning() { return SDL_AtomicGet(&running_); }

SDL_mutex* Server::getMutex() {
  if (event_mutex_ == nullptr) event_mutex_ = SDL_CreateMutex();
  return event_mutex_;
}

void Server::pushEvent(const Server::server_event_data_t& event) {
  if (SDL_LockMutex(getMutex())) {
    events_.push(event);
    SDL_UnlockMutex(event_mutex_);
  } else {
    fprintf(stderr, "Couldn't lock mutex: %s", SDL_GetError());
  }
}

int Server::clientPollEvent(Server::server_event_data_t* event) {
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
