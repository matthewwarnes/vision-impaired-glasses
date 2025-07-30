#ifndef __IMAGE_THREAD_H__
#define __IMAGE_THREAD_H__

#include <thread>
#include <mutex>
#include <atomic>
#include <yaml-cpp/yaml.h>
#include <opencv2/opencv.hpp>


class image_thread {
public:
  image_thread(YAML::Node& config);
  ~image_thread();

  int start();

  void cancel();
  bool is_running();

  int get_current_frame(std::vector<uint8_t>& jpg);

  void send_cmd(const std::string cmd);

private:
  std::thread _thread;
  std::atomic<bool> _thread_ctrl;

  std::recursive_mutex _mutex;
  bool _running;

  int _camId;

  cv::Mat _currFrame;

  std::recursive_mutex _cmd_mutex;
  bool _cmd_pending;
  std::string _cmd_message;

  void thread_handler();

};


#endif
