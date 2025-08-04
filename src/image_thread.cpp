#include "image_thread.h"

#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>

extern bool running;

image_thread::image_thread(YAML::Node& config) {
  _camId = config["camera"]["camId"].as<int>();
  _cmd_pending = false;
  _audio_pending = false;

  _RRzoomin = config["RoboRob"]["zoomin"].as<std::string>();
  _RRzoomout = config["RoboRob"]["zoomout"].as<std::string>();
  _RRnormal = config["RoboRob"]["normal"].as<std::string>();
  _RRedges = config["RoboRob"]["edges"].as<std::string>();
  _RRcontrast = config["RoboRob"]["contrast"].as<std::string>();
  _RRmore = config["RoboRob"]["more"].as<std::string>();
  _RRless = config["RoboRob"]["less"].as<std::string>();
  _RRflip = config["RoboRob"]["flip"].as<std::string>();
  _RRhelp = config["RoboRob"]["help"].as<std::string>();
  _RRvolume = config["RoboRob"]["volume"].as<std::string>();


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
    spdlog::error("Can't find camera");
    return;
  }

  // set volume out
  std::string turnitdown;
  turnitdown = "amixer -D pulse sset Master " + _RRvolume;

  system(turnitdown.c_str());

  /* THESE WORK ON GLASSES BUT NOT WEBCAM*/
  camera.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
  camera.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
  // create a window to display the images from the webcam
  cv::namedWindow("RoboRob", cv::WINDOW_NORMAL);
//  cv::setWindowProperty("RoboRob",cv::WND_PROP_FULLSCREEN,cv::WINDOW_FULLSCREEN);

  // array to hold image
  cv::Mat img;
  int mode = 1;
  int zoom = 0;
  int edgeno = 50;
  int thresh_lev = 128;
  int threshmode = 0;

  while(_thread_ctrl.load()) {

    // capture the next frame from the webcam
    camera >> img;

//   cv::resize(img, img, cv::Size(1920, 1080));  /* only needed with the test webcam*/
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

    // wait (10ms) for esc key to be pressed to stop // need this to let the display happen?

    cv::waitKey(10);

    bool gotit = 0;
    //check for control command
    {
      std::unique_lock<std::recursive_mutex> accessLock(_cmd_mutex);
      if(_cmd_pending) {
        // to use .find but not get a match on part of a word we have to have space/word/space
        //sometimes there is a full stop at the end
        // remove full stop if there
        if (ispunct(_cmd_message[_cmd_message.length()-1])) {
          _cmd_message.erase((_cmd_message.length()-1),1);
        }

        // now add a single space at the end in case our keyword is there
        _cmd_message += " ";


        std::size_t found = _cmd_message.find(_RRzoomin);
        if (found!=std::string::npos) {
          gotit=1;
          if (zoom < 9) {
            play_audio_file("./samples/zoom_in.mp3");
            zoom++;
          } else {
            play_audio_file("./samples/zoom_limit.mp3");
          }
        }

        found = _cmd_message.find(_RRzoomout);
        if (found!=std::string::npos) {
          gotit=1;
          if (zoom > 0) {
            zoom--;
            play_audio_file("./samples/zoom_out.mp3");
          } else {
            play_audio_file("./samples/zoom_limit.mp3");
          }
        }

        found = _cmd_message.find(_RRedges);
        if (found!=std::string::npos) {
          if (mode!=2) {
            gotit=1;
            mode=2;
            play_audio_file("./samples/edges.mp3");
          }
        }

        found = _cmd_message.find(_RRnormal);
        if (found!=std::string::npos) {
          if (mode!=1) {
            gotit=1;
            mode=1;
            play_audio_file("./samples/normal.mp3");
          }
        }

        found = _cmd_message.find(_RRcontrast);
        if (found!=std::string::npos) {
          if (mode!=3) {
            gotit=1;
            mode=3;
            play_audio_file("./samples/contrast.mp3");
          }
        }

        found = _cmd_message.find(_RRless);
        if (found!=std::string::npos) {
          gotit=1;
          if(mode==2) {
            if (edgeno < 200) {
              edgeno=edgeno+40;
              play_audio_file("./samples/less.mp3");
            } else {
              play_audio_file("./samples/edge_limit.mp3");
            }
          } else if (mode==3) {
            if (thresh_lev < 220) {
              thresh_lev = thresh_lev + 10;
              play_audio_file("./samples/less.mp3");
            } else{
              play_audio_file("./samples/contrast_limit.mp3");
            }
          } else {
            play_audio_file("./samples/not_allowed.mp3");
          }
        }

        found = _cmd_message.find(_RRmore);
        if (found!=std::string::npos) {
          gotit=1;
          if (mode==2){
            if(edgeno > 40) {
              edgeno=edgeno-10;
              play_audio_file("./samples/more.mp3");
            } else {
              play_audio_file("./samples/edge_limit.mp3");
            }
          } else if (mode==3) {
            if (thresh_lev > 20) {
              thresh_lev = thresh_lev - 10;
              play_audio_file("./samples/more.mp3");
            } else {
              play_audio_file("./samples/contrast_limit.mp3");
            }
          } else {
            play_audio_file("./samples/not_allowed.mp3");
          }
        }

        found = _cmd_message.find(_RRflip);
        if (found!=std::string::npos) {
          if(mode==3) {
            gotit=1;
            if(threshmode!=1) {
              threshmode=1;
              play_audio_file("./samples/flip.mp3");
            } else if (threshmode==1) {
              threshmode=0;
              play_audio_file("./samples/flip.mp3");
            }
          } else {
            play_audio_file("./samples/not_allowed.mp3");
          }
        }

        found = _cmd_message.find(_RRhelp);
        if ((found!=std::string::npos)&(mode==1)) {
          gotit=1;
          play_audio_file("./samples/help.mp3");
        }

        if(!gotit) {
          play_audio_file("./samples/i_didn't_get_that.mp3");
          gotit=0;
        }


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

void image_thread::play_audio_file(const std::string file) {
  std::unique_lock<std::recursive_mutex> accessLock(_cmd_mutex);
  if(!_audio_pending) {
    _audio_pending = true;
    _audio_file = file;
  }
}

bool image_thread::is_audio_pending(std::string& file) {
  std::unique_lock<std::recursive_mutex> accessLock(_cmd_mutex);
  if(_audio_pending) {
    file = _audio_file;
    _audio_pending = false;
    return true;
  }
  return false;
}
