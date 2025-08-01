#ifndef __AUDIO_WRAPPER_H__
#define __AUDIO_WRAPPER_H__

#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <portaudio.h>

#include "whisper_wrapper.h"


class audio_wrapper {
public:
  audio_wrapper(YAML::Node config, whisper_wrapper& w, std::atomic<bool>& cancel);
  ~audio_wrapper();

  int start();

  int play_from_mem(std::vector<uint8_t>& audio);
  int play_from_file(const std::string filename);

  int check_for_speech(std::vector<uint8_t>& speechAudio, std::string& estimated_text);

  void clear_speech_buffer();

  void list_mics() const;

private:

  std::atomic<bool>& _thread_ctrl;

  std::string _mic_dev;
  uint32_t _samples_per_second;
  uint32_t _samples_per_check;


  whisper_wrapper& _whisp;

  std::vector<float> _audio_buffer;
  std::vector<float> _pre_speech;

  std::recursive_mutex _audio_mutex;
  std::vector<float> _audio_to_check;
  bool _audio_pending;

  PaStream* _stream;


  std::vector<float> _speech_segment;


  int find_device_id(const std::string device_name) const;

  void thread_handler();

  int recordCallback(const void *inputBuffer, unsigned long framesPerBuffer,
                      PaStreamCallbackFlags statusFlags);

  static int recordCallbackStatic(const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData);

};


#endif
