WINDRES=windres
CXX=c++
CC=cc
STRIP=strip
COMPILE=$(CXX) $(CXXFLAGS) -Wall -std=c++03 -c -o
LINK=$(CXX) $(LDFLAGS) $(LINK_FLAGS_EXTRA) -o
LINK_FLAGS_FLTK=`fltk-config --use-images --ldflags`
COMPILE_FLAGS_FLTK=`fltk-config --use-images --cflags`
LINK_FLAGS_CURL=`pkg-config --static --libs libcurl`
COMPILE_FLAGS_CURL=`pkg-config --cflags libcurl`
LINK_MBEDTLS_FLAGS=-lmbedcrypto
LINK_ZLIB_FLAGS=-lz
UNAME := $(shell uname)
ifeq ($(UNAME), Windows)
TARGET ?= windows
else
ifeq ($(UNAME), Darwin)
TARGET ?= mac
else
TARGET ?= linux
endif
endif

ifeq ($(TARGET),windows)
FLREXUIZLAUNCHER=flrexuizlauncher.exe
LINK_OS_FLAGS=-lshlwapi
ICON=icon.res
else
ifeq ($(TARGET),mac)
FLREXUIZLAUNCHER=RexuizLauncher.app/Contents/MacOS/flrexuizlauncher
LINK_OS_FLAGS=
ICON=
else
FLREXUIZLAUNCHER=flrexuizlauncher
LINK_OS_FLAGS=
ICON=
endif
endif

.PHONY: all clean

all: $(FLREXUIZLAUNCHER)

clean:
	rm -f *.o rexuiz_icon.h rexuiz_logo.h rexuiz_pub_key.h $(FLREXUIZLAUNCHER) $(ICON)
ifeq ($(TARGET),mac)
	rm -rf RexuizLauncher.app
endif

main.o: main.cpp launcher.h
	$(COMPILE) $@ $<

launcher.o: launcher.cpp launcher.h gui.h downloader.h settings.h fs.h rexuiz.h sign.h index.h translation.h translation_data.h process.h
	$(COMPILE) $@ $<

gui.o: gui.cpp gui.h rexuiz_logo.h rexuiz_icon.h
	$(COMPILE) $@ $(COMPILE_FLAGS_FLTK) $<

sign.o: sign.cpp rexuiz_pub_key.h fs.h
	$(COMPILE) $@ $<

downloader.o: downloader.cpp downloader.h fs.h
	$(COMPILE) $@ $(COMPILE_FLAGS_CURL) $<

fs.o: fs.cpp fs.h
	$(COMPILE) $@ $<

process.o: process.cpp process.h
	$(COMPILE) $@ $<

icon.res: icon.rc icon.ico
	$(WINDRES) $< -O coff -o $@

$(FLREXUIZLAUNCHER): main.o launcher.o gui.o downloader.o rexuiz.o fs.o settings.o sign.o unzip.o index.o translation.o process.o $(ICON)
ifeq ($(TARGET),mac)
	rm -rf RexuizLauncher.app/
	cp -a RexuizLauncher.app-tmpl/ RexuizLauncher.app/
	mkdir -m 755 -p RexuizLauncher.app/Contents/MacOS
endif
	$(LINK) $@ $^ $(LINK_FLAGS_FLTK) $(LINK_FLAGS_CURL) $(LINK_MBEDTLS_FLAGS) $(LINK_ZLIB_FLAGS) $(LINK_OS_FLAGS)
	$(STRIP) $@

rexuiz_logo.h: rexuiz_logo.png
	xxd -n rexuiz_logo -i $< $@

rexuiz_icon.h: rexuiz_icon.png
	xxd -n rexuiz_icon -i $< $@

rexuiz.o: rexuiz.cpp rexuiz.h fs.h
	$(COMPILE) $@ $<

rexuiz_pub_key.h: rexuiz_pub.key
	xxd -n rexuiz_pub_key -i $< $@

unzip.o : unzip.cpp unzip.h fs.h
	$(COMPILE) $@ $<

translation_data.h: translation.txt
	xxd -n translation_data -i $< $@

translation.o: translation.cpp translation.h
	$(COMPILE) $@ $<
