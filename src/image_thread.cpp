#include "image_thread.h"

#include <opencv2/opencv.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <iostream>
#include <fstream>

extern bool running;

image_thread::image_thread(YAML::Node& config) {
  _camId = config["camera"]["camId"].as<int>();
  _cmd_pending = false;
  _audio_pending = false;
  _speech_pending = false;
  _muted = false;

  _RRzoomin = config["RoboRob"]["zoomin"].as<std::string>();
  _RRzoomout = config["RoboRob"]["zoomout"].as<std::string>();
  _RRnormal = config["RoboRob"]["normal"].as<std::string>();
  _RRedges = config["RoboRob"]["edges"].as<std::string>();
  _RRcontrast = config["RoboRob"]["contrast"].as<std::string>();
  _RRmore = config["RoboRob"]["more"].as<std::string>();
  _RRless = config["RoboRob"]["less"].as<std::string>();
  _RRflip = config["RoboRob"]["flip"].as<std::string>();
  _RRhelp = config["RoboRob"]["help"].as<std::string>();
  _RRspeakervolume = config["RoboRob"]["speakervolume"].as<std::string>();
  _RRspeakervolmessage = config["RoboRob"]["speakervolmessage"].as<std::string>();
  _RRdefaultspeaker = config["RoboRob"]["defaultspeaker"].as<std::string>();
  _RRdefaultmic = config["RoboRob"]["defaultmic"].as<std::string>();
  _RRmicvol = config["RoboRob"]["micvol"].as<std::string>();
  _RRmain = config["RoboRob"]["main"].as<std::string>();

  _RRnote = config["RoboRob"]["note"].as<std::string>();
  _RRnoteadd = config["RoboRob"]["noteadd"].as<std::string>();
  _RRnoteclear = config["RoboRob"]["noteclear"].as<std::string>();
  _RRnoteread = config["RoboRob"]["noteread"].as<std::string>();

  _RRmute = config["RoboRob"]["mute"].as<std::string>();
  _RRunmute = config["RoboRob"]["unmute"].as<std::string>();

  _RRaiActivation = config["audio"]["aiActivationWords"].as<std::vector<std::string>>();
  _RRcmdActivation = config["audio"]["cmdActivationWords"].as<std::vector<std::string>>();
  _RRimageInclusionKeywords = config["openai"]["imageInclusionKeywords"].as<std::vector<std::string>>();

  _RRimagemode = config["Imaging"]["imagemode"].as<int>();
  _RRimagezoom = config["Imaging"]["imagezoom"].as<int>();
  _RRimageedgeno = config["Imaging"]["imageedgeno"].as<int>();
  _RRimagethresh_lev = config["Imaging"]["imagethresh-lev"].as<int>();
  _RRimagethreshmode = config["Imaging"]["imagethreshmode"].as<int>();

  _RRusedebugcamera = config["Imaging"]["usedebugcamera"].as<bool>();
  _RRglassesfullscreen = config["Imaging"]["glassesfullscreen"].as<bool>();
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
    play_audio_file("./samples/camera_not_found.mp3");
    return;
  }

  std::string ourcommand;
  // set default speaker
  system(_RRdefaultspeaker.c_str());
  // set volume out
  ourcommand = _RRspeakervolmessage + _RRspeakervolume;
  system(ourcommand.c_str());
  // set default microphone
  system(_RRdefaultmic.c_str());
  // set mic volume at 100
  system(_RRmicvol.c_str());

if (!_RRusedebugcamera){
  /* THESE WORK ON GLASSES BUT NOT WEBCAM*/
  camera.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
  camera.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
  }
  // create a window to display the images from the webcam
  cv::namedWindow("RoboRob", cv::WINDOW_NORMAL);
if (_RRglassesfullscreen){
   cv::setWindowProperty("RoboRob",cv::WND_PROP_FULLSCREEN,cv::WINDOW_FULLSCREEN);
}

  // array to hold image
  cv::Mat img;
  int mode = _RRimagemode;
  int zoom = _RRimagezoom;
  int edgeno = _RRimageedgeno;
  int thresh_lev = _RRimagethresh_lev;
  int threshmode = _RRimagethreshmode;



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
        //sometimes there is punctuation
        // strip out the puntuaution
       for (int i = 0, len = _cmd_message.size(); i <= len; i++)
       {
          if (ispunct(_cmd_message[i]))
             {
             _cmd_message.erase(i--, 1);
             len = _cmd_message.size();
             }
       }




   //     if (ispunct(_cmd_message[_cmd_message.length()-1])) {
   //       _cmd_message.erase((_cmd_message.length()-1),1);
   //     }

        // now add a single space at the end in case our keyword is there
        _cmd_message += " ";




// these are the only ones to happen when muted


        std::size_t found = _cmd_message.find(_RRunmute);
        if (found!=std::string::npos) {
           {
              gotit=1;
              _muted = false;
              play_audio_file("./samples/mute_off.mp3");
           }
        }

        found = _cmd_message.find(_RRhelp);
        if (found!=std::string::npos) {
          gotit=1;
          found = _cmd_message.find(_RRcmdActivation[1]); // fallback name
          if (found!=std::string::npos) {
             std::stringstream message;
             message << "Your trigger word for the imaging is, " << _RRcmdActivation[0] << ". Your trigger word for chat G P T, is, "<< _RRaiActivation[0] << ". Your trigger for notes is ," << _RRcmdActivation[0] << " ," << _RRnote << ". You can mute using,"<< _RRcmdActivation[0] << " " << _RRmute << " . And unmute using,"<< _RRcmdActivation[0] << " " << _RRunmute <<" ";
             speak_text(message.str());
             }
}


// these only happen when unmuted

if (!_muted)
{

        found = _cmd_message.find(_RRzoomin);
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

        found = _cmd_message.find(_RRmute);
        if (found!=std::string::npos) {
           {
              gotit=1;
              _muted = true;
              play_audio_file("./samples/mute_on.mp3");
           }
        }



        found = _cmd_message.find(_RRnote);
        if (found!=std::string::npos) {
               found = _cmd_message.find(_RRnoteadd);
               if (found!=std::string::npos)
               { // add to file


               // remove the first 3 words
                std::string tostore;
                std::stringstream ss(_cmd_message);
                std::string word;
                ss>>word;
                ss>>word;
                ss>>word;
                // store any words that are remaining
                while (ss>>word) {
                        tostore = tostore + word +" ";
                        }

               // read file contents
               std::ifstream iNotesFile;
               iNotesFile.open("notesfile.txt");
               std::string data;
               getline(iNotesFile,data);
               iNotesFile.close();

               // write the file
               std::ofstream oNotesFile;
               oNotesFile.open("notesfile.txt");

               // capitalise so that the readback is better
               tostore[0]=toupper(tostore[0]);
               data = data +  " . " + tostore ;
               gotit=1;
               oNotesFile << data;
               oNotesFile.close();

               std::stringstream message;
             message << "added note , " << tostore;
             speak_text(message.str());

             }
                found = _cmd_message.find(_RRnoteread);
               if (found!=std::string::npos)
               { // read  file


               std::ifstream iNotesFile;
               iNotesFile.open("notesfile.txt");
               std::string data;

               if (getline(iNotesFile,data)){
                  std::stringstream message;
                  message << "notes are , " << data;
                  speak_text(message.str());
               }
               else{
                   std::stringstream message;
                   message << "Notes, are, empty" ;
                   speak_text(message.str());
               }
               iNotesFile.close();

               gotit=1;


             }
                found = _cmd_message.find(_RRnoteclear);
               if (found!=std::string::npos)
               { // empty  file

               // write file
               std::ofstream oNotesFile;
               oNotesFile.open("notesfile.txt");
               //this makes it empty

               gotit=1;

               oNotesFile.close();

               std::stringstream message;
             message << "cleared notes " ;
             speak_text(message.str());

             }
        }





        found = _cmd_message.find(_RRhelp);
        if (found!=std::string::npos) {
          gotit=1;
          found = _cmd_message.find(_RRcmdActivation[1]); // fallback name
          if (found!=std::string::npos) {
          // moved outside of mute
//             std::stringstream message;
//             message << "Your trigger word for the imaging is, " << _RRcmdActivation[0] << ". Your trigger word for chat G P T is, "<< _RRaiActivation[0] << ". You trigger for notes is " << _RRcmdActivation[0] << " " << _RRnote;
//             speak_text(message.str());
             }
          else{
          found = _cmd_message.find(_RRnote);  // just asking about notes
          if (found!=std::string::npos) {
             std::stringstream message;
             message << "to add something to your notes use," << _RRnote << " ," << _RRnoteadd << ", to clear your notes use, " << _RRnote  << " " << _RRnoteclear << ". To hear your notes use, " << _RRnote << " " << _RRnoteread <<" ";
             speak_text(message.str());
          } else   // not fallback or notes
          {
           found = _cmd_message.find(_RRmain);  // high level help
          if (found!=std::string::npos) {
             std::stringstream message;
             message << " You can use your notes using. " << _RRcmdActivation[0] << " "<< _RRnote << ". You can use chat G P T by saying, " << _RRaiActivation[0] << " . If you say some words like "  << _RRimageInclusionKeywords[0] << " Or," <<_RRimageInclusionKeywords[1] << " Or, " <<_RRimageInclusionKeywords[2] << ", in your chat request, an image from the camera will be sent with your query" << ". You can mute using,"<< _RRcmdActivation[0] << " , " << _RRmute << " . And unmute using,"<< _RRcmdActivation[0] << " " << _RRunmute <<" ,";
             speak_text(message.str());
          }
          else {
          if (mode==1) {
            std::stringstream message;
            message << "In this mode. You can zoom in, by saying, " << _RRzoomin << ", zoom out, by saying, " << _RRzoomout << ". You can choose to display edges by saying, " << _RRedges << ", or to display the contrast mode by saying, " << _RRcontrast <<", ";
            speak_text(message.str());
          }
          else if (mode==2) {
            std::stringstream message;
            message << "In this mode. You can zoom in, by saying, " << _RRzoomin << ", zoom out, by saying, " << _RRzoomout << ". You can increase the edges by saying," << _RRmore << ", or decrease the edges by saying, " << _RRless << ". You can choose to display just the image, by saying, " << _RRnormal << ", or to display the contrast mode by saying, " << _RRcontrast <<", " ;
            speak_text(message.str());
          } else if (mode==3) {
            std::stringstream message;
            message << "In this mode. you can zoom in, by saying " << _RRzoomin << ", zoom out, by saying " << _RRzoomout << ". You can increase the contrast by saying" << _RRmore << ", or decrease the contrast by saying " << _RRless << ". You can invert the colours by saying, " << _RRflip << ". You can choose to display just the image, by saying " << _RRnormal << ", or to display the edges by saying ," << _RRedges <<" ,";
            speak_text(message.str());
          }
          }

       }
}

}

       found = _cmd_message.find("please stop");
       if (found!=std::string::npos) {
          gotit=1;
          running = false;
          exit(0);
       }


       if(!gotit) {
         play_audio_file("./samples/i_didn't_get_that.mp3");
         gotit=0;
       }
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

void image_thread::speak_text(const std::string message) {
  std::unique_lock<std::recursive_mutex> accessLock(_cmd_mutex);
  if(!_speech_pending) {
    _speech_pending = true;
    _speech_message = message;
  }
}

bool image_thread::is_speech_pending(std::string& message) {
  std::unique_lock<std::recursive_mutex> accessLock(_cmd_mutex);
  if(_speech_pending) {
    message = _speech_message;
    _speech_pending = false;
    return true;
  }
  return false;
}

bool image_thread::is_muted() {
  std::unique_lock<std::recursive_mutex> accessLock(_cmd_mutex);
  return _muted;
}
