#ifndef __CONTROL_THREAD_H__
#define __CONTROL_THREAD_H__


#include <thread>
#include <mutex>
#include <atomic>
#include <yaml-cpp/yaml.h>

#include "openai/ai_wrapper.h"
#include "audio/audio_wrapper.h"
#include "audio/espeak_wrapper.h"
#include "image_thread.h"

class control_thread {
public:
  control_thread(YAML::Node& config, image_thread& it);
  ~control_thread();

  int start();

  void cancel();
  bool is_running();

  audio_wrapper& get_audio();

private:

  std::thread _thread;
  std::atomic<bool> _thread_ctrl;

  std::recursive_mutex _mutex;
  bool _running;

  whisper_wrapper _whisp;
  ai_wrapper _ai;
  audio_wrapper _au;
  speech_synth _speech;

  bool _cmdLocalSttOnly;
  bool _aiLocalSttOnly;
  std::vector<std::string> _aiActivation;
  std::vector<std::string> _cmdActivation;

  std::vector<std::string> _image_words;

  bool _echo_speech;
  bool _read_text_back;

  image_thread& _img_thread;

  bool requires_image(const std::string message);
  bool is_activation(const std::string message);
  bool is_ai_activation(const std::string message);
  bool is_cmd_activation(const std::string message);


  void thread_handler();


};

#endif
