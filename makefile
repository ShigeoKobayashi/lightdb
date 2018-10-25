DLL_SOURCES=base.cpp blockio.cpp cacheio.cpp fileio.cpp lightdb.cpp pageio.cpp
TEST_SOURCE=test.c
HEADERS=stdafx.h base.h blockio.h cacheio.h fileio.h lightdb.h pageio.h
OBJS=test.o base.o blockio.o cacheio.o fileio.o lightdb.o pageio.o

lib:
	g++ -o liblightdb.so -shared -fPIC -fvisibility=hidden $(DLL_SOURCES)
link:
	gcc -L./ -o test test.c -llightdb
run:
	LD_LIBRARY_PATH=. ./test
