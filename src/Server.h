#pragma once

#include <queue>
#include <vector>

#include "SDL.h"
#include "SDL_net.h"

class Server {
  enum ClientStatus { PENDING, RUNNING, CLOSED };

  enum ClientEventDataType { ASK_NAME };

  typedef struct {
    ClientEventDataType type;
    void* data;
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

  static void pushEvent(const server_event_data_t& event);
};
