#include <Arduino.h>
#include "settings.h"
#include "RotaryEncoder.h"
#include "AudioPlayer.h"
#include "Log.h"
#include "System.h"

#ifdef USEROTARY_ENABLE
#include <ESP32Encoder.h>
#endif

// Rotary encoder-configuration
#ifdef USEROTARY_ENABLE
	ESP32Encoder encoder;
	// Rotary encoder-helper
	int32_t lastEncoderValue;
	int32_t currentEncoderValue;
	int32_t lastVolume = -1;                                // Don't change -1 as initial-value!
#endif

void RotaryEncoder_Init(void) {
	// Init rotary encoder
	#ifdef USEROTARY_ENABLE
		#ifndef REVERSE_ROTARY
			encoder.attachHalfQuad(ROTARYENCODER_CLK, ROTARYENCODER_DT);
		#else
			encoder.attachHalfQuad(ROTARYENCODER_DT, ROTARYENCODER_CLK);
		#endif
		encoder.clearCount();
		encoder.setCount(AudioPlayer_GetInitVolume() * 2); // Ganzes Raster ist immer +2, daher initiale Lautstärke mit 2 multiplizieren
	#endif
}

void RotaryEncoder_Readjust(void) {
	#ifdef USEROTARY_ENABLE
		encoder.clearCount();
		encoder.setCount(AudioPlayer_GetCurrentVolume() * 2);
	#endif
}

const int RunningAverageCount = 32;
float RunningAverageBuffer[RunningAverageCount];
int NextRunningAverage;
int32_t potVolume = 0;
uint8_t currentVolume = AudioPlayer_GetCurrentVolume();
int32_t lastVolume = -1;                              


// Handles volume directed by rotary encoder
void RotaryEncoder_Cyclic(void) {


  float RawVolume = analogRead(POTENTIOMETER);
  RunningAverageBuffer[NextRunningAverage++] = RawVolume;
  if (NextRunningAverage >= RunningAverageCount)
  {
    NextRunningAverage = 0; 
  }
  float RunningAverageVolume = 0;
  for(int i=0; i< RunningAverageCount; ++i)
  {
    RunningAverageVolume += RunningAverageBuffer[i];
  }
  RunningAverageVolume /= RunningAverageCount;

    potVolume = (RunningAverageVolume - 300)/161.9f ; 
    if (potVolume<0)
        potVolume = 0;
    if (potVolume>21)   
        potVolume = 21;
    
    currentVolume = potVolume;
    if (currentVolume != lastVolume) {  
        lastVolume = currentVolume;
		AudioPlayer_VolumeToQueueSender(currentVolume,false);
    }

	if (AudioPlayer_GetCurrentVolume() != lastVolume) {
		AudioPlayer_PauseOnMinVolume(lastVolume, AudioPlayer_GetCurrentVolume());
		lastVolume = AudioPlayer_GetCurrentVolume();
		AudioPlayer_VolumeToQueueSender(AudioPlayer_GetCurrentVolume(), false);
	}

	#ifdef USEROTARY_ENABLE
#ifdef INCLUDE_ROTARY_IN_CONTROLS_LOCK
		if (System_AreControlsLocked()) {
			encoder.clearCount();
			encoder.setCount(AudioPlayer_GetCurrentVolume() * 2);
			return;
		}
#endif

		currentEncoderValue = encoder.getCount();
		// Only if initial run or value has changed. And only after "full step" of rotary encoder
		if (((lastEncoderValue != currentEncoderValue) || lastVolume == -1) && (currentEncoderValue % 2 == 0)) {
			System_UpdateActivityTimer(); // Set inactivity back if rotary encoder was used
			if ((AudioPlayer_GetMaxVolume() * 2) < currentEncoderValue) {
				encoder.clearCount();
				encoder.setCount(AudioPlayer_GetMaxVolume() * 2);
				Log_Println((char *) FPSTR(maxLoudnessReached), LOGLEVEL_INFO);
				currentEncoderValue = encoder.getCount();
			} else if (currentEncoderValue < AudioPlayer_GetMinVolume()) {
				encoder.clearCount();
				encoder.setCount(AudioPlayer_GetMinVolume());
				Log_Println((char *) FPSTR(minLoudnessReached), LOGLEVEL_INFO);
				currentEncoderValue = encoder.getCount();
			}
			lastEncoderValue = currentEncoderValue;
			AudioPlayer_SetCurrentVolume(lastEncoderValue / 2u);
			if (AudioPlayer_GetCurrentVolume() != lastVolume) {
				AudioPlayer_PauseOnMinVolume(lastVolume, AudioPlayer_GetCurrentVolume());
				lastVolume = AudioPlayer_GetCurrentVolume();
				AudioPlayer_VolumeToQueueSender(AudioPlayer_GetCurrentVolume(), false);
			}
		}
	#endif
}
