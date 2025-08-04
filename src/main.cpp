#include "config/config.h"
#include "control_thread.h"
#include "image_thread.h"

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <vector>

const size_t max_log_file_size = 1048576; //1B

volatile bool running = true;

void intHandler(int dummy) {
    running = false;
}

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

void setup_logging(YAML::Node config) {

  std::vector<spdlog::sink_ptr> sinks;

  if(config["logToConsole"].as<bool>()) {
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.back()->set_level(spdlog::level::info);
    sinks.back()->set_pattern("[%^%l%$] %v");
  }

  if(config["logToFile"].as<bool>()) {
    size_t max_files = 1;
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink<std::mutex>>(config["logFile"].as<std::string>(), max_log_file_size, max_files));
    sinks.back()->set_level(spdlog::level::debug);
    sinks.back()->set_pattern("[%^%l%$] %v");
  }

  auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());
  logger->set_level(spdlog::level::debug);

  spdlog::set_default_logger(logger);
}


int main(int argc, char *argv[]) {

  config::config args;
  if(process_args(args, argc, argv)) return EXIT_SUCCESS;

  YAML::Node config = load_config(args.get_value<std::string>("config"));

  setup_logging(config["logging"]);

  image_thread images(config);
  control_thread ctrl(config, images);

  if(images.start()) {
    spdlog::error("failed to start image thread");
    return EXIT_FAILURE;
  }

  if(ctrl.start()) {
    spdlog::error("failed to start control thread");
    images.cancel();
    return EXIT_FAILURE;
  }

  //setup ctrl-c handling
  signal(SIGINT, intHandler);

  while(running) {
    usleep(1000);
  }

  images.cancel();
  ctrl.cancel();

  return EXIT_SUCCESS;
}
