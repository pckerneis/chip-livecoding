# Detect the operating system
UNAME_S := $(shell uname -s)

# Common compiler flags
CFLAGS = -Wall -Wextra -O2 -std=c99 -Iinclude
LDFLAGS = -lm

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    # Windows settings
    CC = gcc
    TARGET = chip-livecoding.exe
    LDFLAGS += -lportaudio -lsndfile -llua54
    CFLAGS += -I"C:/Program Files (x86)/Lua/include"
    LDFLAGS += -L"C:/Program Files (x86)/Lua/lib"
    RM = del /Q /F
    MKDIR = mkdir
else
    # Linux settings
    CC = gcc
    TARGET = chip-livecoding
    LDFLAGS += -lportaudio -lsndfile -llua5.1
    CFLAGS += -I/usr/include/lua5.1
    RM = rm -f
    MKDIR = mkdir -p
endif

# Source files
SRC = src/main.c src/audio.c src/lua_utils.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJ) $(TARGET)

install: $(TARGET)
ifeq ($(OS),Windows_NT)
	copy $(TARGET) "C:\Windows\"
else
	sudo install -m 755 $(TARGET) /usr/local/bin/
endif

uninstall:
ifeq ($(OS),Windows_NT)
	del "C:\Windows\$(TARGET)"
else
	sudo rm -f /usr/local/bin/$(TARGET)
endif
