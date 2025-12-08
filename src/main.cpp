#include <Arduino.h>
#include <AudioTools.h>


uint16_t sample_rate = 44100;
uint8_t channels = 2;
SineWaveGenerator<int16_t> sineWave(32000);
GeneratedSoundStream<int16_t> sound(sineWave);
CsvOutput<int16_t> out(Serial);
StreamCopy copier(out, sound);


void setup() {
  // open Serial
  Serial.begin(9600);
  AudioLogger::instance().begin(Serial, AudioLogger::Info);

  // defince CSV output
  auto config = out.defaultConfig();
  config.sample_rate = sample_rate;
  config.channels = channels;
  out.begin(config);

  // Setup sine wave
  sineWave.begin(channels, sample_rate, N_B4);
  Serial.println("started...");
}


void loop() {
  // copy sound to out
  copier.copy();
}
