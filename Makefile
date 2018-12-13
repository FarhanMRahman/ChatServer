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

obj/%.o: src/%.cpp ${OBJ_DIR}
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

server: obj/server.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ 

client: obj/client.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ 

clean:
	rm -f server client submission
	rm -rf obj
	rm -f *~ obj/*.o obj/*.a *.zip
