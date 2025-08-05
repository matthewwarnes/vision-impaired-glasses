#include "audio_wrapper.h"

#include <SDL_mixer.h>
#include <SDL.h>

#include <iostream>
#include <unistd.h>
#include <cstring>
#include <fstream>

#include <chrono>

#include <spdlog/spdlog.h>

#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;

#define SAMPLES_PER_BUFFER 512



typedef struct {
  /* RIFF Chunk Descriptor */
  uint8_t RIFF[4] = {'R', 'I', 'F', 'F'}; // RIFF Header Magic header
  uint32_t ChunkSize;                     // RIFF Chunk Size
  uint8_t WAVE[4] = {'W', 'A', 'V', 'E'}; // WAVE Header
  /* "fmt" sub-chunk */
  uint8_t fmt[4] = {'f', 'm', 't', ' '}; // FMT header
  uint32_t Subchunk1Size = 16;           // Size of the fmt chunk
  uint16_t AudioFormat = 3; // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
                            // Mu-Law, 258=IBM A-Law, 259=ADPCM
  uint16_t NumOfChan = 1;   // Number of channels 1=Mono 2=Sterio
  uint32_t SamplesPerSec = 0;   // Sampling Frequency in Hz
  uint32_t bytesPerSec = 0; // bytes per second
  uint16_t blockAlign = 2;          // 2=16-bit mono, 4=16-bit stereo
  uint16_t bitsPerSample = sizeof(SAMPLE)*8; // Number of bits per sample
  /* "data" sub-chunk */
  uint8_t Subchunk2ID[4] = {'d', 'a', 't', 'a'}; // "data"  string
  uint32_t Subchunk2Size;                        // Sampled data length
} wav_hdr_t;



audio_wrapper::audio_wrapper(YAML::Node config, whisper_wrapper& w, std::atomic<bool>& cancel) : _thread_ctrl(cancel), _whisp(w) {
  _stream = nullptr;

  _mic_dev = config["device"].as<std::string>();
  _samples_per_second = config["samplesPerSec"].as<uint32_t>();
  _samples_per_check = config["samplesPerCheck"].as<uint32_t>();


  if(SDL_Init(SDL_INIT_AUDIO) == -1) {
    spdlog::error("failed to initialise SDL: {}", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1) {
    spdlog::error("failed to initialise SDL Mixer: {}", Mix_GetError());
    exit(EXIT_FAILURE);
  }

  PaError err = Pa_Initialize();
  if( err != paNoError ) {
    spdlog::error("failed to initialise portaudio: {}",  Pa_GetErrorText(err));
    exit(EXIT_FAILURE);
  }

}

audio_wrapper::~audio_wrapper() {

  if(_stream != nullptr) {
    _thread_ctrl.store(false);
    PaError err;

    while( ( err = Pa_IsStreamActive( _stream ) ) == 1 ) {
      Pa_Sleep(10);
    }
    if( err == paNoError ) {
      Pa_CloseStream( _stream );
      _stream = nullptr;
    }
  }

  Pa_Terminate();
  Mix_CloseAudio();
  SDL_Quit();
}

int audio_wrapper::start() {
  int id = find_device_id(_mic_dev);
  if(id == -1) {
    spdlog::error("failed to fine microphone device");
    return -1;
  }

  PaStreamParameters  inputParameters;
  inputParameters.device = id;
  inputParameters.channelCount = 1;
  inputParameters.sampleFormat = PA_SAMPLE_TYPE;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  PaError err;

  err = Pa_OpenStream(
            &_stream,
            &inputParameters,
            NULL,
            _samples_per_second,
            SAMPLES_PER_BUFFER,
            0,
            recordCallbackStatic,
            this );
  if( err != paNoError ) {
    spdlog::error("failed to open audio stream: {}", Pa_GetErrorText(err));
    return -2;
  }

  err = Pa_StartStream( _stream );
  if( err != paNoError ) {
    spdlog::error("failed to start audio stream: {}", Pa_GetErrorText(err));
    return -3;
  }

  return 0;
}

int audio_wrapper::play_from_mem(std::vector<uint8_t>& audio_arr) {

  SDL_RWops* rw = SDL_RWFromMem(audio_arr.data(), audio_arr.size());
  Mix_Music* audio =  Mix_LoadMUS_RW(rw,1);
  if(!audio) {
    spdlog::error("failed to load audio data: {}", Mix_GetError());
    return -1;
  }

  if(Mix_PlayMusic(audio, 1)) {
    spdlog::error("failed to play music: {}", Mix_GetError());
    return -2;
  }

  while(1) {
    if(!Mix_PlayingMusic()) {
      break;
    }
    usleep(1000);
  }

  Mix_FreeMusic(audio);
  audio = NULL;
  return 0;
}

int audio_wrapper::play_from_file(const std::string filename) {
  spdlog::info("Playing audio file: {}", filename);
  Mix_Music *audio_file = NULL;
  audio_file = Mix_LoadMUS(filename.c_str());
  if(!audio_file) {
    spdlog::error("failed to load audio file: {}", Mix_GetError());
    return -1;
  }

  if(Mix_PlayMusic(audio_file, 1)) {
    spdlog::error("failed to play audio file: {}", Mix_GetError());
    return -2;
  }

  while(1) {
    if(!Mix_PlayingMusic()) {
      break;
    }
    usleep(1000);
  }

  Mix_FreeMusic(audio_file);
  audio_file = NULL;
  return 0;
}

void audio_wrapper::list_mics() const {
  int numDevices = Pa_GetDeviceCount();
  for(int i = 0; i < numDevices; i++) {
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
    std::cout << "Mic Device:\t" << deviceInfo->name << std::endl;
  }
}

int audio_wrapper::find_device_id(std::string device_name) const {
  int numDevices = Pa_GetDeviceCount();
  for(int i = 0; i < numDevices; i++) {
    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
    if(device_name == deviceInfo->name) {
      return i;
    }
  }

  return -1;
}

int audio_wrapper::recordCallbackStatic( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
  if ( userData == nullptr) {
    return paComplete;
  }

  (void) outputBuffer; /* Prevent unused variable warnings. */
  (void) timeInfo;

  audio_wrapper* objPtr = static_cast<audio_wrapper*>(userData);
  return objPtr->recordCallback(inputBuffer, framesPerBuffer, statusFlags);
}


int audio_wrapper::recordCallback( const void *inputBuffer, unsigned long framesPerBuffer,
                           PaStreamCallbackFlags statusFlags)
{
  SAMPLE *in = (SAMPLE*)inputBuffer;

  if(!_thread_ctrl.load()) {
    //finish stream if signal for close is sent
    return paComplete;
  }

  if( inputBuffer != NULL ) {
    //add data to audio_buffer
    for(size_t i = 0; i < framesPerBuffer; i++) {
      _audio_buffer.push_back(in[i]);
    }

    if (_audio_buffer.size() >= _samples_per_check) {
      std::unique_lock<std::recursive_mutex> accessLock(_audio_mutex);
      _audio_to_check.clear();
      _audio_to_check.swap(_audio_buffer);
      _audio_pending = true;
    }

  }

  return paContinue;
}

int audio_wrapper::check_for_speech(std::vector<uint8_t>& speech, std::string& estimated_text, bool muted)
{
  bool speech_to_process = false;

  {
    std::unique_lock<std::recursive_mutex> accessLock(_audio_mutex);
    if(_audio_pending) {
      _audio_pending = false;
      //we have audio in buffer, check it for speech

      int vad_result = _whisp.contains_speech(_audio_to_check);
      if(vad_result < 0) {
        return -2;
      } else if (vad_result > 0) {

        //speech found
        if(!_speech_segment.size()) {
          _speech_segment.swap(_pre_speech);
        }
        std::copy(_audio_to_check.begin(), _audio_to_check.end(), std::back_inserter(_speech_segment));
      } else {
        //no speech
        if(_speech_segment.size()) {
          //end of speech (there was previous speech found)

          //add additional background to end of speech
          std::copy(_audio_to_check.begin(), _audio_to_check.end(), std::back_inserter(_speech_segment));

          //copy speech segments into output wav format
          size_t numBytes = _speech_segment.size() * sizeof(float);
          speech.clear();
          speech.resize(numBytes + sizeof(wav_hdr_t));
          {
            wav_hdr_t wav;
            wav.ChunkSize = numBytes + 36;
            wav.Subchunk2Size = numBytes;
            wav.SamplesPerSec = _samples_per_second;   // Sampling Frequency in Hz
            wav.bytesPerSec = _samples_per_second * sizeof(SAMPLE); // bytes per second
            uint8_t *p = (uint8_t *)&wav;
            for(size_t i = 0; i < sizeof(wav_hdr_t); i++) {
              speech[i] = p[i];
            }
          }
          std::memcpy(&speech[sizeof(wav_hdr_t)], (uint8_t*)_speech_segment.data(), numBytes);
          speech_to_process = true;
          if(!muted) play_from_file("./samples/beep_short.mp3");
        } else {
          //no speech and previous also wasn't speech

          //fill pre speech ready to be added before speech
          _pre_speech.swap(_audio_to_check);
        }
      }
    }
  }

  if(speech_to_process) {
    spdlog::info("Found speech, processing locally");

    if(_whisp.convert_audio_to_text(_speech_segment, _speech_segment.size(), estimated_text)) {
      _speech_segment.clear();
      return -5;
    }
    _speech_segment.clear();
    return 1;
  }

  return 0;
}

void audio_wrapper::clear_speech_buffer() {
  _speech_segment.clear();
}
