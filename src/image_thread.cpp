#include "image_thread.h"

#include <iostream>

#include <opencv2/opencv.hpp>

image_thread::image_thread(YAML::Node& config) {
  _camId = config["camera"]["camId"].as<int>();
}

image_thread::~image_thread() {
  cancel();
}

int image_thread::start() {
  cancel();
  _thread_ctrl.store(true);
  _thread = std::thread(&image_thread::thread_handler, this);
  return 0;
}

void image_thread::cancel() {
  _thread_ctrl.store(false);
  if(_thread.joinable())
    _thread.join();
}

bool image_thread::is_running() {
  std::unique_lock<std::recursive_mutex> accessLock(_mutex);
  return _running;
}

int image_thread::get_current_frame(std::vector<uint8_t>& jpg) {
  std::unique_lock<std::recursive_mutex> accessLock(_mutex);
  //encode currFrame to jpg
  jpg.clear();
  //jpg.reserve(1024*1024*4); //reserve excessive buffer
  cv::imencode(".jpg", _currFrame, jpg);
  return 0;
}

void image_thread::thread_handler() {
	cv::VideoCapture cap(_camId, cv::CAP_V4L2);
  if(!cap.isOpened())
  {
    std::cout << "ERROR: can't find camera" << std::endl;
    return;
  }

  while(_thread_ctrl.load()){
    {
      //take control of mutex to keep thread safe access to image
      std::unique_lock<std::recursive_mutex> accessLock(_mutex);
      if(cap.read(_currFrame)) {
        imshow("", _currFrame);
      }
    }

    cv::waitKey(33);

    //check for control command

  }
}
