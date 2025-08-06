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

  bool is_audio_pending(std::string& file);
  bool is_speech_pending(std::string& message);

  bool is_muted();

private:
  std::thread _thread;
  std::atomic<bool> _thread_ctrl;

  std::recursive_mutex _mutex;
  bool _running;

  int _camId;
  std::string _RRzoomin;
  std::string _RRzoomout;
  std::string _RRedges;
  std::string _RRnormal;
  std::string _RRcontrast;
  std::string _RRmore;
  std::string _RRless;
  std::string _RRflip;
  std::string _RRhelp;
  std::string _RRmain;
  
  std::string _RRmute;
  std::string _RRunmute; 

  std::string _RRnote;
  std::string _RRnoteadd;
  std::string _RRnoteclear;
  std::string _RRnoteread;

  std::string _RRspeakervolume;
  std::string _RRspeakervolmessage;
  std::string _RRdefaultspeaker;
  std::string _RRdefaultmic;
  std::string _RRmicvol;
  
  std::vector<std::string> _RRaiActivation;
  std::vector<std::string> _RRcmdActivation;
  std::vector<std::string> _RRimageInclusionKeywords;
  
  int _RRimagemode;
  int _RRimagezoom;
  int _RRimageedgeno;
  int _RRimagethresh_lev;
  int _RRimagethreshmode;
  
  bool _RRusedebugcamera;
  bool _RRglassesfullscreen;
  
  cv::Mat _currFrame;

  std::recursive_mutex _cmd_mutex;
  bool _cmd_pending;
  std::string _cmd_message;

  bool _audio_pending;
  std::string _audio_file;

  bool _speech_pending;
  std::string _speech_message;


  bool _muted;

  void thread_handler();


  void play_audio_file(const std::string file);
  void speak_text(const std::string message);

};


#endif
