# What to call the final executable
TARGET = monknxrf

OBJS= monitorknxrf.o cc1101.o sensorKNXRF.o

# What compiler to use
CC = g++

CFLAGS = -c -Wall -Iinclude/

LIBS = -lwiringPi -lsystemd

# Link the target with all objects and libraries
$(TARGET) : $(OBJS)
	$(CC)  -o $(TARGET) $(OBJS) $(LIBS)

$(OBJS) : monitorknxrf.cpp
	$(CC) $(CFLAGS) $< include/*.cpp

.PHONY : clean
clean :
	rm -f $(TARGET) $(OBJS)
