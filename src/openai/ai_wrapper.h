#ifndef __AI_WRAPPER_H__
#define __AI_WRAPPER_H__

#include <string>
#include <yaml-cpp/yaml.h>

class ai_wrapper {
public:
  ai_wrapper(YAML::Node config);

  int convert_text_to_audio(std::string input);
  int convert_audio_to_text(std::vector<uint8_t>& wavData, std::string &text);

  int ai_text_to_text(std::string input, std::string& output);
  int ai_text_to_audio(std::string input, std::vector<uint8_t>& output);
  int ai_audio_to_audio();


private:
  std::string _key;
  std::string _model;
  std::string _transcribe_model;
  std::string _voice;
};


#endif
