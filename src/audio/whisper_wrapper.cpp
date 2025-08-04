#include "whisper_wrapper.h"

#include <iostream>
#include <thread>
#include <spdlog/spdlog.h>

const int    vad_min_speech_duration_ms = 250;
const int    vad_min_silence_duration_ms = 1000;
const float  vad_max_speech_duration_s = 10.0f;
const int    vad_speech_pad_ms = 30;
const float  vad_samples_overlap = 0.1f;

const int32_t audio_ctx = 0;
const int32_t beam_size = 5;

void cb_log(enum ggml_log_level level, const char * str, void * arg) {
  if(level == 3) {
    spdlog::warn(" [whisper] {}", str);
  }
  if(level > 3) {
    spdlog::error(" [whisper] {}", str);
  }
}

whisper_wrapper::whisper_wrapper(YAML::Node config) {
  _vctx = nullptr;
  _vad_model = config["vadModel"].as<std::string>();
  _vad_threshold = config["vadThreshold"].as<float>();

  _model = config["model"].as<std::string>();

  _max_threads = config["maxThreads"].as<int>();

  ggml_backend_load_all();
  whisper_log_set(cb_log, NULL);

  // Initialize the context which loads the VAD model.
  struct whisper_vad_context_params ctx_params = whisper_vad_default_context_params();
  ctx_params.n_threads  = std::min(_max_threads, (int32_t) std::thread::hardware_concurrency());
  ctx_params.use_gpu    = false;
  _vctx = whisper_vad_init_from_file_with_params(
          _vad_model.c_str(),
          ctx_params);
  if (_vctx == nullptr) {
    spdlog::error("failed to initialise whisper vad context");
    exit(EXIT_FAILURE);
  }

  struct whisper_context_params cparams = whisper_context_default_params();
  cparams.use_gpu = false;
  cparams.flash_attn = false;
  _ctx = whisper_init_from_file_with_params(_model.c_str(), cparams);
  if (_ctx == nullptr) {
    spdlog::error("failed to initialise whisper context");
    exit(EXIT_FAILURE);
  }


}

whisper_wrapper::~whisper_wrapper() {
  whisper_vad_free(_vctx);
  whisper_free(_ctx);
}

int whisper_wrapper::contains_speech(std::vector<float>& audioin) {
  // Detect speech in the input audio file.
  if (!whisper_vad_detect_speech(_vctx, audioin.data(), audioin.size())) {
    spdlog::error("failed to detect speech");
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

int whisper_wrapper::convert_audio_to_text(std::vector<float>& audio, size_t samples_to_process, std::string& text) {
  //perform local speech to text conversion

  whisper_full_params wparams = whisper_full_default_params(beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY);

  wparams.print_progress   = false;
  wparams.print_special    = false;
  wparams.print_realtime   = false;
  wparams.print_timestamps = false;
  wparams.translate        = false;
  wparams.single_segment   = true;
  wparams.max_tokens       = 100;
  wparams.language         = "en";
  wparams.n_threads        = std::min(_max_threads, (int32_t) std::thread::hardware_concurrency());
  wparams.beam_search.beam_size = beam_size;
  wparams.audio_ctx        = audio_ctx;
  wparams.tdrz_enable      = false; // [TDRZ]
  // disable temperature fallback
  //wparams.temperature_inc  = -1.0f;
  wparams.temperature_inc  = 0.0f;
  wparams.prompt_tokens    = nullptr;
  wparams.prompt_n_tokens  = 0;

  if (whisper_full(_ctx, wparams, audio.data(), samples_to_process) != 0) {
    spdlog::error("failed to whisper process audio");
    return -1;
  }

  std::stringstream ss;
  {
      const int n_segments = whisper_full_n_segments(_ctx);
      for (int i = 0; i < n_segments; ++i) {
          const char * text = whisper_full_get_segment_text(_ctx, i);
          ss << text;
      }
  }
  text = ss.str();


  return 0;
}
