CXX=g++
CXXFLAGS += -g -Wall -Wextra -pthread
CPPFLAGS += -isystem src -std=c++14

MKDIR_P = mkdir -p
OBJ_DIR = obj

all: server client submission

${OBJ_DIR}:
	${MKDIR_P} ${OBJ_DIR}

submission:
	zip -r ChatSystem-submission.zip src

server: scr/server.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ 

client: scr/client.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ 

clean:
	rm -f server client submission
	rm -rf obj

