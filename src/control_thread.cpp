#include "control_thread.h"

#include <iostream>
#include <unistd.h>
#include <spdlog/spdlog.h>

#include "string_utils.h"

control_thread::control_thread(YAML::Node& config, image_thread& it)
  :  _whisp(config["whisper"]), _ai(config["openai"]), _au(config["audio"], _whisp, _thread_ctrl),
    _speech(config["espeak"]), _img_thread(it)
{
  _image_words = config["openai"]["imageInclusionKeywords"].as<std::vector<std::string>>();
  _aiLocalSttOnly = config["audio"]["aiLocalSpeechDetectOnly"].as<bool>();
  _cmdLocalSttOnly = config["audio"]["cmdLocalSpeechDetectOnly"].as<bool>();

  _echo_speech = config["audio"]["echoSpeech"].as<bool>();

  _aiActivation = config["audio"]["aiActivationWord"].as<std::string>();
  _cmdActivation = config["audio"]["cmdActivationWord"].as<std::string>();
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

bool control_thread::requires_image(const std::string message) {
  for(const auto& str: _image_words) {
    if(message.find(str) != std::string::npos) {
      return true;
    }
  }
  return false;
}

void control_thread::thread_handler()
{
  //TODO: remove, this is just an example
  {
    std::vector<uint8_t> spk;
    if(!_speech.convert_text_to_audio("Hello, i am speaking to you", spk)) {
      _au.play_from_mem(spk);
    }
  }

  _au.start();
  std::cout << "Listening for speech..." << std::endl;

  while(_thread_ctrl.load()) {
    //check for audio to playback
    {
      std::string audio_file;
      if(_img_thread.is_audio_pending(audio_file)) {
        _au.play_from_file(audio_file);

        _au.clear_speech_buffer();
        usleep(1000*800);
        std::cout << "Listening for speech..." << std::endl;
      }
    }

    std::vector<uint8_t> speech_data;
    std::string speech_estimated_string;
    int speech_result = _au.check_for_speech(speech_data, speech_estimated_string);
    if(speech_result < 0) {
      std::cerr << "ERROR: failed to capture microphone data" << std::endl;
      return;
    } else if(speech_result == 0) {
      usleep(10);
      continue;
    }

    if(!_thread_ctrl.load()) break;

    if(_echo_speech) {
      _au.play_from_mem(speech_data);
      usleep(1000*800);
    }

    speech_estimated_string = trim_and_lowercase(speech_estimated_string);

    std::cout << "Estimated Text: " << speech_estimated_string << std::endl;

    if(!is_activation(speech_estimated_string)) {
      std::cout << "Listening for speech..." << std::endl;
      continue;
    }

    if(is_cmd_activation(speech_estimated_string)) {
      std::string requestText = speech_estimated_string;
      if(!_cmdLocalSttOnly) {
        if(_ai.convert_audio_to_text(speech_data, requestText)) {
          std::cerr << "ERROR: failed to convert audio to text" << std::endl;
          return;
        }
      }

      //send the string to the image thread
      _img_thread.send_cmd(requestText);
    } else if(is_ai_activation(speech_estimated_string)) {
      std::string requestText = speech_estimated_string;
      if(!_aiLocalSttOnly) {
        if(_ai.convert_audio_to_text(speech_data, requestText)) {
          std::cerr << "ERROR: failed to convert audio to text" << std::endl;
          return;
        }
      }


      if(requestText.size() > 10) {
        std::cout << "Spoken: " << requestText << std::endl;

        std::vector<uint8_t> audio_data;
        if(requires_image(requestText)) {
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
        usleep(1000*800);
      }
    }

    std::cout << "Listening for speech..." << std::endl;
  }
}


audio_wrapper& control_thread::get_audio() {
  return _au;
}

bool control_thread::is_activation(const std::string message) {
  return is_ai_activation(message) || is_cmd_activation(message);
}

bool control_thread::is_ai_activation(const std::string message) {
  if(message.find(_aiActivation) != std::string::npos) {
    return true;
  }
  return false;
}

bool control_thread::is_cmd_activation(const std::string message) {
  if(message.find(_cmdActivation) != std::string::npos) {
    return true;
  }
  return false;
}
