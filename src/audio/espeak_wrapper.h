#ifndef __ESPEAK_WRAPPER_H__
#define __ESPEAK_WRAPPER_H__

#include <string>
#include <vector>
#include <queue>
#include <map>

#include <onnxruntime_cxx_api.h>
#include <yaml-cpp/yaml.h>

class speech_synth {
public:
  speech_synth(YAML::Node config);

  ~speech_synth();

  int convert_text_to_audio(const std::string, std::vector<uint8_t>& audio);


private:
  std::string _espeak_voice;
  int _sample_rate;
  int _num_speakers;
  std::map<char32_t, std::vector<int64_t>> _phoneme_id_map;

  // Default synthesis settings for the voice
  float _length_scale;
  float _noise_scale;
  float _noise_w_scale;

  int64_t _speaker_id;

  // onnx
  std::unique_ptr<Ort::Session> _session;
  Ort::SessionOptions _session_options;

  int start(const std::string text, std::queue<std::vector<int64_t>>& id_queue);
  int next(std::vector<int64_t> next_ids, std::vector<float>& samples);


};

#endif
