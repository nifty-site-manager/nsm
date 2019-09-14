#basic makefile for nsm
objects=nsm.o DateTimeInfo.o Directory.o Filename.o PageBuilder.o PageInfo.o Path.o Quoted.o SiteInfo.o Title.o
cppfiles=nsm.cpp DateTimeInfo.cpp Directory.cpp Filename.cpp PageBuilder.cpp PageInfo.cpp Path.cpp Quoted.cpp SiteInfo.cpp Title.cpp
CC=g++
LINK=-pthread
CXXFLAGS=-std=c++11 -Wall -Wextra -pedantic -O3
#Flags to use when compiling for Chocolatey
#CXXFLAGS=-std=c++11 -Wall -Wextra -pedantic -O3 -static -static-libgcc -static-libstdc++

nsm: $(objects)
	$(CC) $(CXXFLAGS) $(cppfiles) -o nsm $(LINK)
	$(CC) $(CXXFLAGS) $(cppfiles) -o nift $(LINK)

nsm.o: nsm.cpp SiteInfo.o Timer.h
	$(CC) $(CXXFLAGS) -c -o $@ $< $(LINK)

SiteInfo.o: SiteInfo.cpp SiteInfo.h PageBuilder.o
	$(CC) $(CXXFLAGS) -c -o $@ $< $(LINK)

PageBuilder.o: PageBuilder.cpp PageBuilder.h DateTimeInfo.o PageInfo.o
	$(CC) $(CXXFLAGS) -c -o $@ $<

DateTimeInfo.o: DateTimeInfo.cpp DateTimeInfo.h
	$(CC) $(CXXFLAGS) -c -o $@ $<

PageInfo.o: PageInfo.cpp PageInfo.h Path.o Title.o
	$(CC) $(CXXFLAGS) -c -o $@ $<

Path.o: Path.cpp Path.h Directory.o Filename.o
	$(CC) $(CXXFLAGS) -c -o $@ $<

Directory.o: Directory.cpp Directory.h Quoted.h
	$(CC) $(CXXFLAGS) -c -o $@ $<

Filename.o: Filename.cpp Filename.h Quoted.h
	$(CC) $(CXXFLAGS) -c -o $@ $<

Title.o: Title.cpp Title.h Quoted.o
	$(CC) $(CXXFLAGS) -c -o $@ $<

Quoted.o: Quoted.cpp Quoted.h
	$(CC) $(CXXFLAGS) -c -o $@ $<

linux-gedit-highlighting:
	chmod 644 html.lang
	cp html.lang /usr/share/gtksourceview-3.0/language-specs/html.lang

linux-install:
	chmod 755 nsm
	mv nift /usr/local/bin
	mv nsm /usr/local/bin

linux-uninstall:
	rm /usr/local/bin/nift
	rm /usr/local/bin/nsm

osx-install:
	chmod 755 nsm
	mv nift /usr/local/bin
	mv nsm /usr/local/bin

osx-uninstall:
	rm /usr/local/bin/nift
	rm /usr/local/bin/nsm 

git-bash-install:
	chmod 755 nsm
	mv nift ~/bin
	mv nsm ~/bin

git-bash-uninstall:
	rm ~/bin/nift
	rm ~/bin/nsm

gitbash-install:
	chmod 755 nsm
	mv nift ~/bin
	mv nsm ~/bin

gitbash-uninstall:
	rm ~/bin/nift
	rm ~/bin/nsm

linux-clean:
	rm -f $(objects)

linux-clean-all:
	rm -f $(objects) nsm nift

windows-clean:
	del -f $(objects)

windows-clean-all:
	del -f $(objects) nsm.exe nift.exe

clean:
	rm -f $(objects)

clean-all:
	rm -f $(objects) nsm nift

