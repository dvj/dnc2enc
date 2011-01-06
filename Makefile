

INCDIRS =	-I/home/dvj/dev/stage/include -I.
LBDIRS   =	-L/home/dvj/dev/stage/lib
LIBS     = -lgdal
CC = g++
CFLAGS = -g -Wall

SRC = buildtables.cpp dnctoenc.cpp

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





