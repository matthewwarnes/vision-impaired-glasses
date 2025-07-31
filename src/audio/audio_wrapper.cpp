#include "audio_wrapper.h"

#include <SDL_mixer.h>
#include <SDL.h>
#include <portaudio.h>

#include <iostream>
#include <unistd.h>
#include <cstring>

#define SAMPLE_RATE  (16000)
#define FRAMES_PER_BUFFER (512)
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define NUM_CHANNELS    (1)
#define SAMPLE_SILENCE  (0)

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
  uint32_t SamplesPerSec = SAMPLE_RATE;   // Sampling Frequency in Hz
  uint32_t bytesPerSec = SAMPLE_RATE * 4; // bytes per second
  uint16_t blockAlign = 2;          // 2=16-bit mono, 4=16-bit stereo
  uint16_t bitsPerSample = sizeof(SAMPLE)*8; // Number of bits per sample
  /* "data" sub-chunk */
  uint8_t Subchunk2ID[4] = {'d', 'a', 't', 'a'}; // "data"  string
  uint32_t Subchunk2Size;                        // Sampled data length
} wav_hdr_t;



audio_wrapper::audio_wrapper(YAML::Node config, whisper_wrapper& w) : _whisp(w) {

  _mic_dev = config["device"].as<std::string>();


  if(SDL_Init(SDL_INIT_AUDIO) == -1) {
    std::cerr << "ERROR: failed to initialise SDL: " << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024) == -1) {
    std::cerr << "ERROR: failed to initialise SDL Mixer: " << Mix_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  PaError err = Pa_Initialize();
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to initialise portaudio: " << Pa_GetErrorText(err) << std::endl;
    exit(EXIT_FAILURE);
  }

}

audio_wrapper::~audio_wrapper() {
  Pa_Terminate();
  Mix_CloseAudio();
  SDL_Quit();
}

int audio_wrapper::play_from_mem(std::vector<uint8_t>& audio_arr) {

  SDL_RWops* rw = SDL_RWFromMem(audio_arr.data(), audio_arr.size());
  Mix_Music* audio =  Mix_LoadMUS_RW(rw,1);
  if(!audio) {
    std::cerr << "ERROR: failed to load audio data: " << Mix_GetError() << std::endl;
    return -1;
  }

  if(Mix_PlayMusic(audio, 1)) {
    std::cerr << "ERROR: failed to play music: " << Mix_GetError() << std::endl;
    return -2;
  }

  while(1) {
    if(!Mix_PlayingMusic()) {
      break;
    }
    usleep(50000);
  }

  Mix_FreeMusic(audio);
  audio = NULL;
  return 0;
}

int audio_wrapper::play_from_file(const std::string filename) {

  Mix_Music *audio_file = NULL;
  audio_file = Mix_LoadMUS(filename.c_str());
  if(!audio_file) {
    std::cerr << "ERROR: failed to load audio file: " << Mix_GetError() << std::endl;
    return -1;
  }

  if(Mix_PlayMusic(audio_file, 1)) {
    std::cerr << "ERROR: failed to play music: " << Mix_GetError() << std::endl;
    return -2;
  }

  while(1) {
    if(!Mix_PlayingMusic()) {
      break;
    }
    usleep(50000);
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

typedef struct
{
    int          frameIndex;  /* Index into sample array. */
    int          maxFrameIndex;
    SAMPLE      *recordedSamples;
} paTestData;

static int recordCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    paTestData *data = (paTestData*)userData;
    const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
    SAMPLE *wptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
    long framesToCalc;
    long i;
    int finished;
    unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

    (void) outputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if( framesLeft < framesPerBuffer )
    {
        framesToCalc = framesLeft;
        finished = paComplete;
    }
    else
    {
        framesToCalc = framesPerBuffer;
        finished = paContinue;
    }

    if( inputBuffer == NULL )
    {
        for( i=0; i<framesToCalc; i++ )
        {
            *wptr++ = SAMPLE_SILENCE;  /* left */
            if( NUM_CHANNELS == 2 ) *wptr++ = SAMPLE_SILENCE;  /* right */
        }
    }
    else
    {
        for( i=0; i<framesToCalc; i++ )
        {
            *wptr++ = *rptr++;  /* left */
            if( NUM_CHANNELS == 2 ) *wptr++ = *rptr++;  /* right */
        }
    }
    data->frameIndex += framesToCalc;
    return finished;
}


int audio_wrapper::capture_audio(const uint32_t seconds, std::vector<uint8_t>& audio)
{
  int id = find_device_id(_mic_dev);
  if(id == -1) {
    std::cerr << "ERROR: failed to find microphone device" << std::endl;
    return -1;
  }

  PaStreamParameters  inputParameters;
  inputParameters.device = id;
  inputParameters.channelCount = 1;
  inputParameters.sampleFormat = PA_SAMPLE_TYPE;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;


  PaError err;
  PaStream* stream;
  paTestData data;
  int                 i;
  int                 totalFrames;
  int                 numSamples;
  int                 numBytes;

  totalFrames = seconds * SAMPLE_RATE;
  data.maxFrameIndex = totalFrames;
  data.frameIndex = 0;
  numSamples = totalFrames * NUM_CHANNELS;
  numBytes = numSamples * sizeof(SAMPLE);
  data.recordedSamples = (SAMPLE *) malloc( numBytes ); /* From now on, recordedSamples is initialised. */
  if( data.recordedSamples == NULL )
  {
      std::cerr << "ERROR: failed to malloc mic data buffer" << std::endl;
      return -6;
  }
  for( i=0; i<numSamples; i++ ) data.recordedSamples[i] = 0;

  err = Pa_OpenStream(
            &stream,
            &inputParameters,
            NULL,                  /* &outputParameters, */
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            paClipOff,      /* we won't output out of range samples so don't bother clipping them */
            recordCallback,
            &data );
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to open audio stream: " << Pa_GetErrorText(err) << std::endl;
    free( data.recordedSamples );
    return -2;
  }


  err = Pa_StartStream( stream );
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to start audio stream: " << Pa_GetErrorText(err) << std::endl;
    free( data.recordedSamples );
    return -3;
  }

  while( ( err = Pa_IsStreamActive( stream ) ) == 1 ) {
      Pa_Sleep(10);
  }
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to stream audio: " << Pa_GetErrorText(err) << std::endl;
    free( data.recordedSamples );
    return -4;
  }

  err = Pa_CloseStream( stream );
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to close stream: " << Pa_GetErrorText(err) << std::endl;
    free( data.recordedSamples );
    return -5;
  }


  //process raw sample data
  audio.clear();
  audio.resize(numBytes + sizeof(wav_hdr_t));
  {
    wav_hdr_t wav;
    wav.ChunkSize = numBytes + 36;
    wav.Subchunk2Size = numBytes;
    uint8_t *p = (uint8_t *)&wav;
    for(size_t i = 0; i < sizeof(wav_hdr_t); i++) {
      audio[i] = p[i];
    }
  }
  std::memcpy(&audio[sizeof(wav_hdr_t)], (uint8_t*)data.recordedSamples, numBytes);

  free( data.recordedSamples );
  return 0;

}

int audio_wrapper::capture_audio(const uint32_t seconds, std::vector<float>& audio)
{
  int id = find_device_id(_mic_dev);
  if(id == -1) {
    std::cerr << "ERROR: failed to find microphone device" << std::endl;
    return -1;
  }

  PaStreamParameters  inputParameters;
  inputParameters.device = id;
  inputParameters.channelCount = 1;
  inputParameters.sampleFormat = PA_SAMPLE_TYPE;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;


  PaError err;
  PaStream* stream;
  paTestData data;
  int                 i;
  int                 totalFrames;
  int                 numSamples;
  int                 numBytes;

  totalFrames = seconds * SAMPLE_RATE;
  data.maxFrameIndex = totalFrames;
  data.frameIndex = 0;
  numSamples = totalFrames * NUM_CHANNELS;
  numBytes = numSamples * sizeof(SAMPLE);
  data.recordedSamples = (SAMPLE *) malloc( numBytes ); /* From now on, recordedSamples is initialised. */
  if( data.recordedSamples == NULL )
  {
      std::cerr << "ERROR: failed to malloc mic data buffer" << std::endl;
      return -6;
  }
  for( i=0; i<numSamples; i++ ) data.recordedSamples[i] = 0;

  err = Pa_OpenStream(
            &stream,
            &inputParameters,
            NULL,                  /* &outputParameters, */
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,
            paClipOff,      /* we won't output out of range samples so don't bother clipping them */
            recordCallback,
            &data );
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to open audio stream: " << Pa_GetErrorText(err) << std::endl;
    free( data.recordedSamples );
    return -2;
  }


  err = Pa_StartStream( stream );
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to start audio stream: " << Pa_GetErrorText(err) << std::endl;
    free( data.recordedSamples );
    return -3;
  }

  while( ( err = Pa_IsStreamActive( stream ) ) == 1 ) {
    Pa_Sleep(10);
  }
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to stream audio: " << Pa_GetErrorText(err) << std::endl;
    free( data.recordedSamples );
    return -4;
  }

  err = Pa_CloseStream( stream );
  if( err != paNoError ) {
    std::cerr << "ERROR: failed to close stream: " << Pa_GetErrorText(err) << std::endl;
    free( data.recordedSamples );
    return -5;
  }


  //process raw sample data
  audio.clear();
  audio.resize(numSamples);
  std::memcpy(audio.data(), (uint8_t*)data.recordedSamples, numBytes);

  free( data.recordedSamples );
  return 0;

}


int audio_wrapper::capture_speech(std::vector<uint8_t>& speech, std::string& estimated_text, std::atomic<bool>& cancel)
{
  std::cout << "Listening for speech..." << std::endl;

  std::vector<float> audio_segment;
  std::vector<float> speech_segment;

  //TODO: stream audio input for processing while checking for speech

  while(cancel.load()) {
    if(capture_audio(1, audio_segment)) {
      return -1;
    }

    int vad_result = _whisp.contains_speech(audio_segment);
    if(vad_result < 0) {
      return -2;
    } else if (vad_result > 0) {
      //speech found
      std::cout << "Speech started" << std::endl;
      std::copy(audio_segment.begin(), audio_segment.end(), std::back_inserter(speech_segment));
    } else {
      //no speech
      if(speech_segment.size()) {
        //end of speech (there was previous speech found)
        std::cout << "Speech ended" << std::endl;
        break;
      }
    }
  }

  if(speech_segment.size()) {
    //copy speech segments into output wav format
    size_t numBytes = speech_segment.size() * sizeof(float);
    speech.clear();
    speech.resize(numBytes + sizeof(wav_hdr_t));
    {
      wav_hdr_t wav;
      wav.ChunkSize = numBytes + 36;
      wav.Subchunk2Size = numBytes;
      uint8_t *p = (uint8_t *)&wav;
      for(size_t i = 0; i < sizeof(wav_hdr_t); i++) {
        speech[i] = p[i];
      }
    }
    std::memcpy(&speech[sizeof(wav_hdr_t)], (uint8_t*)speech_segment.data(), numBytes);

    std::cout << "Processing speech locally" << std::endl;
    if(_whisp.convert_audio_to_text(speech_segment, speech_segment.size(), estimated_text)) return -5;
    return 1;
  }


  return 0;
}
