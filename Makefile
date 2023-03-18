CC = g++
CFLAGS = -Wall -g
HEADERS = -I include
ARGS = -std=c++14 -lstdc++ -pthread

TARGET = test.out

SRC = $(shell find src -type f | grep '\.cpp')
OBJECT = $(patsubst src/%.cpp,obj/%.o,$(SRC))

all : $(TARGET)

$(TARGET): $(OBJECT)
	$(CC) $(CFLAGS) $(HEADERS) $(ARGS) $(OBJECT) -o $@

obj/%.o : src/%.cpp
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(HEADERS) $(ARGS) -c $< -o $@

clean:
	rm -f $(TARGET)
	rm -f obj/*.o