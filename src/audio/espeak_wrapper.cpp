#include "espeak_wrapper.h"

#include <espeak-ng/speak_lib.h>

//#include "uni_algo.h"
#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <iostream>
#include <fstream>

#include <onnxruntime_cxx_api.h>
#include <uni_algo/all.h>

// espeak
#define CLAUSE_INTONATION_FULL_STOP 0x00000000
#define CLAUSE_INTONATION_COMMA 0x00001000
#define CLAUSE_INTONATION_QUESTION 0x00002000
#define CLAUSE_INTONATION_EXCLAMATION 0x00003000

#define CLAUSE_TYPE_CLAUSE 0x00040000
#define CLAUSE_TYPE_SENTENCE 0x00080000

#define CLAUSE_PERIOD (40 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_SENTENCE)
#define CLAUSE_COMMA (20 | CLAUSE_INTONATION_COMMA | CLAUSE_TYPE_CLAUSE)
#define CLAUSE_QUESTION (40 | CLAUSE_INTONATION_QUESTION | CLAUSE_TYPE_SENTENCE)
#define CLAUSE_EXCLAMATION                                                     \
    (45 | CLAUSE_INTONATION_EXCLAMATION | CLAUSE_TYPE_SENTENCE)
#define CLAUSE_COLON (30 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_CLAUSE)
#define CLAUSE_SEMICOLON (30 | CLAUSE_INTONATION_COMMA | CLAUSE_TYPE_CLAUSE)

typedef char32_t Phoneme;
typedef int64_t PhonemeId;
typedef int64_t SpeakerId;
typedef std::map<Phoneme, std::vector<PhonemeId>> PhonemeIdMap;

const PhonemeId ID_PAD = 0; // interleaved
const PhonemeId ID_BOS = 1; // beginning of sentence
const PhonemeId ID_EOS = 2; // end of sentence

const float DEFAULT_LENGTH_SCALE = 1.0f;
const float DEFAULT_NOISE_SCALE = 0.667f;
const float DEFAULT_NOISE_W_SCALE = 0.8f;

using json = nlohmann::json;

// onnx
Ort::Env ort_env{ORT_LOGGING_LEVEL_WARNING, "onnx"};

// Get the first UTF-8 codepoint of a string
std::optional<Phoneme> get_codepoint(std::string s) {
    auto view = una::views::utf8(s);
    auto it = view.begin();

    if (it != view.end()) {
        return *it;
    }

    return std::nullopt;
}

speech_synth::speech_synth(YAML::Node yaml_config) {

  std::string model_path = yaml_config["model"].as<std::string>();
  std::string data_path = yaml_config["data"].as<std::string>();

  std::string config_path_str = model_path + ".json";

  std::ifstream config_stream(config_path_str);
  auto config = json::parse(config_stream);

  if (espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, data_path.c_str(), 0) <
      0) {
      std::cerr << "ERROR: failed to initialise espeak" << std::endl;
      exit(EXIT_FAILURE);
  }

  // Load config options
  _espeak_voice = "en-gb"; // default
  if (config.contains("espeak")) {
      auto &espeak_obj = config["espeak"];
      if (espeak_obj.contains("voice")) {
          _espeak_voice = espeak_obj["voice"].get<std::string>();
      }
  }

  if (config.contains("audio")) {
      auto &audio_obj = config["audio"];
      if (audio_obj.contains("sample_rate")) {
          // Sample rate of generated audio in hertz
          _sample_rate = audio_obj["sample_rate"].get<int>();
      }
  }

  // phoneme to [id] map
  // Maps phonemes to one or more phoneme ids (required).
  if (config.contains("phoneme_id_map")) {
      auto &phoneme_id_map_value = config["phoneme_id_map"];
      for (auto &from_phoneme_item : phoneme_id_map_value.items()) {
          std::string from_phoneme = from_phoneme_item.key();
          auto from_codepoint = get_codepoint(from_phoneme);
          if (!from_codepoint) {
              // No codepoint
              continue;
          }

          for (auto &to_id_value : from_phoneme_item.value()) {
              PhonemeId to_id = to_id_value.get<PhonemeId>();
              _phoneme_id_map[*from_codepoint].push_back(to_id);
          }
      }
  }

  _num_speakers = config["num_speakers"].get<SpeakerId>();

  _length_scale = DEFAULT_LENGTH_SCALE;
  _noise_scale = DEFAULT_NOISE_SCALE;
  _noise_w_scale = DEFAULT_NOISE_W_SCALE;
  if (config.contains("inference")) {
      // Overrides default inference settings
      auto inference_value = config["inference"];
      if (inference_value.contains("noise_scale")) {
          _noise_scale =
              inference_value["noise_scale"].get<float>();
      }

      if (inference_value.contains("length_scale")) {
          _length_scale =
              inference_value["length_scale"].get<float>();
      }

      if (inference_value.contains("noise_w")) {
          _noise_w_scale =
              inference_value["noise_w"].get<float>();
      }
  }

  _speaker_id = 0;

  // Load onnx model
  _session_options.DisableCpuMemArena();
  _session_options.DisableMemPattern();
  _session_options.DisableProfiling();

  _session = std::make_unique<Ort::Session>(
      Ort::Session(ort_env, model_path.c_str(), _session_options));

}

speech_synth::~speech_synth() {
  espeak_Terminate();
}

int speech_synth::convert_text_to_audio(const std::string text, std::vector<uint8_t>& audio) {
  std::queue<std::vector<PhonemeId>> phoneme_id_queue;
  std::vector<float> sound;



  if(start(text, phoneme_id_queue)) {
    std::cerr << "ERROR: failed to start speech synthesis" << std::endl;
    return -1;
  }


  while(!phoneme_id_queue.empty()) {
    // Process next list of phoneme ids
    auto next_ids = std::move(phoneme_id_queue.front());
    phoneme_id_queue.pop();

    if(next(next_ids, sound)) {
      std::cerr << "ERROR: processing speech synthesis" << std::endl;
      return -1;
    }
  }

  typedef struct {
    /* RIFF Chunk Descriptor */
    uint8_t RIFF[4] = {'R', 'I', 'F', 'F'}; // RIFF Header Magic header
    uint32_t ChunkSize;                     // RIFF Chunk Size
    uint8_t WAVE[4] = {'W', 'A', 'V', 'E'}; // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t fmt[4] = {'f', 'm', 't', ' '}; // FMT header
    uint32_t Subchunk1Size = 16;           // Size of the fmt chunk
    uint16_t AudioFormat = 3; // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
                              // Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t NumOfChan = 1;   // Number of channels 1=Mono 2=Sterio
    uint32_t SamplesPerSec = 0;   // Sampling Frequency in Hz
    uint32_t bytesPerSec = 0; // bytes per second
    uint16_t blockAlign = 2;          // 2=16-bit mono, 4=16-bit stereo
    uint16_t bitsPerSample = sizeof(float)*8; // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t Subchunk2ID[4] = {'d', 'a', 't', 'a'}; // "data"  string
    uint32_t Subchunk2Size;                        // Sampled data length
  } wav_hdr_t;



  //copy audio into output wav format
  size_t numBytes = sound.size() * sizeof(float);
  audio.clear();
  audio.resize(numBytes + sizeof(wav_hdr_t));
  {
    wav_hdr_t wav;
    wav.ChunkSize = numBytes + 36;
    wav.Subchunk2Size = numBytes;
    wav.SamplesPerSec = _sample_rate;   // Sampling Frequency in Hz
    wav.bytesPerSec = _sample_rate * sizeof(float); // bytes per second
    uint8_t *p = (uint8_t *)&wav;
    for(size_t i = 0; i < sizeof(wav_hdr_t); i++) {
      audio[i] = p[i];
    }
  }
  std::memcpy(&audio[sizeof(wav_hdr_t)], (uint8_t*)sound.data(), numBytes);

  return 0;


}

int speech_synth::start(const std::string text, std::queue<std::vector<int64_t>>& id_queue) {

  if (espeak_SetVoiceByName(_espeak_voice.c_str()) != EE_OK) {
      return -1;
  }

  // phonemize
  std::vector<std::string> sentence_phonemes{""};
  std::size_t current_idx = 0;
  const void *text_ptr = text.c_str();
  while (text_ptr != nullptr) {
      int terminator = 0;
      std::string terminator_str = "";

      const char *phonemes = espeak_TextToPhonemesWithTerminator(
          &text_ptr, espeakCHARS_AUTO, espeakPHONEMES_IPA, &terminator);

      if (phonemes) {
          sentence_phonemes[current_idx] += phonemes;
      }

      // Categorize terminator
      terminator &= 0x000FFFFF;

      if (terminator == CLAUSE_PERIOD) {
          terminator_str = ".";
      } else if (terminator == CLAUSE_QUESTION) {
          terminator_str = "?";
      } else if (terminator == CLAUSE_EXCLAMATION) {
          terminator_str = "!";
      } else if (terminator == CLAUSE_COMMA) {
          terminator_str = ", ";
      } else if (terminator == CLAUSE_COLON) {
          terminator_str = ": ";
      } else if (terminator == CLAUSE_SEMICOLON) {
          terminator_str = "; ";
      }

      sentence_phonemes[current_idx] += terminator_str;

      if ((terminator & CLAUSE_TYPE_SENTENCE) == CLAUSE_TYPE_SENTENCE) {
          sentence_phonemes.push_back("");
          current_idx = sentence_phonemes.size() - 1;
      }
  }

  // phonemes to ids
  std::vector<PhonemeId> sentence_ids;
  for (auto &phonemes_str : sentence_phonemes) {
      if (phonemes_str.empty()) {
          continue;
      }

      sentence_ids.push_back(ID_BOS);
      sentence_ids.push_back(ID_PAD);

      auto phonemes_norm = una::norm::to_nfd_utf8(phonemes_str);
      auto phonemes_range = una::ranges::utf8_view{phonemes_norm};
      auto phonemes_iter = phonemes_range.begin();
      auto phonemes_end = phonemes_range.end();

      // Filter out (lang) switch (flags).
      // These surround words from languages other than the current voice.
      bool in_lang_flag = false;
      while (phonemes_iter != phonemes_end) {
          if (in_lang_flag) {
              if (*phonemes_iter == U')') {
                  // End of (lang) switch
                  in_lang_flag = false;
              }
          } else if (*phonemes_iter == U'(') {
              // Start of (lang) switch
              in_lang_flag = true;
          } else {
              // Look up ids
              auto ids_for_phoneme =
                  _phoneme_id_map.find(*phonemes_iter);
              if (ids_for_phoneme != _phoneme_id_map.end()) {
                  for (auto id : ids_for_phoneme->second) {
                      sentence_ids.push_back(id);
                      sentence_ids.push_back(ID_PAD);
                  }
              }
          }

          phonemes_iter++;
      }

      sentence_ids.push_back(ID_EOS);

      id_queue.emplace(std::move(sentence_ids));
      sentence_ids.clear();
  }

  return 0;
}

int speech_synth::next(std::vector<PhonemeId> next_ids, std::vector<float>& samples) {

  int num_samples = 0;

  auto memoryInfo = Ort::MemoryInfo::CreateCpu(
      OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

  // Allocate
  std::vector<int64_t> phoneme_id_lengths{(int64_t)next_ids.size()};
  std::vector<float> scales{_noise_scale, _length_scale, _noise_w_scale};

  std::vector<Ort::Value> input_tensors;
  std::vector<int64_t> phoneme_ids_shape{1, (int64_t)next_ids.size()};
  input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
      memoryInfo, next_ids.data(), next_ids.size(), phoneme_ids_shape.data(),
      phoneme_ids_shape.size()));

  std::vector<int64_t> phoneme_id_lengths_shape{
      (int64_t)phoneme_id_lengths.size()};
  input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
      memoryInfo, phoneme_id_lengths.data(), phoneme_id_lengths.size(),
      phoneme_id_lengths_shape.data(), phoneme_id_lengths_shape.size()));

  std::vector<int64_t> scales_shape{(int64_t)scales.size()};
  input_tensors.push_back(Ort::Value::CreateTensor<float>(
      memoryInfo, scales.data(), scales.size(), scales_shape.data(),
      scales_shape.size()));

  // Add speaker id.
  // NOTE: These must be kept outside the "if" below to avoid being
  // deallocated.
  std::vector<int64_t> speaker_id{(int64_t)_speaker_id};
  std::vector<int64_t> speaker_id_shape{(int64_t)speaker_id.size()};

  if (_num_speakers > 1) {
      input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
          memoryInfo, speaker_id.data(), speaker_id.size(),
          speaker_id_shape.data(), speaker_id_shape.size()));
  }

  // From export_onnx.py
  std::array<const char *, 4> input_names = {"input", "input_lengths",
                                             "scales", "sid"};
  std::array<const char *, 1> output_names = {"output"};

  // Infer
  auto output_tensors = _session->Run(
      Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(),
      input_tensors.size(), output_names.data(), output_names.size());

  if ((output_tensors.size() != 1) || (!output_tensors.front().IsTensor())) {
      return -1;
  }

  auto audio_shape =
      output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
  num_samples = audio_shape[audio_shape.size() - 1];

  const float *audio_tensor_data =
      output_tensors.front().GetTensorData<float>();
  samples.reserve(samples.size() + num_samples);
  std::copy(audio_tensor_data, audio_tensor_data + num_samples,
            std::back_inserter(samples));

  // Clean up
  for (std::size_t i = 0; i < output_tensors.size(); i++) {
      Ort::detail::OrtRelease(output_tensors[i].release());
  }

  for (std::size_t i = 0; i < input_tensors.size(); i++) {
      Ort::detail::OrtRelease(input_tensors[i].release());
  }

  return 0;
}
