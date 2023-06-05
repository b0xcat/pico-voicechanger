#include <Arduino.h>
#include <I2S.h>
#include <etl/circular_buffer.h>
#include <pico/float.h>

const int32_t min_24 = -8388608;
const int32_t max_24 = 8388607;

const int16_t min_16 = -32768;
const int16_t max_16 = 32767;

template <uint32_t N_TAPS>
class NLMSFilter {
private:

    float weights[N_TAPS] = {0};
    float samples[N_TAPS] = {0};

    float step_size {};
    float eps {};

    template<typename T1, typename T2>
    float dot(T1 v1, T2 v2) {
        float acc = 0;
        for(uint32_t i = 0; i < N_TAPS; i++) {
            acc += v1[i] * v2[i];
        }
        return acc;
    }



public:
    NLMSFilter(float step_size = 0.3, float eps = 0.001) {

        this->step_size = step_size;
        this->eps = eps;
    }

    void learn(int16_t speaker_samples[], int16_t mic_samples[], uint32_t n_samples) {
        for (uint32_t i = 0; i < n_samples - N_TAPS; i++) {
            
            // Copy samples into samples buffer
            for (uint32_t p = 0; p < N_TAPS; p++) {
                samples[p] = (float)speaker_samples[i + p] / (float)max_16;
            }

            // Calculate the filter output
            float y = dot(samples, weights);

            // Calculate the error (how much we deviate from what the mic hears)
            float expected = (float)mic_samples[i + N_TAPS] / (float)max_16;
            float e = expected - y;

            printf("Training [%d/%d]: error = %f\n", i, n_samples - N_TAPS, e);

            // Calculate the normalization factor to prevent blowup on large values
            float normfactor = 1.0f / (dot(samples, samples) + eps);

            // update weights
            for (uint32_t i = 0; i < N_TAPS; i++) {
                weights[i] = weights[i] + step_size * normfactor * samples[i] * e;
            }
        }
        // float y = calc_filter_output(sample);
        // float e = -y;

        // float normfactor = 1.0f / (dot(samples, samples) + eps);

        // // update weights
        // for (uint32_t i = 0; i < N_TAPS; i++) {
        //     weights[i] = weights[i] + step_size * normfactor * samples[i] * e;
        // }

        // return y;
    }

    float calc_filter_output(float sample) {
        samples.push(sample);
        float y = dot(weights, samples);
        return y;
    }
};

void calibrate(const uint32_t sample_rate, uint32_t len_ms, I2S& in, I2S& out) {
    // Generate white noise to capture impulse response
    // We are working in 24-bits signed values, so limit appropriately


    // Calculate how many samples we need to run 
    uint32_t n_samples_target = (sample_rate / 1000) * len_ms;
    uint32_t n_samples_played = 0;

    NLMSFilter<64> filter;

    // const uint32_t num_samples = 4800;

    int32_t mic_samples[n_samples_target] = {0};
    int16_t speaker_samples[n_samples_target] = {0};

    // Generate white noise
    int16_t rand_sample;
    for (uint32_t i = 0; i < n_samples_target; i++) {
        speaker_samples[i] = random(min_16, max_16);
    }

    // Play noise and record mic

    // delay(1000);
    printf("hecc");

    int32_t in_l{};
    int32_t in_r{};

    // first zeros
    for (uint32_t i = 0; i < 48000 * 5; i++) {
        out.write16(0, 0);
        in.read24(&in_l, &in_r);
    }

    for (uint32_t i = 0; i < n_samples_target; i++) {
        int16_t cur_out_sample = speaker_samples[i];
        out.write16(cur_out_sample, cur_out_sample);
        in.read24(&in_l, &in_r);
        // in_l >>= 8;
        mic_samples[i] = in_l;
    }


    // Play noise and record mic
    // for (uint32_t i = 0; i < n_samples_target; i++) {
    //     int16_t cur_out_sample = speaker_samples[i];
    //     out.write16(cur_out_sample, cur_out_sample);
    //     in.read24(&in_l, &in_r);
    //     mic_samples[i] = in_l >> 16;
    // }

    printf("mic = [");
    for(uint32_t i = 0; i < n_samples_target; i++) {
        printf("%d", mic_samples[i]);
        if (i < n_samples_target - 1) {
            printf(", ");
        }
    }
    printf("]\n");

    printf("speaker = [");
    for(uint32_t i = 0; i < n_samples_target; i++) {
        printf("%d", speaker_samples[i]);
        if (i < n_samples_target - 1) {
            printf(", ");
        }
    }
    printf("]\n");
    
    // for (uint i = 0; i < 10; i++) {
    //     printf("\n");
    // }
    

    // Train filter
    // filter.learn(speaker_samples, mic_samples, n_samples_target);
}