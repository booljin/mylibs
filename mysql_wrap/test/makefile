CXX = g++
CC = gcc
RM = rm -f
CXXFLAGS += -Wall -g -D__DEBUG  -D_WIN32 

SRC_FILES = ../mysql_wrap.cpp test.cpp

O_FILES = $(foreach name, $(notdir $(SRC_FILES)), $(basename $(name)).o)
INC += $(foreach base_dir, $(dir $(SRC_FILES)), -I $(base_dir))
INC += -I/c/"Program Files (x86)"/MySQL/mysql-connector-c-6.1.2-win32/include

LIBS = -L/c/"Program Files (x86)"/MySQL/mysql-connector-c-6.1.2-win32/lib

all:$(O_FILES)
	g++  $(LIBS)  -o test $^ /c/"Program Files (x86)"/MySQL/mysql-connector-c-6.1.2-win32/lib/vs9/mysqlclient.lib libcmt.lib AdvAPI32.Lib  #-lmysql   #-enable-stdcall-fixup

mysql_wrap.o:../mysql_wrap.cpp
	g++ $(CXXFLAGS) -c $(INC) $< -o $@
	
test.o:test.cpp
	g++ $(CXXFLAGS) -c $(INC) $< -o $@

clean:
	$(RM) $(O_FILES) test.exe
	
test:
	echo $(O_FILES)
	echo $(INC)
	
test1:
	g++ $(CXXFLAGS) -E -c $(INC) ../mysql_wrap.cpp -o pp.txt