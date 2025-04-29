CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I/usr/local/include -I/opt/homebrew/include
LDFLAGS = -lSDL2 -L/usr/local/lib -L/opt/homebrew/lib

TARGET = raycaster
SRCS = raycaster.cpp

OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
