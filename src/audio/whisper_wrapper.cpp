#include "whisper_wrapper.h"

#include <iostream>
#include <thread>

const int    vad_min_speech_duration_ms = 250;
const int    vad_min_silence_duration_ms = 1000;
const float  vad_max_speech_duration_s = 10.0f;
const int    vad_speech_pad_ms = 30;
const float  vad_samples_overlap = 0.1f;

void cb_log(enum ggml_log_level level, const char * str, void * arg) {
  if((int)level >= 3) { //warn or more
    std::cout << str << std::endl;
  }
}

whisper_wrapper::whisper_wrapper(YAML::Node config) {
  _vctx = nullptr;
  _vad_model = config["vadModel"].as<std::string>();
  _vad_threshold = config["vadThreshold"].as<float>();

  _model = config["model"].as<std::string>();

  ggml_backend_load_all();
  whisper_log_set(cb_log, NULL);

  // Initialize the context which loads the VAD model.
  struct whisper_vad_context_params ctx_params = whisper_vad_default_context_params();
  ctx_params.n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
  ctx_params.use_gpu    = false;
  _vctx = whisper_vad_init_from_file_with_params(
          _vad_model.c_str(),
          ctx_params);
  if (_vctx == nullptr) {
    std::cerr << "ERROR: failed to initialise whisper vad context" << std::endl;
    exit(EXIT_FAILURE);
  }
}

whisper_wrapper::~whisper_wrapper() {
  whisper_vad_free(_vctx);
}

int whisper_wrapper::start() {
  std::cout << "WHISPER: " << _model << std::endl;
  return 0;
}

struct cli_params {
    int32_t     n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    std::string vad_model = "";
    float       vad_threshold = 0.5f;

    bool        use_gpu = false;
    std::string fname_inp = {};
};

int whisper_wrapper::contains_speech(std::vector<float>& audioin) {
  // Detect speech in the input audio file.
  if (!whisper_vad_detect_speech(_vctx, audioin.data(), audioin.size())) {
      fprintf(stderr, "error: failed to detect speech\n");
      return 3;
  }

  // Get the the vad segements using the probabilities that have been computed
  // previously and stored in the whisper_vad_context.
  struct whisper_vad_params params = whisper_vad_default_params();
  params.threshold = _vad_threshold;
  params.min_speech_duration_ms = vad_min_speech_duration_ms;
  params.min_silence_duration_ms = vad_min_silence_duration_ms;
  params.max_speech_duration_s = vad_max_speech_duration_s;
  params.speech_pad_ms = vad_speech_pad_ms;
  params.samples_overlap = vad_samples_overlap;
  struct whisper_vad_segments * segments = whisper_vad_segments_from_probs(_vctx, params);

  int ret = whisper_vad_segments_n_segments(segments);
  whisper_vad_free_segments(segments);
  return ret;
}
