#ifndef __CONTROL_THREAD_H__
#define __CONTROL_THREAD_H__


#include <thread>
#include <mutex>
#include <atomic>
#include <yaml-cpp/yaml.h>

#include "openai/ai_wrapper.h"
#include "audio/audio_wrapper.h"
#include "image_thread.h"

class control_thread {
public:
  control_thread(YAML::Node& config, image_thread& it);
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

  std::vector<std::string> _image_words;

  image_thread& _img_thread;

  bool requires_image(const std::string message);

  void thread_handler();

};

#endif
