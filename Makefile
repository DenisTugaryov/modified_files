all: modified_files

modified_files: modified_files.o
	g++ modified_files.o -lboost_system -lboost_filesystem -o modified_files

modified_files.o: modified_files.cpp
	g++ -Wall -c -std=c++0x modified_files.cpp

clean:
	rm -rf *.o