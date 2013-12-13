CXX = g++
CC = gcc
RM = rm -f
CXXFLAGS += -Wall -g 

SRC_FILES = ../epoll_wrap.cpp ../connect_manager.cpp ../socket_util.cpp test_server.cpp

O_FILES = $(foreach name, $(notdir $(SRC_FILES)), $(basename $(name)).o)
INC += $(foreach base_dir, $(dir $(SRC_FILES)), -I $(base_dir))


all:$(O_FILES)
	g++  -o test_server $^

epoll_wrap.o:../epoll_wrap.cpp
	g++ $(CXXFLAGS) -c $(INC) $< -o $@
	
connect_manager.o:../connect_manager.cpp
	g++ $(CXXFLAGS) -c $(INC) $< -o $@
    
socket_util.o:../socket_util.cpp
	g++ $(CXXFLAGS) -c $(INC) $< -o $@

test_server.o:test_server.cpp
	g++ $(CXXFLAGS) -c $(INC) $< -o $@

clean:
	$(RM) $(O_FILES) test_server
