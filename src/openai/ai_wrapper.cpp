#include "ai_wrapper.h"

#include <fstream>
#include <sstream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <turbob64.h>

#include <spdlog/spdlog.h>

using json = nlohmann::json;
//#include <liboai.h>

static const std::string responsesApiURL = "https://api.openai.com/v1/chat/completions";
static const std::string transcribeApiURL = "https://api.openai.com/v1/audio/transcriptions";
static const std::string ttsApiURL = "https://api.openai.com/v1/audio/speech";

size_t b64_encoded_length(const size_t binaryLen) {
  double tmp = ((double)binaryLen) / 3;
  tmp = ceil(tmp);
  tmp *= 4;
  return tmp;
}

ai_wrapper::ai_wrapper(YAML::Node config) {
  try {
    std::ifstream kf(config["keyFile"].as<std::string>());
    if(!kf) {
      spdlog::error("failed to open openai key file");
      exit(EXIT_FAILURE);
    }
    std::getline(kf, _key);
  } catch(...) {
    spdlog::error("failed to open openai key file");
    exit(EXIT_FAILURE);
  }
  _model = config["model"].as<std::string>();
  _transcribe_model = config["transcribeModel"].as<std::string>();
  _image_model = config["imageModel"].as<std::string>();
  _tts_model = config["ttsModel"].as<std::string>();
  _voice = config["voice"].as<std::string>();
}

int ai_wrapper::ai_text_to_text(const std::string request, std::string& response) {
  json data = {
    {"model", _model},
    {"messages", {{{"role", "user"}, {"content", request}}}}
  };

  cpr::Response r = cpr::Post(cpr::Url{responsesApiURL},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Bearer{_key},
            cpr::Body{data.dump()}
            );
  if(r.status_code != 200) {
    spdlog::error("Bad HTTP Status Code - {}", r.status_code);
    //std::cerr << r.text << std::endl;
    return -1;
  } else {
    try {
      json responseJson = json::parse(r.text);
      response = responseJson["choices"][0]["message"]["content"];
    } catch(...) {
      spdlog::error("failed to parse openai response");
      return -2;
    }
  }

  return 0;
}


int ai_wrapper::ai_text_to_audio(const std::string request, std::vector<uint8_t>& output) {
  json data = {
    {"model", _model},
    {"modalities", {"text", "audio"}},
    {"audio", {{"voice", _voice}, {"format", "mp3"}}},
    {"messages", {{{"role", "user"}, {"content", request}}}}
  };

  cpr::Response r = cpr::Post(cpr::Url{responsesApiURL},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Bearer{_key},
            cpr::Body{data.dump()}
            );
  if(r.status_code != 200) {
    spdlog::error("Bad HTTP Status Code - {}", r.status_code);
    //std::cerr << r.text << std::endl;
    return -1;
  } else {
    //std::cout << "RESPONSE: " << r.text << std::endl;
    std::string audio_str;
    try {
      json responseJson = json::parse(r.text);
      audio_str = responseJson["choices"][0]["message"]["audio"]["data"];
    } catch(...) {
      spdlog::error("failed to parse openai response");
      return -2;
    }

    //decode base64 to binary
    output.clear();
    output.resize(audio_str.size());
    uint32_t out_len = tb64dec((uint8_t*)audio_str.c_str(), audio_str.size(), output.data());
    output.resize(out_len);

    if(!output.size()) {
      spdlog::error("failed to decode audio datae");
      return -4;
    }
    //std::cout << "AUDIO: " << audio[0] << std::endl;
  }

  return 0;
}

int ai_wrapper::ai_text_image_to_text(const std::string request, const std::vector<uint8_t>& img, std::string& response) {

  //convert image to base64 string for embedded in request
  std::vector<char> b64_arr(b64_encoded_length(img.size()));
  size_t len = tb64enc(img.data(), img.size(), (uint8_t*)b64_arr.data());
  b64_arr.resize(len);
  std::string b64_image(b64_arr.begin(), b64_arr.end());

  json data = {
    {"model", _image_model},
    {"messages", {
      {
        {"role", "user"},
        {"content", {
          { {"type", "text"}, {"text", request} },
          { {"type", "image_url"}, {"image_url", { {"url", "data:image/jpeg;base64," + b64_image } } } }
        }}
      }
    }}
  };

  cpr::Response r = cpr::Post(cpr::Url{responsesApiURL},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Bearer{_key},
            cpr::Body{data.dump()}
            );
  if(r.status_code != 200) {
    spdlog::error("Bad HTTP Status Code - {}", r.status_code);
    //std::cerr << r.text << std::endl;
    return -1;
  } else {
    std::string audio_str;
    try {
      json responseJson = json::parse(r.text);
      response = responseJson["choices"][0]["message"]["content"];
    } catch(...) {
      spdlog::error("failed to parse openai response");
      return -2;
    }
  }

  return 0;
}

int ai_wrapper::convert_text_to_audio(const std::string input, std::vector<uint8_t>& output)
{
  //take a string and convert to speech using openai

  json data = {
    {"model", _tts_model},
    {"voice", _voice},
    {"instructions", "speak in a calm and friendly tone"},
    {"input", input}
  };

  cpr::Response r = cpr::Post(cpr::Url{ttsApiURL},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Bearer{_key},
            cpr::Body{data.dump()}
            );
  if(r.status_code != 200) {
    spdlog::error("Bad HTTP Status Code - {}", r.status_code);
    //std::cerr << r.text << std::endl;
    return -1;
  } else {
    output.clear();
    std::copy(r.text.begin(), r.text.end(), std::back_inserter(output));
  }

  return 0;
}

int ai_wrapper::convert_audio_to_text(const std::vector<uint8_t>& wavData, std::string &text)
{
  cpr::Response r = cpr::Post(cpr::Url{transcribeApiURL},
            cpr::Bearer{_key},
            cpr::Multipart{
              {"model", _transcribe_model},
              {"language", "en"},
              {"file", cpr::Buffer{wavData.begin(), wavData.end(), "speech.wav"}}
            });
  if(r.status_code != 200) {
    spdlog::error("Bad HTTP Status Code - {}", r.status_code);
    //std::cerr << r.text << std::endl;
    return -1;
  } else {
    //std::cout << "RESPONSE: " << r.text << std::endl;
    try {
      json responseJson = json::parse(r.text);
      text = responseJson["text"];
    } catch(...) {
      spdlog::error("failed to parse openai transcription response");
      return -2;
    }
  }
  return 0;
}
