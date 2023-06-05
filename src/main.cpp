#include <Arduino.h>
#include <I2S.h>

#include "biquadnotch.h"
#include "calibrate.h"

#define M_PI   3.14159265f

const uint32_t buf_size = 256;
const uint32_t sample_rate = 48000;

auto i2s_mic_input = I2S(INPUT);
bool i2s_read_ready = false;

auto i2s_speaker_output = I2S(OUTPUT);



int WtrP;
float Rd_P;
float Shift;
float CrossFade;
const int BufSize = 256;//buf_size;
int Buf[BufSize];
const int Overlap = 100;

void i2s_rcv_callback()
{
  i2s_read_ready = true;
  // printf("Buf full!\n");

  // int32_t sample_l {};
  // int32_t sample_r {};

  //  printf("%d %d %d\n", sample_l >> 8, 838860, -838860);
}

void setup()
{
  Serial1.begin(115200);

  // Setup mic input
  i2s_mic_input.setBCLK(19);
  i2s_mic_input.setDATA(21);
  i2s_mic_input.setBitsPerSample(24);
  i2s_mic_input.setBuffers(4, buf_size);
  i2s_mic_input.setFrequency(sample_rate);
  i2s_mic_input.onReceive(i2s_rcv_callback);
  if (!i2s_mic_input.begin())
  {
    printf("Error setting up i2s mic input");
    while (1)
    {
    };
  }

  // Setup speaker output
  i2s_speaker_output.setBCLK(3);
  i2s_speaker_output.setDATA(2);
  i2s_speaker_output.setBitsPerSample(16);
  i2s_speaker_output.setBuffers(4, buf_size);
  i2s_speaker_output.setFrequency(sample_rate);
  if (!i2s_speaker_output.begin())
  {
    printf("Error setting up i2s output");
    while (1)
    {
    };
  }

  WtrP = 0;
	Rd_P = 0.0f;
	Shift = 1.5f;
	CrossFade = 1.0f;

  calibrate(sample_rate, 100, i2s_mic_input, i2s_speaker_output);
}

BiquadNotch filter1(2350, sample_rate, 5);
BiquadNotch filter2(3800, sample_rate, 5);

int32_t sample_l{};
int32_t sample_r{};
int32_t highest {};
int32_t highest_ever {};

float sample_l_f{};
const float gain = 1;

// pitch shifting algorithm adapted from https://github.com/YetAnotherElectronicsChannel/STM32_DSP_PitchShift
void loop()
{
  i2s_mic_input.read24(&sample_l, &sample_r);

  // To float
  sample_l >>= 16;
  sample_l_f = (float)sample_l;

  // // Notch filter
  // sample_l_f = filter1.apply(sample_l_f);
  // // sample_l_f = filter2.apply(sample_l_f);

  // // Gain
  sample_l_f *= 2.5;

  // if (sample_l_f > )

  // // Back to int
  sample_l = (int32_t)sample_l_f;

  // Pitch shift
  //write to ringbuffer
	Buf[WtrP] = sample_l;

  //read fractional readpointer and generate 0° and 180° read-pointer in integer
	int RdPtr_Int = roundf(Rd_P);
	int RdPtr_Int2 = 0;
	if (RdPtr_Int >= BufSize/2) RdPtr_Int2 = RdPtr_Int - (BufSize/2);
	else RdPtr_Int2 = RdPtr_Int + (BufSize/2);

  //read the two samples...
	float Rd0 = (float) Buf[RdPtr_Int];
	float Rd1 = (float) Buf[RdPtr_Int2];

  //Check if first readpointer starts overlap with write pointer?
	// if yes -> do cross-fade to second read-pointer
	if (Overlap >= (WtrP-RdPtr_Int) && (WtrP-RdPtr_Int) >= 0 && Shift!=1.0f) {
		int rel = WtrP-RdPtr_Int;
		CrossFade = ((float)rel)/(float)Overlap;
	}
	else if (WtrP-RdPtr_Int == 0) CrossFade = 0.0f;

	//Check if second readpointer starts overlap with write pointer?
	// if yes -> do cross-fade to first read-pointer
	if (Overlap >= (WtrP-RdPtr_Int2) && (WtrP-RdPtr_Int2) >= 0 && Shift!=1.0f) {
			int rel = WtrP-RdPtr_Int2;
			CrossFade = 1.0f - ((float)rel)/(float)Overlap;
		}
	else if (WtrP-RdPtr_Int2 == 0) CrossFade = 1.0f;

	//do cross-fading and sum up
	sample_l = (Rd0*CrossFade + Rd1*(1.0f-CrossFade));

  //increment fractional read-pointer and write-pointer
	Rd_P += Shift;
	WtrP++;
	if (WtrP == BufSize) WtrP = 0;
	if (roundf(Rd_P) >= BufSize) Rd_P = 0.0f;

  // sample_l <<= 16;
  i2s_speaker_output.write16(sample_l, sample_l);
  // if (i2s_read_ready)
  // {
  //   // printf("Reading %d samples!\n", buf_size);
  //   highest = 0;
  //   for (uint32_t i = 0; i < buf_size; i++)
  //   {
  //     i2s_mic_input.read24(&sample_l, &sample_r);
  //     // sample_l = sample_l >> 8; // Move 24-bits packed in MSB 32-bit to LSB
  //     // // sample_l_f = (float)sample_l;
  //     // // sample_l_f = filter.apply(sample_l_f);
  //     // // // sample_l_f *= 1.2;
  //     // // sample_l = (int32_t)sample_l_f;
  //     // // sample_l = cancelFeedback(sample_l);

  //     // // sample_l_f = (float)sample_l;
  //     // // sample_l_f *= gain;
  //     // // sample_l = (int32_t)sample_l_f;
  //     // sample_l = sample_l >> 8; // Truncate 8 LSB to make 16-bit value;
  //     // if (abs(sample_l) > highest) {
  //     //   highest = abs(sample_l);
  //     // }
  //     i2s_speaker_output.write24(sample_l, sample_l);
  //   }
  //   if (highest > highest_ever) {
  //     highest_ever = highest;
  //   }
  //   // printf("Highest: %d, %d\n", highest, highest_ever);
  // }
}