# Makefile for WordStar Clone
# For use with Microsoft Visual C++ (cl.exe)

# Compiler and flags
CC = cl
CFLAGS = /nologo /W3 /O2 /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN
LDFLAGS = /link kernel32.lib user32.lib
RC = rc

# Debug build flags
DEBUG_CFLAGS = /nologo /W3 /Od /Zi /D_DEBUG /D_CRT_SECURE_NO_WARNINGS /DWIN32_LEAN_AND_MEAN
DEBUG_LDFLAGS = /link /DEBUG kernel32.lib user32.lib

# Target names
TARGET = wordstar.exe
DEBUG_TARGET = wordstar_debug.exe

# Object files
OBJS = main.obj
DEBUG_OBJS = main_debug.obj

# Default target
all: $(TARGET)

# Debug target
debug: $(DEBUG_TARGET)

# Release build
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) /out:$(TARGET)
	@echo.
	@echo Release build complete: $(TARGET)

# Debug build
$(DEBUG_TARGET): $(DEBUG_OBJS)
	$(CC) $(DEBUG_OBJS) $(DEBUG_LDFLAGS) /out:$(DEBUG_TARGET)
	@echo.
	@echo Debug build complete: $(DEBUG_TARGET)

# Object file compilation - Release
main.obj: main.c
	$(CC) $(CFLAGS) /c main.c

# Object file compilation - Debug
main_debug.obj: main.c
	$(CC) $(DEBUG_CFLAGS) /c main.c /Fo:main_debug.obj

# Clean all generated files
clean:
	-del *.obj *.exe *.pdb *.ilk 2>nul
	@echo Clean complete

# Install to system directory (requires admin)
install: $(TARGET)
	copy $(TARGET) %WINDIR%\system32\
	@echo Installed to system directory

# Create distribution package
dist: $(TARGET)
	-mkdir dist 2>nul
	copy $(TARGET) dist\
	copy README.md dist\
	copy LICENSE dist\
	@echo Distribution package created in dist\

# Run the program
run: $(TARGET)
	$(TARGET)

# Run with test file
test: $(TARGET)
	echo This is a test file > test.txt
	$(TARGET) test.txt

# Help target
help:
	@echo Available targets:
	@echo   all     - Build release version (default)
	@echo   debug   - Build debug version
	@echo   clean   - Remove all generated files
	@echo   install - Install to system directory (admin required)
	@echo   dist    - Create distribution package
	@echo   run     - Run the program
	@echo   test    - Run with test file
	@echo   help    - Show this help

.PHONY: all debug clean install dist run test help