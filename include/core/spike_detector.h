// added by [X] LLM model
// Ref: Z-Score anomaly detection with Welford's online variance - see docs/REFERENCES.md "Z-Score Anomaly Detection"
#pragma once

#include <deque>
#include <chrono>

namespace chatclipper {

class SpikeDetector {
public:
    explicit SpikeDetector(size_t window_size = 60, 
                          double threshold_sigma = 3.0);
    
    // Add a new sample (messages per second)
    void addSample(double message_rate);
    
    // Check if current state is a spike
    bool isSpike() const;
    
    // Get spike intensity (0.0 - 1.0)
    double getSpikeIntensity() const;
    
    // Get current baseline (moving average)
    double getBaseline() const;
    
    // Reset detector
    void reset();
    
private:
    std::deque<double> samples_;
    size_t window_size_;
    double threshold_sigma_;
    double sum_;
    double sum_sq_;
    
    void updateStats(double sample, bool adding);
    double calculateMean() const;
    double calculateStdDev() const;
};

} // namespace chatclipper
