#include "config/config.h"
#include "control_thread.h"
#include "image_thread.h"

#include <yaml-cpp/yaml.h>

#include <iostream>
#include <unistd.h>
#include <signal.h>

static volatile bool running = true;

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



int main(int argc, char *argv[]) {

  config::config args;
  if(process_args(args, argc, argv)) return EXIT_SUCCESS;

  YAML::Node config = load_config(args.get_value<std::string>("config"));

  image_thread images(config);
  control_thread ctrl(config, images);


  if(images.start()) {
    std::cerr << "ERROR: failed to start image thread" << std::endl;
    return EXIT_FAILURE;
  }

  if(ctrl.start()) {
    std::cerr << "ERROR: failed to start control thread" << std::endl;
    images.cancel();
    return EXIT_FAILURE;
  }

  //setup ctrl-c handling
  signal(SIGINT, intHandler);

  while(running) {
    usleep(10000);
  }

  images.cancel();
  ctrl.cancel();

  return EXIT_SUCCESS;
}
