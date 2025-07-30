#include "control_thread.h"

#include <iostream>

control_thread::control_thread(YAML::Node& config, image_thread& it) : _ai(config["openai"]), _au(config["audio"]), _img_thread(it)
{
}


control_thread::~control_thread() {
	cancel();
}

int control_thread::start() {

	cancel();
	_thread_ctrl.store(true);
  _thread = std::thread(&control_thread::thread_handler, this);
	return 0;
}

void control_thread::cancel()
{
	_thread_ctrl.store(false);
	if(_thread.joinable())
		_thread.join();
}

bool control_thread::is_running()
{
	std::unique_lock<std::recursive_mutex> accessLock(_mutex);
	return _running;
}

void control_thread::thread_handler()
{
  while(_thread_ctrl.load()) {

    std::vector<uint8_t> mic_data;
    if(_au.capture_audio(4, mic_data)) {
      std::cerr << "ERROR: failed to capture microphone data" << std::endl;
      return;
    }
    if(!_thread_ctrl.load()) break;

    std::string requestText;
    if(_ai.convert_audio_to_text(mic_data, requestText)) {
      std::cerr << "ERROR: failed to convert audio to text" << std::endl;
      return;
    }

    if(requestText.size() > 10) {
      std::cout << "Spoken: " << requestText << std::endl;

      std::vector<uint8_t> audio_data;
      if(requestText.find("image") != std::string::npos) {
        //word image in request to send with current camera frame
        std::vector<uint8_t> img;
        if(_img_thread.get_current_frame(img)) {
          continue;
        }
        std::string responseText;
        if(_ai.ai_text_image_to_text(requestText, img, responseText)) {
          std::cerr << "ERROR: failed to process request" << std::endl;
          return;
        }
        if(_ai.convert_text_to_audio(responseText, audio_data)) {
          std::cerr << "ERROR: failed to convert text to audio" << std::endl;
          return;
        }
      } else {
        //normal ai request without sending image
        if(_ai.ai_text_to_audio(requestText, audio_data)) {
          std::cerr << "ERROR: failed to process request" << std::endl;
          return;
        }
      }

      if(!_thread_ctrl.load()) break;

      if(_au.play_from_mem(audio_data)) {
        std::cerr << "ERROR: failed to output audio data" << std::endl;
        return;
      }
    }

  }
}
