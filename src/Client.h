#pragma once

#include <queue>

#include "SDL_atomic.h"
#include "SDL_net.h"

class Client {
 private:
  SDL_atomic_t running_{};
  IPaddress ip_{};
  TCPsocket socket_;

  enum ClientEventDataType { ASK_NAME };

  typedef struct {
    ClientEventDataType type;
    void* data;
  } client_event_data_t;

  SDL_mutex* event_mutex_ = nullptr;
  std::queue<client_event_data_t> events_{};

  static void initializeThread();

  static Client* instance_;

  Client();

  SDL_mutex* getMutex();

  void pushEvent(const client_event_data_t& event);

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
  int clientPollEvent(client_event_data_t* event);

  static Client* getInstance();
};
