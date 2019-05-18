#basic makefile for nsm
objects=nsm.o DateTimeInfo.o Directory.o Filename.o PageBuilder.o PageInfo.o Path.o Quoted.o SiteInfo.o Title.o
cppfiles=nsm.cpp DateTimeInfo.cpp Directory.cpp Filename.cpp PageBuilder.cpp PageInfo.cpp Path.cpp Quoted.cpp SiteInfo.cpp Title.cpp
CC=g++
CXXFLAGS=-std=c++11 -Wall -Wextra -pedantic

nsm: $(objects)
	$(CC) $(CXXFLAGS) $(cppfiles) -o nsm

nsm.o: nsm.cpp SiteInfo.o
	$(CC) $(CXXFLAGS) -c -o $@ $<

SiteInfo.o: SiteInfo.cpp SiteInfo.h PageBuilder.o
	$(CC) $(CXXFLAGS) -c -o $@ $<

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

install:
	chmod 755 nsm
	sudo mv nsm /usr/local/bin

linux-gedit-highlighting:
	chmod 644 html.lang
	sudo cp html.lang /usr/share/gtksourceview-3.0/language-specs/html.lang

linux-install:
	chmod 755 nsm
	sudo mv nsm /usr/local/bin
	chmod 644 html.lang
	sudo cp html.lang /usr/share/gtksourceview-3.0/language-specs/html.lang

osx-install:
	chmod 755 nsm
	sudo mkdir ~/.bin
	sudo mv nsm ~/.bin
	sudo cp /etc/paths ./
	sudo chmod a+w paths
	sudo echo "~/.bin" >> paths
	sudo chmod 644 paths
	sudo mv paths /etc/paths

windows-install:
	chmod 755 nsm
	mv nsm ~/bin

clean:
	rm -f $(objects)

clean-all:
	rm -f $(objects) nsm

