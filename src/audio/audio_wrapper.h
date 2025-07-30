#ifndef __AUDIO_WRAPPER_H__
#define __AUDIO_WRAPPER_H__

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class audio_wrapper {
public:
  audio_wrapper(YAML::Node config);
  ~audio_wrapper();

  int play_from_mem(std::vector<uint8_t>& audio);
  int play_from_file(const std::string filename);


  int capture_audio(const uint32_t seconds, std::vector<uint8_t>& audio);


  void list_mics() const;

private:
  std::string _mic_dev;


  int find_device_id(const std::string device_name) const;
};


#endif
