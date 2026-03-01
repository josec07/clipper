// Ref: Z-Score anomaly detection with Welford's online variance - see docs/REFERENCES.md "Z-Score Anomaly Detection"
#pragma once

#include <deque>
#include <chrono>

namespace ctic {
namespace core {

class SpikeDetector {
public:
    explicit SpikeDetector(size_t window_size = 60, double threshold_sigma = 3.0);
    
    void addSample(double message_rate);
    
    bool isSpike() const;
    
    double getSpikeIntensity() const;
    
    double getBaseline() const;
    
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

}
}
