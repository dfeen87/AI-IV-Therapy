CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread
INCLUDES = -I src

SRCS = src/adaptive_iv_therapy_control_system.cpp \
       src/SystemLogger.cpp \
       src/SafetyMonitor.cpp \
       src/StateEstimator.cpp \
       src/AdaptiveController.cpp

OBJS = $(SRCS:.cpp=.o)

TARGET = ai_iv

# Tests
TEST_SRCS = src/SystemLogger.cpp src/SafetyMonitor.cpp src/StateEstimator.cpp src/AdaptiveController.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

# Neural estimator settings
NEURAL_MODEL_PATH ?= models/sensor_fusion_fdeep.json
NEURAL_INCLUDES   = -I/usr/include/eigen3
NEURAL_FLAGS      = -DENABLE_NEURAL_ESTIMATOR \
                    -DNEURAL_MODEL_PATH='"$(NEURAL_MODEL_PATH)"'

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS)

# Neural-enabled main binary
neural: $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(NEURAL_INCLUDES) $(NEURAL_FLAGS) \
	    -o $(TARGET)_neural $(SRCS)

test_safety_monitor: tests/test_safety_monitor.cpp $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o test_safety_monitor tests/test_safety_monitor.cpp $(TEST_OBJS)

test_state_estimator: tests/test_state_estimator.cpp $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o test_state_estimator tests/test_state_estimator.cpp $(TEST_OBJS)

test_neural_estimator: tests/test_neural_estimator.cpp $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(NEURAL_INCLUDES) \
	    -DENABLE_NEURAL_ESTIMATOR \
	    -DNEURAL_MODEL_PATH='"$(NEURAL_MODEL_PATH)"' \
	    -o test_neural_estimator tests/test_neural_estimator.cpp $(TEST_OBJS)

test: test_safety_monitor test_state_estimator
	./test_safety_monitor
	./test_state_estimator

test_all: test test_neural_estimator
	./test_neural_estimator

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET)_neural \
	      test_safety_monitor test_state_estimator test_neural_estimator

.PHONY: all neural clean test test_all
