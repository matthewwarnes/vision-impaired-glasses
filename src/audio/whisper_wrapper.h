#ifndef __WHISPER_WRAPPER_H__
#define __WHISPER_WRAPPER_H__

#include <string>
#include <yaml-cpp/yaml.h>
#include <whisper.h>
#include <vector>

class whisper_wrapper {
public:
  whisper_wrapper(YAML::Node config);

  ~whisper_wrapper();

  int contains_speech(std::vector<float>& audio);

  int convert_audio_to_text(std::vector<float>& audio, size_t samples_to_process, std::string& text);

private:
  std::string _vad_model;
  float _vad_threshold;

  std::string _model;

  int _max_threads;

  struct whisper_vad_context * _vctx;
  struct whisper_context *_ctx;



};



#endif
