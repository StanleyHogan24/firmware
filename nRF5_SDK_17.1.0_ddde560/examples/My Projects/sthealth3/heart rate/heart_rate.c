//heart_rate.c

#include "heart_rate.h"
#include "nrf_log.h"
#include <math.h>         // For sqrtf(), isnanf(), and isfinitef()
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "globals.h"
#include "max30101.h"

/*
 * Configuration Constants
 *
 * Note: The MAX30101 datasheet specifies an 18–bit ADC with a full–scale range of 16384 nA.
 * In our processing chain, we assume a baseline signal level that falls within the sensor’s dynamic range.
 * The sample rate here is chosen to be 100 Hz, which is appropriate for heart rate monitoring.
 */
#define SAMPLE_RATE_HZ          100                           // Sampling rate: 100 samples per second.
#define MS_PER_SAMPLE           (1000 / SAMPLE_RATE_HZ)       // Milliseconds per sample (~10 ms per sample).

// Define minimum and maximum allowable intervals (in ms) between peaks.
// These thresholds correspond to physiologically reasonable heart rate limits (40 BPM to 200 BPM).
#define MIN_PEAK_SPACING_MS     300   // Minimum interval: ~300 ms (~200 BPM maximum)
#define MAX_PEAK_SPACING_MS     1500  // Maximum interval: ~1500 ms (~40 BPM minimum)

// Filter Parameters for Signal Conditioning:
// The MAX30101’s raw output (after its onboard processing) can be noisy. These filters help isolate the pulsatile component.
// LPF_ALPHA and HPF_ALPHA are chosen empirically (range 0 < alpha < 1).
#define LPF_ALPHA               0.5f  // Low-pass filter coefficient.
#define HPF_ALPHA               0.1f  // High-pass filter coefficient.

// Dynamic Threshold Parameters for Peak Detection:
// The dynamic threshold is computed based on a moving average (baseline) and the signal’s standard deviation.
// This allows the algorithm to adapt to changes in signal amplitude.
#define THRESHOLD_FACTOR        1.7f   // Multiplier for the standard deviation (tunable based on experimental data).
#define MOVING_AVG_WINDOW       100    // Window size (number of samples) used for the moving average and standard deviation.

// BPM Calculation Limits (for sanity checking detected peak intervals)
#define MAX_BPM                 200
#define MIN_BPM                 40



/*
 * Static Variables to Maintain State Between Calls
 */

// Sample counter: counts the total number of samples processed.
static uint32_t s_sampleCounter  = 0;

// Latest computed BPM value.
static uint16_t s_bpm            = 0;

// Holds the sample counter value at the last detected peak (for computing time intervals between peaks).
static uint32_t s_lastPeakSample = 0;

// Variables used for filtering:
// s_prev_input is used to compute the high–pass filter output.
// The low–pass filter (LPF) state variable is initialized to 13000.0f; this baseline is chosen
// based on expected signal levels from the MAX30101 (with a full–scale range around 16384 nA).
static float s_prev_input = 0.0f; // Previous input sample for the high-pass filter.

// s_prevSlopeSign holds the previous sign (positive or negative) of the slope of the filtered signal.
static int8_t s_prevSlopeSign = 0;

// Filter state variables for the low–pass and high–pass filters.
static float lp_filtered = 12000.0f; // Low-pass filter state, starting near expected baseline.
static float hp_filtered = 0.0f;     // High-pass filter state.

// Moving average buffer and related variables for dynamic thresholding.
static float moving_avg_buffer[MOVING_AVG_WINDOW];
static uint16_t moving_avg_index = 0;
static float moving_avg_sum = 0.0f; // Running sum for the moving average calculation.



/*
 * heart_rate_init()
 *
 * Initializes the heart rate processing module.
 * This includes resetting sample counters, filter states, and clearing the moving average buffer.
 * It is important that the LPF state is initialized near the expected baseline level
 * (here, 13000.0f, which is chosen based on typical sensor output levels per the datasheet).
 */
void heart_rate_init(void)
{
    NRF_LOG_INFO("HR_INIT: Initialization started.");
    
    s_sampleCounter  = 0;
    s_bpm            = 0;
    s_lastPeakSample = 0;
    s_prevSlopeSign  = 0;
    
    // Initialize the moving average buffer to zero.
    for (uint16_t i = 0; i < MOVING_AVG_WINDOW; i++) {
        moving_avg_buffer[i] = 0.0f;
    }
    moving_avg_sum = 0.0f;
    
    // Initialize filter states:
    // lp_filtered is set to an initial baseline (here, 1000000.0f) to reflect the expected sensor reading.
    // hp_filtered is initialized to zero since there is no initial high-frequency component.
    lp_filtered = 12000.0f;
    hp_filtered = 0.0f;
    s_prev_input = lp_filtered;
    
    NRF_LOG_INFO("HR_INIT: Initialization completed.");
}

/*
 * heart_rate_update()
 *
 * Processes a new sample from the sensor, applies filtering,
 * performs dynamic thresholding for peak detection, and calculates BPM.
 *
 * The algorithm steps are:
 *  1. Validate the new sample to ensure it is not NaN or infinite.
 *  2. Apply a low–pass filter (LPF) to smooth the signal.
 *  3. Apply a high–pass filter (HPF) to remove baseline wander.
 *  4. Compute the slope of the HPF output to detect the change in signal polarity.
 *  5. Update a moving average buffer to estimate the baseline and compute the standard deviation.
 *  6. Calculate a dynamic threshold using the baseline plus a multiple of the standard deviation.
 *  7. Detect a peak when the slope changes from positive to negative and the signal exceeds the threshold.
 *  8. Compute the time interval (in milliseconds) between successive peaks and convert it to BPM.
 *
 * @param newSample The latest raw or pre-filtered sample from the sensor.
 * @return The current BPM value.
 */
uint16_t heart_rate_update(float newSample)
{
    NRF_LOG_INFO(">> Entered heart_rate_udpate\r\n");
    s_sampleCounter++;
    NRF_LOG_INFO("HR_UPDATE: Sample Counter: %u\r\n", s_sampleCounter);
    
    
    // Step 0: Input Validation
    // If the new sample is not a number (NaN) or is infinite, skip processing.
    if (isnan(newSample) || !isfinite(newSample)) {
        NRF_LOG_ERROR("HR_UPDATE: newSample is NaN or INF. Skipping this sample.\r\n");
        return s_bpm;
    }

 

    // Step 1: Low-Pass Filtering (LPF)
    // The LPF smooths the input signal to reduce high-frequency noise.
    // The filter uses an exponential moving average formula.
    lp_filtered = LPF_ALPHA * newSample + (1.0f - LPF_ALPHA) * lp_filtered;
    if (isnan(lp_filtered) || !isfinite(lp_filtered)) {
        NRF_LOG_ERROR("HR_UPDATE: LPF resulted in NaN or INF. Resetting to baseline.\r\n");
        lp_filtered =1400000.0f;  // Reset to a fallback baseline value if error occurs.
    }
    NRF_LOG_DEBUG("Low-Pass Filtered Value:" NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(lp_filtered));

    
    
    // Step 2: High-Pass Filtering (HPF)
    // The HPF removes slow–varying components (baseline wander) from the LPF output.
    // This is computed using the previous LPF value.
    hp_filtered = (HPF_ALPHA * (hp_filtered + lp_filtered - s_prev_input))*-1;
    if (isnan(hp_filtered) || !isfinite(hp_filtered)) {
        NRF_LOG_ERROR("HR_UPDATE: HPF resulted in NaN or INF. Resetting to baseline.\r\n");
        hp_filtered = 0.0f;
        s_prev_input = lp_filtered;
    }
    NRF_LOG_INFO("High-Pass Filtered Value:" NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(hp_filtered));
    
    // Update the previous input for use in the next HPF calculation.
    s_prev_input = lp_filtered;

    
    // Step 3: Compute the Slope of the HPF Output
    // The slope (difference between current and previous HPF outputs) helps detect the moment when the signal peaks.
    static float s_prev_hp = 0.0f; // Holds the previous HPF output.
    float slope = hp_filtered - s_prev_hp;
    if (isnan(slope) || !isfinite(slope)) {
        NRF_LOG_ERROR("HR_UPDATE: Slope resulted in NaN or INF. Resetting slope sign.\r\n");
        slope = 0.0f;
    }
    s_prev_hp = hp_filtered;  // Update for next iteration.
    
    NRF_LOG_DEBUG("Slope:" NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(slope));
    NRF_LOG_DEBUG("prev_hp:" NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(s_prev_hp));
    
    // Determine the current slope sign.
    // A positive slope indicates a rising edge; a negative slope indicates a falling edge.
    int8_t currSlopeSign = 0;
    if (slope > 0) {
        currSlopeSign = +1;
    }
    else if (slope < 0) {
        currSlopeSign = -1;
    }
    NRF_LOG_DEBUG("Current Slope Sign: %d", currSlopeSign);
    
    bool peakDetected = false;
    
    // Step 4: Baseline Wander Removal and Dynamic Normalization
    // Update the moving average buffer with the latest HPF output.
    // This helps establish a baseline that adapts to slow changes in the signal.
    moving_avg_sum -= moving_avg_buffer[moving_avg_index];
    moving_avg_buffer[moving_avg_index] = hp_filtered;
    moving_avg_sum += hp_filtered;
    moving_avg_index = (moving_avg_index + 1) % MOVING_AVG_WINDOW;
    
    // Compute the moving average (baseline).
    float moving_avg = moving_avg_sum / MOVING_AVG_WINDOW;
    if (isnan(moving_avg) || !isfinite(moving_avg)) {
        NRF_LOG_ERROR("HR_UPDATE: Moving Average resulted in NaN or INF. Resetting to baseline.\r\n");
        moving_avg = 0.0f;
        moving_avg_sum = 0.0f;
        for (uint16_t i = 0; i < MOVING_AVG_WINDOW; i++) {
            moving_avg_buffer[i] = 0.0f;
        }
    }
    NRF_LOG_DEBUG("Moving Average:" NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(moving_avg));
    
    // Compute the standard deviation over the moving average window.
    // This quantifies the variability (noise level) of the baseline.
    float sum_sq_diff = 0.0f;
    for (uint16_t i = 0; i < MOVING_AVG_WINDOW; i++) {
        float diff = moving_avg_buffer[i] - moving_avg;
        sum_sq_diff += diff * diff;
    }
    float std_dev = sqrtf(sum_sq_diff / MOVING_AVG_WINDOW);
    if (isnan(std_dev) || !isfinite(std_dev)) {
        NRF_LOG_ERROR("HR_UPDATE: Standard Deviation resulted in NaN or INF. Resetting to zero.\r\n");
        std_dev = 0.0f;
    }
    NRF_LOG_DEBUG("Standard Deviation:" NRF_LOG_FLOAT_MARKER , NRF_LOG_FLOAT(std_dev));
    
    // Step 5: Dynamic Thresholding for Peak Detection
    // The dynamic threshold adapts to the current noise level by adding a multiple of the standard deviation to the baseline.
    float dynamic_threshold = moving_avg + (THRESHOLD_FACTOR * std_dev);
    if (isnan(dynamic_threshold) || !isfinite(dynamic_threshold)) {
        NRF_LOG_ERROR("HR_UPDATE: Dynamic Threshold resulted in NaN or INF. Resetting to moving average.\r\n");
        dynamic_threshold = moving_avg;
    }
    NRF_LOG_INFO("Dynamic Threshold:" NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(dynamic_threshold));
    
    // Step 6: Peak Detection
    // A peak is detected when:
    //   - The slope changes from positive to negative (indicating a local maximum), and
    //   - The high-pass filtered signal exceeds the dynamic threshold.
    if ((s_prevSlopeSign > 0) && (currSlopeSign < 0) && (hp_filtered > dynamic_threshold)) {
        peakDetected = true;
        NRF_LOG_INFO("Peak Detected at Sample: %u, HPF Value:" NRF_LOG_FLOAT_MARKER, s_sampleCounter, NRF_LOG_FLOAT(hp_filtered));
    }
    
    // Update the previous slope sign for the next iteration.
    s_prevSlopeSign = currSlopeSign;
    
    // Step 7: BPM Calculation Based on Detected Peaks
    if (peakDetected) {
        // Calculate the interval (in samples) since the last detected peak.
        uint32_t peakIntervalSamples = s_sampleCounter - s_lastPeakSample;
        s_lastPeakSample = s_sampleCounter;
        
        // Convert the interval to milliseconds.
        uint32_t peakIntervalMs = peakIntervalSamples * MS_PER_SAMPLE;
        NRF_LOG_DEBUG("Peak Interval: %u ms", peakIntervalMs);
        
        // Validate the peak interval to ensure it falls within physiologically reasonable limits.
        if ((peakIntervalMs >= MIN_PEAK_SPACING_MS) && (peakIntervalMs <= MAX_PEAK_SPACING_MS)) {
            s_bpm = (uint16_t)(60000UL / peakIntervalMs);
            NRF_LOG_INFO("BPM Calculated: %u BPM\r\n", s_bpm);

             if(!max30101_reset_fifo())

                {
                  NRF_LOG_ERROR("Failed to reset MAX30101 FIFO registers.")

        } else {
            NRF_LOG_WARNING("Peak Interval Out of Range: %u ms", peakIntervalMs);
        }

          //nrf_delay_ms(SAMPLE_PERIOD_MS);
    }
    
    // Return the most recently calculated BPM.
    return s_bpm;
  }
}

/*
 * heart_rate_get_bpm()
 *
 * Returns the latest computed BPM value.
 */
uint16_t heart_rate_get_bpm(void)
{
    return s_bpm;
}
