#pragma once

#include <queue>
#include <string>
#include <utility>

#include "SDL_atomic.h"
#include "SDL_net.h"

class Client {
 private:
  SDL_atomic_t running_{};
  IPaddress ip_{};
  TCPsocket socket_;

  enum ClientEventDataType {
    /**
     * \brief Command sent via broadcast to all connected users announcing a new
     * game. \payload nullptr.
     */
    GAME_AVAILABLE,

    /**
     * \brief Command sent via broadcast to all connected users announcing the
     * lobby has been locked. \payload nullptr.
     */
    GAME_UNAVAILABLE,

    /**
     * \brief Command sent via broadcast to all users announcing the game start.
     * \note Not to be confused with `ClientEventDataType::PLAYER_READY`.
     * \payload nullptr.
     */
    GAME_READY,

    /**
     * \brief Command from the server to ask for the name.
     * \payload nullptr on success.
     */
    ASK_NAME,

    /**
     * \brief Command sent via broadcast to all players to add a new one.
     * \payload Player's position and name. (Health is always full).
     */
    PLAYER_ADD,

    /**
     * \brief Command sent via broadcast to all users announcing one player has
     * clicked on ready. \payload Amount of players ready.
     */
    PLAYER_READY,

    /**
     * \brief Command sent via broadcast to all players to remove one.
     * \payload Player's name.
     */
    PLAYER_REMOVE,

    /**
     * \brief Command sent via broadcast to all players to mark a player as
     * dead. \payload Player's name.
     */
    PLAYER_DEATH,

    /**
     * \brief Command sent via broadcast to all players to mark a player as
     * alive. \payload Player's name.
     */
    PLAYER_REVIVE,

    /**
     * \brief Command sent to joining players to add all the current players.
     * \payload Players health, position, direction, speed, and name.
     */
    PLAYERS_SYNC,

    /**
     * \brief Command sent via broadcast to all players to add a new bullet.
     * \payload The bullet information, including the origin (player's shooter)
     * and the direction.
     */
    SHOT_CREATE,

    /**
     * \brief Command sent via broadcast to all players to remove a bullet.
     * \payload The bullet's ID, sent from the
     * `ClientEventDataType::SHOT_CREATE` payload.
     */
    SHOT_DESTROY,

    /**
     * \brief Invalid code, used to check boundaries.
     */
    INVALID
  };

  class ClientEventBase {
   public:
    explicit ClientEventBase(ClientEventDataType type) : type_(type) {}
    ClientEventDataType type_;
  };

  class ClientEventGameAvailable : public ClientEventBase {
   public:
    ClientEventGameAvailable() : ClientEventBase(GAME_AVAILABLE) {}
  };
  class ClientEventGameUnavailable : public ClientEventBase {
   public:
    ClientEventGameUnavailable() : ClientEventBase(GAME_UNAVAILABLE) {}
  };
  class ClientEventGameReady : public ClientEventBase {
   public:
    ClientEventGameReady() : ClientEventBase(GAME_READY) {}
  };
  class ClientEventGameAskName : public ClientEventBase {
   public:
    explicit ClientEventGameAskName(std::string error)
        : ClientEventBase(ASK_NAME), reason_(std::move(error)) {}
    std::string reason_;
  };
  class ClientEventGamePlayerAdd : public ClientEventBase {
   public:
    ClientEventGamePlayerAdd(uint8_t id, float x, float y, std::string name)
        : ClientEventBase(PLAYER_ADD),
          id_(id),
          x_(x),
          y_(y),
          name_(std::move(name)) {}
    uint8_t id_;
    float x_;
    float y_;
    std::string name_;
  };
  class ClientEventGamePlayerReady : public ClientEventBase {
   public:
    explicit ClientEventGamePlayerReady(uint8_t amount)
        : ClientEventBase(PLAYER_READY), amount_(amount) {}
    uint8_t amount_;
  };
  class ClientEventGamePlayerRemove : public ClientEventBase {
   public:
    explicit ClientEventGamePlayerRemove(uint8_t id)
        : ClientEventBase(PLAYER_REMOVE), id_(id) {}
    uint8_t id_;
  };
  class ClientEventGamePlayerDeath : public ClientEventBase {
   public:
    explicit ClientEventGamePlayerDeath(uint8_t id)
        : ClientEventBase(PLAYER_DEATH), id_(id) {}
    uint8_t id_;
  };
  class ClientEventGamePlayerRevive : public ClientEventBase {
   public:
    explicit ClientEventGamePlayerRevive(uint8_t id)
        : ClientEventBase(PLAYER_REVIVE), id_(id) {}
    uint8_t id_;
  };
  class ClientEventGamePlayerSync : public ClientEventBase {
   public:
    typedef struct {
      uint8_t id;
      float x;
      float y;
      float direction;
      float speed;
    } player_t;

    explicit ClientEventGamePlayerSync(std::vector<player_t> players)
        : ClientEventBase(PLAYERS_SYNC), players_(std::move(players)) {}
    std::vector<player_t> players_;
  };
  class ClientEventGameShotCreate : public ClientEventBase {
   public:
    ClientEventGameShotCreate(uint32_t id, float x, float y, float direction,
                              uint8_t shooter)
        : ClientEventBase(SHOT_CREATE),
          id_(id),
          x_(x),
          y_(y),
          direction_(direction),
          shooter_(shooter) {}
    uint32_t id_;
    float x_;
    float y_;
    float direction_;
    uint8_t shooter_;
  };
  class ClientEventGameShotDestroy : public ClientEventBase {
   public:
    explicit ClientEventGameShotDestroy(uint32_t id)
        : ClientEventBase(SHOT_DESTROY), id_(id) {}
    uint32_t id_;
  };

  SDL_mutex* event_mutex_ = nullptr;
  std::queue<ClientEventBase> events_{};

  static void initializeThread();

  static Client* instance_;

  Client();

  SDL_mutex* getMutex();

  void pushEvent(const ClientEventBase& event);

  static Client::ClientEventBase* parseContent(char* buffer);

 public:
  ~Client();

  void run();

  int isRunning();

  void resume();

  void stop();

  /**
   *  \brief Polls for currently pending events.
   *
   *  \return 1 if there are any pending events, or 0 if there are none
   * available.
   *
   *  \param event If not nullptr, the next event is removed from the queue and
   *               stored in that area.
   */
  int clientPollEvent(ClientEventBase* event);

  static Client* getInstance();
};
