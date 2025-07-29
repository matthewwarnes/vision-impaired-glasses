#include "config/config.h"
#include "openai/ai_wrapper.h"
#include "audio/audio_wrapper.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

int process_args(config::config& conf, int argc, char *argv[])
{
  conf.add_option('c', "config", "yaml config file");

  return conf.parse_args(argc, argv, VIG_VERSION);
}


YAML::Node load_config(std::string config_file) {
  YAML::Node config;
  try {
    config = YAML::LoadFile(config_file);
  } catch (...) {
    std::cerr << "ERROR: failed to read yaml config file" << std::endl;
    exit(EXIT_FAILURE);
    return config;
  }
  return config;
}



int main(int argc, char *argv[]) {

  config::config args;
  if(process_args(args, argc, argv)) return EXIT_SUCCESS;

  YAML::Node config = load_config(args.get_value<std::string>("config"));

  ai_wrapper ai(config["openai"]);
  audio_wrapper audio(config["audio"]);

  std::string request("Tell me a three sentence bedtime story about a unicorn");
  std::cout << "User Request:\t" << request << std::endl;

  std::vector<uint8_t> audio_data;
  if(ai.ai_text_to_audio(request, audio_data)) {
    std::cerr << "ERROR: failed to process request" << std::endl;
    return EXIT_FAILURE;
  }

  if(audio.play_from_mem(audio_data)) {
    std::cerr << "ERROR: failed to output audio data" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
