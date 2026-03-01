CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDES = -I../include -I.

CATCH_SRC = catch_amalgamated.cpp

CORE_SRC = ../src/core/text.cpp \
           ../src/core/spike_detector.cpp \
           ../src/core/detection.cpp \
           ../src/core/config.cpp \
           ../src/core/chat_buffer.cpp

TEST_SRC = test_text.cpp \
           test_url.cpp \
           test_spike_detector.cpp \
           test_detector.cpp \
           test_config.cpp

all: test_runner

test_runner: $(CATCH_SRC) $(TEST_SRC) $(CORE_SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(CATCH_SRC) $(TEST_SRC) $(CORE_SRC)

test: test_runner
	./test_runner "[core]"

test_text: test_runner
	./test_runner "[text]"

test_url: test_runner
	./test_runner "[url]"

test_spike: test_runner
	./test_runner "[spike]"

test_detector: test_runner
	./test_runner "[detector]"

test_config: test_runner
	./test_runner "[config]"

clean:
	rm -f test_runner

.PHONY: all test test_text test_url test_spike test_detector test_config clean
