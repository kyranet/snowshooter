#include "Server.h"

#include <ctime>
#include <string>

#include "Client.h"

bool Server::ServerGame::addPlayer(const Server::user_t& user) {
  if (status_ != Status::OPEN) return false;
  if (queueSize_ == 8) return false;

  for (auto& entry : queue_) {
    if (entry.id == user.id) return false;
  }

  queue_[queueSize_++] = {user.id, user.name, false};
  return true;
}

bool Server::ServerGame::readyPlayer(const Server::user_t& user) {
  for (auto& entry : queue_) {
    if (entry.id == user.id) {
      if (entry.ready) return false;

      entry.ready = true;
      ++readySize_;
      return true;
    }
  }

  return false;
}

bool Server::ServerGame::removePlayer(const Server::user_t& user) {
  // Scan from queue
  size_t playerID = queue_.size();
  for (size_t i = 0; i < queue_.size(); ++i) {
    if (queue_[i].id == user.id) {
      playerID = i;
      if (queue_[i].ready) --readySize_;
      break;
    }
  }

  if (playerID == queue_.size()) return false;

  // Remove from queue
  for (size_t i = playerID; i < queue_.size() - 1; ++i) {
    queue_[i] = queue_[i + 1];
  }

  // Scan from game
  for (auto it = players_.begin(); it != players_.end(); ++it) {
    if (it->userID == user.id) {
      // Remove from game
      players_.erase(it);
      break;
    }
  }

  return true;
}

bool Server::ServerGame::shoot(const Server::user_t& user) {
  for (auto& player : players_) {
    if (player.id == user.id) {
      const auto now = std::time(nullptr);
      if (player.availableShoot > now) return false;

      player.availableShoot = now + 750;
      bullet_t bullet{bulletID_++,      player.x, player.y,
                      player.direction, 25.0f,    now + 10000};
      bullets_.push_back(bullet);

      const auto server = Server::getInstance();

      // Broadcast message
      char bulletShotMessage[18];
      write8(bulletShotMessage, getCharacterFrom(SHOT_CREATE), 0);
      write32(bulletShotMessage, bullet.id, 1);
      write32(bulletShotMessage, bullet.x, 5);
      write32(bulletShotMessage, bullet.y, 9);
      write32(bulletShotMessage, bullet.direction, 13);
      write8(bulletShotMessage, player.id, 17);
      server->broadcast(bulletShotMessage, 18);

      return true;
    }
  }

  return false;
}

bool Server::ServerGame::ready() {
  if (status_ == Status::CLOSED) return false;

  status_ = Status::CLOSED;
  for (size_t i = 0; i < queue_.size(); ++i) {
    const auto user = queue_[i];
    players_[i] = {user.id,
                   user.name,
                   static_cast<uint8_t>(i),
                   static_cast<float>(((i % 2) * 80) - 40),
                   static_cast<float>(((i % 4) * 40) - 20),
                   0.0f,
                   0.0f,
                   true,
                   0,
                   0};
  }

  const auto server = Server::getInstance();

  // Broadcast message
  char readyMessage[]{getCharacterFrom(GAME_READY)};
  server->broadcast(readyMessage, 1);

  return true;
}

bool Server::ServerGame::end() {
  if (status_ == Status::OPEN) return false;

  status_ = Status::OPEN;
  queueSize_ = 0;
  readySize_ = 0;
  bulletID_ = 0;
  players_.clear();
  bullets_.clear();

  const auto server = Server::getInstance();

  // Broadcast message
  char readyMessage[]{getCharacterFrom(GAME_END)};
  server->broadcast(readyMessage, 1);

  return true;
}

void Server::ServerGame::check() {
  const auto now = std::time(nullptr);
  const auto server = Server::getInstance();

  size_t i = 0;

  // Clean-up expired bullets
  while (i < bullets_.size()) {
    const auto bullet = bullets_[i];
    if (bullet.expires >= now) {
      bullets_.erase(bullets_.begin() + static_cast<long>(i));

      // Broadcast message
      char bulletDestroyMessage[5];
      write8(bulletDestroyMessage, getCharacterFrom(SHOT_DESTROY), 0);
      write32(bulletDestroyMessage, bullet.id, 1);
      server->broadcast(bulletDestroyMessage, 5);
    } else {
      ++i;
    }
  }

  // Resurrect players
  const auto size = players_.size() * 17 + 1;
  auto* syncMessage = static_cast<char*>(malloc(size + 1));
  write8(syncMessage, getCharacterFrom(PLAYERS_SYNC), 0);
  size_t offset = 1;

  i = 0;
  while (i < players_.size()) {
    auto& player = players_[i];
    if (!player.alive && player.availableRevive >= now) {
      player.alive = true;

      // Broadcast message
      char reviveMessage[2];
      write8(reviveMessage, getCharacterFrom(PLAYER_REVIVE), 0);
      write8(reviveMessage, player.id, 1);
      server->broadcast(reviveMessage, 2);
    }

    write8(syncMessage, player.id, offset);
    write32(syncMessage, player.x, offset + 4);
    write32(syncMessage, player.y, offset + 8);
    write32(syncMessage, player.direction, offset + 12);
    write32(syncMessage, player.speed, offset + 16);
    offset += 17;
  }

  server->broadcast(syncMessage, static_cast<int>(size));
}

char Server::ServerGame::getCharacterFrom(int type) {
  return static_cast<char>(type + 'a');
}

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
      SDLNet_TCP_Send(socket_, ed.data, ed.length);
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

  game_ = new ServerGame();
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

void Server::broadcast(char* message, int length) {
  for (auto& client : clients_) {
    client->pushEvent({message, length});
  }
}
