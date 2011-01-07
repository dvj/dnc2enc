

#INCDIRS =	-I/home/dvj/dev/stage/include -I.
INCDIRS = -I/usr/include/gdal -I. -I/home/dvj/dev/stage/include 
LBDIRS   =	-L/home/dvj/dev/stage/lib
#LIBS     = -lgdal
LIBS	= -lgdal1.7.0
CC = g++
CFLAGS = -g -Wall

SRC = buildtables.cpp dnctoenc.cpp geopoint.cpp geohandler.cpp

BIN = dnc2enc

all: $(BIN)

# Erases your binary, object files, and temporary files
clean:
	rm -rf *.o ./*~ ./#* $(BIN)

# Generates object code for each source file
%.o:    %.cpp
	$(CC) $(CFLAGS) -c $^ $(INCDIRS)
$(BIN): $(SRC:.cpp=.o)
	$(CC) $(CFLAGS) -o $@ $^ $(LBDIRS) $(LIBS)





