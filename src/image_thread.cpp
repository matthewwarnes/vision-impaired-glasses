#include "image_thread.h"

#include <iostream>

#include <opencv2/opencv.hpp>

extern bool running;

image_thread::image_thread(YAML::Node& config) {
  _camId = config["camera"]["camId"].as<int>();
	_cmd_pending = false;
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
  cv::VideoCapture camera(_camId, cv::CAP_V4L2);
  if(!camera.isOpened())
  {
    std::cout << "ERROR: can't find camera" << std::endl;
    return;
  }

  /* THESE WORK ON GLASSES BUT NOT WEBCAM*/
  camera.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
  camera.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
  // create a window to display the images from the webcam
  cv::namedWindow("RoboRob", cv::WINDOW_NORMAL);
  //cv::setWindowProperty("RoboRob",cv::WND_PROP_FULLSCREEN,cv::WINDOW_FULLSCREEN);

  // array to hold image
  cv::Mat img;
  signed char key;
  int mode = 1;
  int zoom = 0;
  int edgeno = 50;
  int thresh_lev = 128;
  int threshmode = 0;

  while(_thread_ctrl.load()) {

    // capture the next frame from the webcam
    camera >> img;

    //cv::resize(img, img, cv::Size(1920, 1080));  /* only needed with the test webcam*/
    // zoom the image before any processing
    img = img(cv::Rect(zoom*95, zoom*54, (1920-(zoom*190)), (1080 - (zoom * 108))));

    //make image full size again
    cv::resize(img, img, cv::Size(1920, 1080));

    {
      //take control of mutex to keep thread safe access to currFrame member for ai image requests
      std::unique_lock<std::recursive_mutex> accessLock(_mutex);
      _currFrame = img.clone();
    }

    if (mode == 2) {
      // edge detection
      // Convert to graycsale
      cv::Mat img_gray;
      cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);

      // Blur the image for better edge detection
      cv::Mat img_blur;
      cv::GaussianBlur(img_gray, img_blur, cv::Size(7, 7), 0);

      // Canny edge detection
      cv::Mat edges;
      cv::Canny(img_blur, edges, edgeno, edgeno, 3, false);

      // make edges wider
      cv::Mat wide_edges;
      cv::dilate(edges, wide_edges, cv::Mat(), cv::Point(-1 ,-1), 2, 1, 1);

      cv::Mat col_edges;
      cv::cvtColor(wide_edges, col_edges, cv::COLOR_GRAY2BGR);
      // set edges to yellow
      col_edges.setTo(cv::Scalar(0, 255, 255),col_edges);

      // "overlay" the edges on the colour image
      img = img + col_edges;

    }

    if (mode == 3) {
      cv::Mat img_gray;
      cv::cvtColor(img, img_gray, cv::COLOR_BGR2GRAY);

      cv::threshold(img_gray, img, thresh_lev, 255, threshmode);
      // set colour to yellow
      cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
      img.setTo(cv::Scalar(0, 255, 255),img);
    }

    // show the image on the window
    cv::imshow("RoboRob", img);

    // wait (10ms) for esc key to be pressed to stop
    key = cv::waitKey(10);
    if (key != -1) {
      if ((key == '+')&(zoom < 9)) {
        zoom++;
      }
      if ((key == '-') & (zoom > 0)) {
        zoom--;
      }
      if ((key == 'm') & (mode == 1)) {
        mode = 2;
      }
      else if ((key == 'm') & (mode == 2)) {
        mode = 3;
      }
      else if ((key == 'm') & (mode == 3)) {
        mode = 1;
      }
      if ((key == 46) & (edgeno < 200) & (mode==2)) {
        edgeno=edgeno+10;
      }
      if ((key == 44) & (edgeno > 30) & (mode==2)) {
        edgeno=edgeno-10;
      }
      if ((key == 46) & (thresh_lev < 220) & (mode == 3)) {
        thresh_lev = thresh_lev + 10;
      }
      if ((key == 44) & (thresh_lev > 20) & (mode == 3)) {
        thresh_lev = thresh_lev - 10;
      }
      if ((key == 'z') & (threshmode == 1) & (mode == 3)) {
        threshmode=0;
      }
      else if ((key == 'z') & (threshmode != 1) & (mode == 3)) {
        threshmode = 1;
      }
      //printf("key %d", key);

      if (key == 'q') {
        //exit application
        running = false;
        break;
      }
    }

    //check for control command
    {
      std::unique_lock<std::recursive_mutex> accessLock(_cmd_mutex);
      if(_cmd_pending) {
        std::cout << "New Control Command: " << _cmd_message << std::endl;
        _cmd_pending = false;
      }
    }
  }
}

void image_thread::send_cmd(const std::string cmd) {
  std::unique_lock<std::recursive_mutex> accessLock(_cmd_mutex);
  if(!_cmd_pending) {
    _cmd_pending = true;
    _cmd_message = cmd;
  }
}
