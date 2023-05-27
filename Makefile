CXX = g++
CXXFLAGS = -Wall -std=c++11
LIBS = -ljpeg

parser: parser.cpp
	$(CXX) $(CXXFLAGS) -o parser parser.cpp $(LIBS)

clean:
	rm -f parser
