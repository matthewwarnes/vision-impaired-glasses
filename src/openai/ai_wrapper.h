#ifndef __AI_WRAPPER_H__
#define __AI_WRAPPER_H__

#include <string>
#include <yaml-cpp/yaml.h>

class ai_wrapper {
public:
  ai_wrapper(YAML::Node config);

  int convert_text_to_audio(const std::string input, std::vector<uint8_t>& output);
  int convert_audio_to_text(const std::vector<uint8_t>& wavData, std::string &text);

  int ai_text_to_text(const std::string input, std::string& output);
  int ai_text_to_audio(const std::string input, std::vector<uint8_t>& output);
  int ai_text_image_to_text(const std::string input, const std::vector<uint8_t>& img, std::string& output);

private:
  std::string _key;
  std::string _model;
  std::string _transcribe_model;
  std::string _image_model;
  std::string _tts_model;
  std::string _voice;
};


#endif
