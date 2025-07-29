#ifndef __AUDIO_WRAPPER_H__
#define __AUDIO_WRAPPER_H__

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class audio_wrapper {
public:
  audio_wrapper(YAML::Node config);

  int play_from_mem(std::vector<uint8_t>& audio);
  int play_from_file(std::string filename);

private:

};


#endif
