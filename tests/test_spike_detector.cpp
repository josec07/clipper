#include "catch_amalgamated.hpp"
#include "core/spike_detector.h"
#include <cmath>

using namespace ctic::core;
using Catch::Approx;

TEST_CASE("SpikeDetector construction", "[spike]") {
    SECTION("default parameters") {
        SpikeDetector detector;
        REQUIRE_FALSE(detector.isSpike());
        REQUIRE(detector.getBaseline() == Approx(0.0));
    }
    
    SECTION("custom parameters") {
        SpikeDetector detector(30, 2.5);
        REQUIRE_FALSE(detector.isSpike());
    }
}

TEST_CASE("SpikeDetector steady rate", "[spike]") {
    SpikeDetector detector(10, 3.0);
    
    for (int i = 0; i < 10; i++) {
        detector.addSample(5.0);
    }
    
    SECTION("no spike at steady rate") {
        REQUIRE_FALSE(detector.isSpike());
    }
    
    SECTION("baseline matches steady rate") {
        REQUIRE(detector.getBaseline() == Approx(5.0));
    }
}

TEST_CASE("SpikeDetector sudden spike", "[spike]") {
    SpikeDetector detector(10, 2.0);
    
    for (int i = 0; i < 8; i++) {
        detector.addSample(2.0);
    }
    
    SECTION("spike detection after sudden increase") {
        detector.addSample(15.0);
        REQUIRE(detector.isSpike());
    }
    
    SECTION("intensity increases with spike") {
        detector.addSample(15.0);
        double intensity = detector.getSpikeIntensity();
        REQUIRE(intensity > 0.5);
    }
}

TEST_CASE("SpikeDetector baseline adapts", "[spike]") {
    SpikeDetector detector(5, 3.0);
    
    for (int i = 0; i < 5; i++) {
        detector.addSample(2.0);
    }
    
    double baseline1 = detector.getBaseline();
    REQUIRE(baseline1 == Approx(2.0));
    
    for (int i = 0; i < 5; i++) {
        detector.addSample(10.0);
    }
    
    double baseline2 = detector.getBaseline();
    REQUIRE(baseline2 == Approx(10.0));
}

TEST_CASE("SpikeDetector spike intensity", "[spike]") {
    SpikeDetector detector(10, 3.0);
    
    for (int i = 0; i < 8; i++) {
        detector.addSample(1.0);
    }
    
    SECTION("intensity scales with deviation") {
        detector.addSample(10.0);
        double intensity = detector.getSpikeIntensity();
        REQUIRE(intensity >= 0.0);
        REQUIRE(intensity <= 1.0);
        REQUIRE(intensity > 0.0);
    }
}

TEST_CASE("SpikeDetector window rotation", "[spike]") {
    SpikeDetector detector(5, 3.0);
    
    for (int i = 0; i < 10; i++) {
        detector.addSample(static_cast<double>(i));
    }
    
    SECTION("baseline reflects recent values only") {
        double baseline = detector.getBaseline();
        REQUIRE(baseline == Approx(7.0));
    }
}

TEST_CASE("SpikeDetector reset", "[spike]") {
    SpikeDetector detector(10, 3.0);
    
    for (int i = 0; i < 10; i++) {
        detector.addSample(5.0);
    }
    
    REQUIRE(detector.getBaseline() == Approx(5.0));
    
    detector.reset();
    
    SECTION("reset clears state") {
        REQUIRE(detector.getBaseline() == Approx(0.0));
        REQUIRE_FALSE(detector.isSpike());
    }
}

TEST_CASE("SpikeDetector numerical stability", "[spike]") {
    SpikeDetector detector(100, 3.0);
    
    SECTION("large values don't overflow") {
        for (int i = 0; i < 100; i++) {
            detector.addSample(10000.0);
        }
        REQUIRE(detector.getBaseline() == Approx(10000.0));
        REQUIRE_FALSE(detector.isSpike());
    }
}
