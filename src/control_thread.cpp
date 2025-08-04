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
  bool audio_played = true;

  while(_thread_ctrl.load()) {

    //check for audio to playback
    {
      std::string audio_file;
      if(_img_thread.is_audio_pending(audio_file)) {
        _au.play_from_file(audio_file);
        _au.clear_speech_buffer();
        audio_played = true;
      }
    }

    //if on previous loop audio was output, add a delay to prevent reprocessing the audio we output
    if(audio_played)  {
      audio_played = false;
      usleep(1000*1000);
      spdlog::info("Listening for speech...");
    }


    std::vector<uint8_t> speech_data;
    std::string speech_estimated_string;
    int speech_result = _au.check_for_speech(speech_data, speech_estimated_string);
    if(speech_result < 0) {
      spdlog::error("Failed to capture microphone data");
      return;
    } else if(speech_result == 0) {
      usleep(10);
      continue;
    }

    //if ctrl-c received, close the thread
    if(!_thread_ctrl.load()) break;

    if(_echo_speech) {
      _au.play_from_mem(speech_data);
      audio_played = true;
    }

    speech_estimated_string = trim_and_lowercase(speech_estimated_string);

    std::cout << "Estimated Text: " << speech_estimated_string << std::endl;
    if(!is_activation(speech_estimated_string)) {
      _au.play_from_file("./samples/beep2.mp3");
      audio_played = true;

    } else if(is_cmd_activation(speech_estimated_string)) {

      std::string requestText = speech_estimated_string;
      if(!_cmdLocalSttOnly) {
        if(_ai.convert_audio_to_text(speech_data, requestText)) {
          //AI ERROR
          _au.play_from_file("./samples/no_internet.mp3");
          continue;
        }
      }

      //send the string to the image thread
      _img_thread.send_cmd(requestText);

    } else if(is_ai_activation(speech_estimated_string)) {

      _au.play_from_file("./samples/chat.mp3");
      audio_played = true;

      std::string requestText = speech_estimated_string;
      if(!_aiLocalSttOnly) {
        if(_ai.convert_audio_to_text(speech_data, requestText)) {
          //AI ERROR
          _au.play_from_file("./samples/no_internet.mp3");
          continue;
        }
      }


      if(requestText.size() > 10) {
        spdlog::info("Asking AI: {}", requestText);

        std::vector<uint8_t> audio_data;
        if(requires_image(requestText)) {
          _au.play_from_file("./samples/camera-shutter.mp3");

          //word image in request to send with current camera frame
          std::vector<uint8_t> img;
          if(_img_thread.get_current_frame(img)) {
            continue;
          }

          std::string responseText;
          _au.play_from_file("./samples/please_wait.mp3");
          if(_ai.ai_text_image_to_text(requestText, img, responseText)) {
            //AI ERROR
            _au.play_from_file("./samples/no_internet.mp3");
            continue;
          }
          if(_ai.convert_text_to_audio(responseText, audio_data)) {
            //AI ERROR
            _au.play_from_file("./samples/no_internet.mp3");
            continue;
          }
        } else {
         _au.play_from_file("./samples/please_wait.mp3");
          //normal ai request without sending image
          if(_ai.ai_text_to_audio(requestText, audio_data)) {
            //AI ERROR
            _au.play_from_file("./samples/no_internet.mp3");
            continue;
          }
        }

        if(!_thread_ctrl.load()) break;

        if(_au.play_from_mem(audio_data)) {
          spdlog::error("Failed to output audio data");
          continue;
        }
      }
    }
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
