#include "Client.h"

Client* Client::instance_ = nullptr;

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

    const auto* payload = parseContent(message);
    if (payload == nullptr) continue;
    instance->pushEvent(*payload);
  }
}

Client::ClientEventBase* Client::parseContent(char* message) {
  const auto length = strlen(message);
  if (length == 0) return nullptr;

  const auto bit = static_cast<int>(message[0] - 'a');
  if (bit < 0 || bit >= ClientEventDataType::INVALID) return nullptr;

  const auto type = static_cast<ClientEventDataType>(bit);
  switch (type) {
    case GAME_AVAILABLE:
      return new ClientEventGameAvailable();
    case GAME_UNAVAILABLE:
      return new ClientEventGameUnavailable();
    case GAME_READY:
      return new ClientEventGameReady();
    case GAME_END:
      return new ClientEventGameEnd();
    case ASK_NAME: {
      std::string name(message + 1, length - 1);
      return new ClientEventGameAskName(name);
    }
    case PLAYER_ADD: {
      const auto id = static_cast<uint8_t>(message[1] - '0');
      std::string rawX(message + 2, 4);
      std::string rawY(message + 6, 4);
      std::string name(message + 10, length - 10);
      const auto x =
          static_cast<float>(strtol(rawX.c_str(), nullptr, 10)) / 10.0f;
      const auto y =
          static_cast<float>(strtol(rawY.c_str(), nullptr, 10)) / 10.0f;
      return new ClientEventGamePlayerAdd(id, x, y, name);
    }
    case PLAYER_READY: {
      const auto amount = static_cast<uint8_t>(message[1] - '0');
      return new ClientEventGamePlayerReady(amount);
    }
    case PLAYER_REMOVE: {
      const auto id = static_cast<uint8_t>(message[1] - '0');
      return new ClientEventGamePlayerRemove(id);
    }
    case PLAYER_DEATH: {
      const auto id = static_cast<uint8_t>(message[1] - '0');
      return new ClientEventGamePlayerDeath(id);
    }
    case PLAYER_REVIVE: {
      const auto id = static_cast<uint8_t>(message[1] - '0');
      return new ClientEventGamePlayerRevive(id);
    }
    case PLAYERS_SYNC: {
      const auto count = (length - 1) / 17;
      std::vector<ClientEventGamePlayerSync::player_t> players(count);
      size_t offset = 1;
      for (size_t i = 0; i < count; ++i) {
        const auto id = static_cast<uint8_t>(message[offset] - '0');
        std::string rawX(message + offset, 4);
        std::string rawY(message + (offset + 4), 4);
        std::string rawDirection(message + (offset + 8), 4);
        std::string rawSpeed(message + (offset + 12), 4);
        const auto x =
            static_cast<float>(strtol(rawX.c_str(), nullptr, 10)) / 10.0f;
        const auto y =
            static_cast<float>(strtol(rawY.c_str(), nullptr, 10)) / 10.0f;
        const auto direction =
            static_cast<float>(strtol(rawDirection.c_str(), nullptr, 10)) /
            10.0f;
        const auto speed =
            static_cast<float>(strtol(rawSpeed.c_str(), nullptr, 10)) / 10.0f;
        players[i] = {id, x, y, direction, speed};
        offset += 17;
      }

      return new ClientEventGamePlayerSync(players);
    }
    case SHOT_CREATE: {
      std::string rawID(message + 1, 4);
      std::string rawX(message + 5, 4);
      std::string rawY(message + 9, 4);
      std::string rawDirection(message + 13, 4);
      const auto shooter = static_cast<uint8_t>(message[17] - '0');
      const auto id = static_cast<uint32_t>(strtol(rawID.c_str(), nullptr, 10));
      const auto x =
          static_cast<float>(strtol(rawX.c_str(), nullptr, 10)) / 10.0f;
      const auto y =
          static_cast<float>(strtol(rawY.c_str(), nullptr, 10)) / 10.0f;
      const auto direction =
          static_cast<float>(strtol(rawDirection.c_str(), nullptr, 10)) / 10.0f;
      return new ClientEventGameShotCreate(id, x, y, direction, shooter);
    }
    case SHOT_DESTROY: {
      std::string rawID(message + 1, 4);
      const auto id = static_cast<uint32_t>(strtol(rawID.c_str(), nullptr, 10));
      return new ClientEventGameShotDestroy(id);
    }
    case INVALID:
      break;
  }
  return nullptr;
}

void Client::run() {
  auto* thread = SDL_CreateThread(
      reinterpret_cast<SDL_ThreadFunction>(Client::initializeThread),
      "tcp-worker", nullptr);
  SDL_DetachThread(thread);
}

int Client::clientPollEvent(Client::ClientEventBase* event) {
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

SDL_mutex* Client::getMutex() {
  if (event_mutex_ == nullptr) event_mutex_ = SDL_CreateMutex();
  return event_mutex_;
}

void Client::pushEvent(const Client::ClientEventBase& event) {
  if (SDL_LockMutex(getMutex())) {
    events_.push(event);
    SDL_UnlockMutex(event_mutex_);
  } else {
    fprintf(stderr, "Couldn't lock mutex: %s", SDL_GetError());
  }
}

int Client::isRunning() { return SDL_AtomicGet(&running_) == 1; }

void Client::resume() { SDL_AtomicSet(&running_, 1); }

void Client::stop() { SDL_AtomicSet(&running_, 0); }

Client* Client::getInstance() {
  if (instance_ == nullptr) instance_ = new Client();
  return instance_;
}
