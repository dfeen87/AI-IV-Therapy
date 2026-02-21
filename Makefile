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

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS)

test_safety_monitor: tests/test_safety_monitor.cpp $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o test_safety_monitor tests/test_safety_monitor.cpp $(TEST_OBJS)

test_state_estimator: tests/test_state_estimator.cpp $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o test_state_estimator tests/test_state_estimator.cpp $(TEST_OBJS)

test: test_safety_monitor test_state_estimator
	./test_safety_monitor
	./test_state_estimator

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) test_safety_monitor test_state_estimator

.PHONY: all clean test
