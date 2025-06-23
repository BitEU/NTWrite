# Makefile for WordStar Clone
# For use with Microsoft Visual C++ (cl.exe)

CC = cl
CFLAGS = /nologo /W3 /O2 /D_CRT_SECURE_NO_WARNINGS
LDFLAGS = /link kernel32.lib user32.lib

TARGET = wordstar.exe
OBJS = main.obj

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) /out:$(TARGET)

main.obj: main.c
	$(CC) $(CFLAGS) /c main.c

clean:
	del *.obj *.exe 2>nul
