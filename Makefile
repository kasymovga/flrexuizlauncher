WINDRES=windres
CXX=c++
CC=cc
COMPILE=$(CXX) $(CXXFLAGS) -Wall -g -std=c++03 -c -o
LINK=$(CXX) $(LDFLAGS) -static-libstdc++ -o
LINK_FLAGS_FLTK=`fltk-config --use-images --ldflags`
COMPILE_FLAGS_FLTK=`fltk-config --use-images --cflags`
LINK_FLAGS_CURL=`pkg-config --static --libs libcurl`
COMPILE_FLAGS_CURL=`pkg-config --cflags libcurl`
LINK_MBEDTLS_FLAGS=-lmbedcrypto
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
	rm -f *.o rexuiz_icon.h rexuiz_logo.h rexuiz_pub_key.h $(FLREXUIZLAUNCHER)
ifeq ($(TARGET),mac)
	rm -rf RexuizLauncher.app
endif

main.o: main.cpp launcher.h
	$(COMPILE) $@ $<

launcher.o: launcher.cpp launcher.h gui.h downloader.h settings.h fs.h rexuiz.h sign.h index.h
	$(COMPILE) $@ $<

gui.o: gui.cpp gui.h rexuiz_logo.h rexuiz_icon.h
	$(COMPILE) $@ $(COMPILE_FLAGS_FLTK) $<

sign.o: sign.cpp rexuiz_pub_key.h fs.h
	$(COMPILE) $@ $<

downloader.o: downloader.cpp downloader.h fs.h
	$(COMPILE) $@ $(COMPILE_FLAGS_CURL) $<

fs.o: fs.cpp fs.h
	$(COMPILE) $@ $<

icon.res: icon.rc icon.ico
	$(WINDRES) $< -O coff -o $@

$(FLREXUIZLAUNCHER): main.o launcher.o gui.o downloader.o rexuiz.o fs.o settings.o sign.o unzip.o miniz.o index.o $(ICON)
ifeq ($(TARGET),mac)
	rm -rf RexuizLauncher.app/
	cp -a RexuizLauncher.app-tmpl/ RexuizLauncher.app/
endif
	$(LINK) $@ $^ $(LINK_FLAGS_FLTK) $(LINK_FLAGS_CURL) $(LINK_MBEDTLS_FLAGS) $(LINK_OS_FLAGS)

rexuiz_logo.h: rexuiz_logo.png
	xxd -n rexuiz_logo -i $< $@

rexuiz_icon.h: rexuiz_icon.png
	xxd -n rexuiz_icon -i $< $@

rexuiz.o: rexuiz.cpp rexuiz.h fs.h
	$(COMPILE) $@ $<

rexuiz_pub_key.h: rexuiz_pub.key
	xxd -n rexuiz_pub_key -i $< $@

unzip.o : unzip.cpp unzip.h fs.h miniz.h
	$(COMPILE) $@ $<

miniz.o: miniz.cpp miniz.h
	$(COMPILE) $@ $<
