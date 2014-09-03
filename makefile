CC=g++
CFLAGS=-c -std=c++11
LDFLAGS=-lcurl
SOURCES=base64.cpp SHA1.cpp HMAC_SHA1.cpp BTCChinaAPI.cpp cpp-wrapper.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=btc-cpp-wrapper

$(EXECUTABLE): $(OBJECTS) 
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o btc-cpp-wrapper*