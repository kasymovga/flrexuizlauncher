CPP=c++
CC=cc
COMPILE=$(CPP) -Wall -g -std=c++03 -c -o
LINK=$(CPP) -static-libstdc++ -o
LINK_FLAGS_FLTK=`fltk-config --use-images --ldflags`
COMPILE_FLAGS_FLTK=`fltk-config --use-images --cflags`
LINK_FLAGS_CURL=`pkg-config --libs libcurl`
COMPILE_FLAGS_CURL=`pkg-config --cflags libcurl`
LINK_MBEDTLS_FLAGS=-lmbedcrypto
UNAME := $(shell uname)

ifeq ($(UNAME), Windows)
FLREXUIZLAUNCHER = flrexuizlauncher.exe
else
FLREXUIZLAUNCHER = flrexuizlauncher
endif

.PHONY: all clean

all: flrexuizlauncher

clean:
	rm -f *.o rexuiz_icon.h rexuiz_logo.h rexuiz_pub_key.h $(FLREXUIZLAUNCHER)

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

$(FLREXUIZLAUNCHER): main.o launcher.o gui.o downloader.o rexuiz.o fs.o settings.o sign.o unzip.o miniz.o index.o
	$(LINK) $@ $^ $(LINK_FLAGS_FLTK) $(LINK_FLAGS_CURL) $(LINK_MBEDTLS_FLAGS)

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
