// Ref: Z-Score anomaly detection with Welford's online variance - see docs/REFERENCES.md "Z-Score Anomaly Detection"
#include "../../include/core/spike_detector.h"
#include <cmath>

namespace ctic {
namespace core {

SpikeDetector::SpikeDetector(size_t window_size, double threshold_sigma)
    : window_size_(window_size), threshold_sigma_(threshold_sigma), sum_(0.0), sum_sq_(0.0) {}

void SpikeDetector::addSample(double message_rate) {
    samples_.push_back(message_rate);
    updateStats(message_rate, true);
    
    if (samples_.size() > window_size_) {
        updateStats(samples_.front(), false);
        samples_.pop_front();
    }
}

bool SpikeDetector::isSpike() const {
    if (samples_.size() < 2) return false;
    
    double current = samples_.back();
    double mean = calculateMean();
    double stddev = calculateStdDev();
    
    if (stddev == 0.0) return false;
    
    double z_score = (current - mean) / stddev;
    return z_score > threshold_sigma_;
}

double SpikeDetector::getSpikeIntensity() const {
    if (samples_.size() < 2) return 0.0;
    
    double current = samples_.back();
    double mean = calculateMean();
    double stddev = calculateStdDev();
    
    if (stddev == 0.0) return 0.0;
    
    double z_score = (current - mean) / stddev;
    
    double intensity = std::min(z_score / 5.0, 1.0);
    return std::max(intensity, 0.0);
}

double SpikeDetector::getBaseline() const {
    return calculateMean();
}

void SpikeDetector::reset() {
    samples_.clear();
    sum_ = 0.0;
    sum_sq_ = 0.0;
}

void SpikeDetector::updateStats(double sample, bool adding) {
    if (adding) {
        sum_ += sample;
        sum_sq_ += sample * sample;
    } else {
        sum_ -= sample;
        sum_sq_ -= sample * sample;
    }
}

double SpikeDetector::calculateMean() const {
    if (samples_.empty()) return 0.0;
    return sum_ / samples_.size();
}

double SpikeDetector::calculateStdDev() const {
    if (samples_.size() < 2) return 0.0;
    
    double mean = calculateMean();
    double variance = (sum_sq_ / samples_.size()) - (mean * mean);
    
    if (variance < 0) return 0.0;
    return std::sqrt(variance);
}

}
}
