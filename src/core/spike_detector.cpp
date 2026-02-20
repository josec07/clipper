// added by [X] LLM model
#include "spike_detector.h"
#include <cmath>

namespace chatclipper {

// Ref: Sliding window parameters - see docs/REFERENCES.md "Moving Window Statistics"
SpikeDetector::SpikeDetector(size_t window_size, double threshold_sigma)
    : window_size_(window_size), threshold_sigma_(threshold_sigma), sum_(0.0), sum_sq_(0.0) {}

// Ref: Sliding window maintenance - see docs/REFERENCES.md "Moving Window Statistics"
void SpikeDetector::addSample(double message_rate) {
    samples_.push_back(message_rate);
    updateStats(message_rate, true);
    
    if (samples_.size() > window_size_) {
        updateStats(samples_.front(), false);
        samples_.pop_front();
    }
}

// Ref: Z-Score anomaly detection - see docs/REFERENCES.md "Z-Score Anomaly Detection"
bool SpikeDetector::isSpike() const {
    if (samples_.size() < 2) return false;
    
    double current = samples_.back();
    double mean = calculateMean();
    double stddev = calculateStdDev();
    
    if (stddev == 0.0) return false;
    
    double z_score = (current - mean) / stddev;
    return z_score > threshold_sigma_;
}

// Ref: Z-Score normalization - see docs/REFERENCES.md "Z-Score Anomaly Detection"
double SpikeDetector::getSpikeIntensity() const {
    if (samples_.size() < 2) return 0.0;
    
    double current = samples_.back();
    double mean = calculateMean();
    double stddev = calculateStdDev();
    
    if (stddev == 0.0) return 0.0;
    
    double z_score = (current - mean) / stddev;
    
    // Normalize to 0.0 - 1.0 range (saturate at 5 sigma)
    double intensity = std::min(z_score / 5.0, 1.0);
    return std::max(intensity, 0.0);
}

// Ref: Moving average baseline - see docs/REFERENCES.md "Moving Window Statistics"
double SpikeDetector::getBaseline() const {
    return calculateMean();
}

// Ref: State reset - standard practice
void SpikeDetector::reset() {
    samples_.clear();
    sum_ = 0.0;
    sum_sq_ = 0.0;
}

// Ref: Welford's online variance - see docs/REFERENCES.md "Online Variance"
void SpikeDetector::updateStats(double sample, bool adding) {
    if (adding) {
        sum_ += sample;
        sum_sq_ += sample * sample;
    } else {
        sum_ -= sample;
        sum_sq_ -= sample * sample;
    }
}

// Ref: Running mean calculation - see docs/REFERENCES.md "Online Variance"
double SpikeDetector::calculateMean() const {
    if (samples_.empty()) return 0.0;
    return sum_ / samples_.size();
}

// Ref: Welford's variance formula - see docs/REFERENCES.md "Online Variance"
double SpikeDetector::calculateStdDev() const {
    if (samples_.size() < 2) return 0.0;
    
    double mean = calculateMean();
    double variance = (sum_sq_ / samples_.size()) - (mean * mean);
    
    if (variance < 0) return 0.0;
    return std::sqrt(variance);
}

} // namespace chatclipper
