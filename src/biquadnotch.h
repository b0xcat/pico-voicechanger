#include <cmath>

/**
 * @brief      Digital biquadratic notch filter implementation. Uses
 *             Direct Form I (DF1) implementation which allows updating
 *             the coefficients of the filter on the fly.
 */
class BiquadNotch
{
public:
    /**
     * @brief      BiquadNotch constructor.
     *
     * @param[in]  fc    Center frequency of notch
     * @param[in]  fs    Sample rate of data filter will be applied to
     * @param[in]  Q     Filter quality factor (higher == tighter bandwidth)
     */
    BiquadNotch(float fc, float fs, float Q) { init(fc, fs, Q); }
    ~BiquadNotch() = default;

    /**
     * @brief      Updates the filter with new specifications
     *
     * @param[in]  fc    Center frequency of notch
     * @param[in]  fs    Sample rate of data filter will be applied to
     * @param[in]  Q     Filter quality factor (higher == tighter bandwidth)
     */
    void update(float fc, float fs, float Q)
    {
        // backup state
        float x1 = x1_, x2 = x2_;
        float y1 = y1_, y2 = y2_;

        // recreate filter with new specs
        init(fc, fs, Q);

        // restore state
        x1_ = x1;
        x2_ = x2;
        y1_ = y1;
        y2_ = y2;
    }

    /**
     * @brief      Applies the filter to the input sample. Uses a DF1
     *             implementation to support dynamically changing filter specs.
     *
     * @param[in]  x     Input sample
     *
     * @return     Filtered output sample
     */
    float apply(float x)
    {
        // apply filter using direct form 1 (DF1)
        const float y = b0_ * x + b1_ * x1_ + b2_ * x2_ - (a1_ * y1_ + a2_ * y2_);

        // shift feedback delay lines
        x2_ = x1_;
        x1_ = x;

        // shift feedforward delay lines
        y2_ = y1_;
        y1_ = y;

        return y;
    }

private:
    // \brief Filter parameters
    float fc_, fs_, Q_;  ///< center freq, sample freq, quality factor
    float b0_, b1_, b2_; ///< num coeffs
    float a1_, a2_;      ///< den coeffs

    // \brief Filter state
    float x1_, x2_; ///< feedback delay elements
    float y1_, y2_; ///< feedforward delay elements

    /**
     * @brief      Create a biquad notch filter
     *
     * @param[in]  fc    Center frequency of notch
     * @param[in]  fs    Sample rate of data filter will be applied to
     * @param[in]  Q     Filter quality factor (higher == tighter bandwidth)
     */
    void init(float fc, float fs, float Q)
    {
        // normalized frequency in [0, pi]
        static constexpr float PI_TWO = 2 * 3.14159265358979323846;
        const float omega = PI_TWO * (fc / fs);

        const float sn = sin(omega);
        const float cs = cos(omega);
        const float alpha = sn / (2 * Q);

        // keep around just for fun
        fc_ = fc;
        fs_ = fs;
        Q_ = Q;

        // notch biquad setup
        const float b0 = 1;
        const float b1 = -2 * cs;
        const float b2 = 1;
        const float a0 = 1 + alpha;
        const float a1 = -2 * cs;
        const float a2 = 1 - alpha;

        // normalize into standard biquad form (a0 == 1)
        b0_ = b0 / a0;
        b1_ = b1 / a0;
        b2_ = b2 / a0;
        a1_ = a1 / a0;
        a2_ = a2 / a0;

        // initialize delay elements
        x1_ = x2_ = 0;
        y1_ = y2_ = 0;
    }
};