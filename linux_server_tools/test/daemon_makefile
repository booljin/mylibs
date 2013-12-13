CXX = g++
CC = gcc
RM = rm -f
CXXFLAGS += -Wall -g -D__DEBUG

SRC_FILES = daemon.cpp test.cpp

O_FILES = $(foreach name, $(notdir $(SRC_FILES)), $(basename $(name)).o)
INC += $(foreach base_dir, $(dir $(SRC_FILES)), -I $(base_dir))

all:$(O_FILES)
	g++ $^ -o test.exe

daemon.o:daemon.cpp
	g++ $(CXXFLAGS) -c $(INC) $< -o $@
	
test.o:test.cpp
	g++ $(CXXFLAGS) -c $(INC) $< -o $@

clean:
	$(RM) $(O_FILES) test.exe
	
test:
	echo $(O_FILES)
	echo $(INC)