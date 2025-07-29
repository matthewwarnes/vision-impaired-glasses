#ifndef __CONTROL_THREAD_H__
#define __CONTROL_THREAD_H__


#include <thread>
#include <mutex>
#include <atomic>

#include "openai/ai_wrapper.h"
#include "audio/audio_wrapper.h"


class control_thread {
public:
  control_thread(YAML::Node& config);
  ~control_thread();

  int start();

  void cancel();
  bool is_running();

private:

  std::thread _thread;
  std::atomic<bool> _thread_ctrl;

  std::recursive_mutex _mutex;
  bool _running;

  ai_wrapper _ai;
  audio_wrapper _au;


  void thread_handler();

};

#endif
