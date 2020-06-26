#pragma once

#include <array>
#include <queue>
#include <string>
#include <vector>

#include "SDL.h"
#include "SDL_net.h"

class Server {
  enum ClientStatus { PENDING, RUNNING, CLOSED };

  typedef struct {
    uint32_t id;
    std::string name;
  } user_t;

  typedef struct {
    uint32_t id;
    std::string name;
    bool ready;
  } potential_player_t;

  typedef struct {
    uint32_t userID;
    std::string name;
    uint8_t id;
    float x;
    float y;
    float direction;
    float speed;
    bool alive;
    time_t availableShoot;
    time_t availableRevive;
  } player_t;

  typedef struct {
    uint32_t id;
    float x;
    float y;
    float direction;
    float speed;
    time_t expires;
  } bullet_t;

  class ServerGame {
    enum class Status { OPEN, CLOSED };

    Status status_ = Status::OPEN;
    std::vector<player_t> users_{};

    uint8_t queueSize_ = 0;
    uint8_t readySize_ = 0;
    uint32_t bulletID_ = 0;
    std::array<potential_player_t, 8> queue_{};
    std::vector<player_t> players_{};
    std::vector<bullet_t> bullets_{};

    static char getCharacterFrom(int type);
    static void write8(char* buffer, char input, size_t offset) {
      buffer[offset] = input;
    }
    static void write8(char* buffer, uint8_t input, size_t offset) {
      write8(buffer, static_cast<char>(input + '0'), offset);
    }
    static void write32(char* buffer, const char* input, size_t offset) {
      memcpy(buffer + offset, input, 4);
    }
    static void write32(char* buffer, uint32_t input, size_t offset) {
      write32(buffer, std::to_string(input).c_str(), offset);
    }
    static void write32(char* buffer, float input, size_t offset) {
      write32(buffer, std::to_string(static_cast<int>(input * 10)).c_str(),
              offset);
    }

   public:
    bool addPlayer(const user_t& user);

    bool readyPlayer(const user_t& user);

    bool removePlayer(const user_t& user);

    bool shoot(const user_t& user);

    bool ready();

    bool end();

    void check();
  };

  typedef struct {
    char* data;
    int length;
  } client_event_data_t;

  class ServerClient {
    ClientStatus status_ = ClientStatus::PENDING;
    IPaddress* remoteIP_ = nullptr;
    TCPsocket socket_;
    SDL_mutex* event_mutex_ = nullptr;
    std::queue<client_event_data_t> events_;

    void initialize();

    explicit ServerClient(TCPsocket socket);

    /**
     *  \brief Polls for currently pending events.
     *
     *  \return 1 if there are any pending events, or 0 if there are none
     * available.
     *
     *  \param event If not nullptr, the next event is removed from the queue
     * and stored in that area.
     */
    int clientPollEvent(client_event_data_t* event);

   public:
    ~ServerClient() = default;

    bool isPending();

    bool isRunning();

    bool isClosed();

    SDL_mutex* getMutex();

    void pushEvent(const client_event_data_t& event);

    static int create(TCPsocket socket);
  };

  enum ServerEventDataType { CONNECT, DISCONNECT };

  typedef struct {
    ServerEventDataType type;
    ServerClient* sender;
    void* data;
  } server_event_data_t;

  static Server* instance_;
  static SDL_atomic_t running_;
  static SDL_mutex* event_mutex_;
  static std::queue<server_event_data_t> events_;
  std::vector<ServerClient*> clients_{};
  TCPsocket server_ = nullptr;
  IPaddress ip_{};
  bool done_ = false;
  ServerGame* game_;

  /**
   *  \brief Polls for currently pending events.
   *
   *  \return 1 if there are any pending events, or 0 if there are none
   * available.
   *
   *  \param event If not nullptr, the next event is removed from the queue and
   *               stored in that area.
   */
  static int clientPollEvent(server_event_data_t* event);

  Server();

 public:
  ~Server();

  void run();

  static Server* getInstance();

  static int getRunning();

  static SDL_mutex* getMutex();

  void broadcast(char* message, int length);

  static void pushEvent(const server_event_data_t& event);
};
