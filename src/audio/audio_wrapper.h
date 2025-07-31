#ifndef __AUDIO_WRAPPER_H__
#define __AUDIO_WRAPPER_H__

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "whisper_wrapper.h"

class audio_wrapper {
public:
  audio_wrapper(YAML::Node config, whisper_wrapper& w);
  ~audio_wrapper();

  int play_from_mem(std::vector<uint8_t>& audio);
  int play_from_file(const std::string filename);


  int capture_audio(const uint32_t seconds, std::vector<uint8_t>& audio);
  int capture_audio(const uint32_t seconds, std::vector<float>& audio);

  int capture_speech(std::vector<uint8_t>& speechAudio, std::string& estimated_text, std::atomic<bool>& cancel);


  void list_mics() const;

private:
  std::string _mic_dev;

  whisper_wrapper& _whisp;

  int find_device_id(const std::string device_name) const;
};


#endif
