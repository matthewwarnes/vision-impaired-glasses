#include "ai_wrapper.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <turbob64.h>

using json = nlohmann::json;
//#include <liboai.h>

static const std::string responsesApiURL = "https://api.openai.com/v1/chat/completions";

ai_wrapper::ai_wrapper(YAML::Node config) {
  try {
    std::ifstream kf(config["keyFile"].as<std::string>());
    if(!kf) {
      std::cerr << "ERROR: failed to open openai key file" << std::endl;
      exit(EXIT_FAILURE);
    }
    std::getline(kf, _key);
  } catch(...) {
    std::cerr << "ERROR: failed to open openai key file" << std::endl;
    exit(EXIT_FAILURE);
  }
  _model = config["model"].as<std::string>();
}

int ai_wrapper::ai_text_to_text(std::string request, std::string& response) {
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
    std::cerr << "ERROR: Status code - " << r.status_code << ": " << std::endl;
  	std::cerr << r.text << std::endl;
    return -1;
  } else {
  	if(r.text != "{}") {
      //std::cout << "RESPONSE: " << r.text << std::endl;
      try {
        json responseJson = json::parse(r.text);
        response = responseJson["choices"][0]["message"]["content"];
      } catch(...) {
        std::cerr << "ERROR: failed to parse openai response" << std::endl;
        return -2;
      }
  	} else {
      std::cerr << "ERROR: Empty response" << std::endl;
      return -3;
  	}
  }

  return 0;
}


int ai_wrapper::ai_text_to_audio(std::string request, std::vector<uint8_t>& output) {
  json data = {
    {"model", _model},
    {"modalities", {"text", "audio"}},
    {"audio", {{"voice", "alloy"}, {"format", "wav"}}},
    {"messages", {{{"role", "user"}, {"content", request}}}}
  };

  cpr::Response r = cpr::Post(cpr::Url{responsesApiURL},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Bearer{_key},
            cpr::Body{data.dump()}
            );
  if(r.status_code != 200) {
    std::cerr << "ERROR: Status code - " << r.status_code << ": " << std::endl;
  	std::cerr << r.text << std::endl;
    return -1;
  } else {
  	if(r.text != "{}") {
      //std::cout << "RESPONSE: " << r.text << std::endl;
      std::string audio_str;
      try {
        json responseJson = json::parse(r.text);
        audio_str = responseJson["choices"][0]["message"]["audio"]["data"];
      } catch(...) {
        std::cerr << "ERROR: failed to parse openai response" << std::endl;
        return -2;
      }

      //decode base64 to binary
      output.clear();
      output.resize(audio_str.size());
      uint32_t out_len = tb64dec((uint8_t*)audio_str.c_str(), audio_str.size(), output.data());
      output.resize(out_len);

      if(!output.size()) {
        std::cerr << "ERROR: failed to decode audio data" << std::endl;
        return -4;
      }
      //std::cout << "AUDIO: " << audio[0] << std::endl;
  	} else {
      std::cerr << "ERROR: Empty response" << std::endl;
      return -3;
  	}
  }

  return 0;
}

int ai_wrapper::ai_audio_to_audio()
{
  return 0;
}

int ai_wrapper::convert_text_to_audio(std::string input)
{
  return 0;
}

int ai_wrapper::convert_audio_to_text(std::string &text)
{
  text = "HELLO";
  return 0;
}
