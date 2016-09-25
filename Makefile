TARGET = jj2-bridge.exe
LIBS =
CC = i686-w64-mingw32-gcc
CFLAGS = -Wall -s -Os

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f release.zip

release: $(TARGET)
	@rm -f release.zip
	zip -9 release.zip $(TARGET)