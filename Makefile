#basic makefile for nsm
objects=nsm.o DateTimeInfo.o Directory.o Filename.o FileSystem.o GitInfo.o Parser.o Path.o ProjectInfo.o Quoted.o Title.o TrackedInfo.o WatchList.o
cppfiles=nsm.cpp DateTimeInfo.cpp Directory.cpp Filename.cpp FileSystem.cpp GitInfo.cpp Parser.cpp Path.cpp ProjectInfo.cpp Quoted.cpp Title.cpp TrackedInfo.cpp WatchList.cpp
CXX?=g++
LINK=-pthread
CXXFLAGS+= -std=c++11 -Wall -Wextra -pedantic -O3
#flags to use when compiling for Netlify
#CXXFLAGS+= -std=c++11 -Wall -Wextra -pedantic -O3 -static-libgcc -static-libstdc++
#Flags to use when compiling for Chocolatey
#CXXFLAGS=-std=c++11 -Wall -Wextra -pedantic -O3 -static -static-libgcc -static-libstdc++
DESTDIR?=
PREFIX?=/usr/local
BINDIR=${DESTDIR}${PREFIX}/bin

all: nsm

nsm: $(objects)
	$(CXX) $(CXXFLAGS) $(objects) -o nsm $(LINK)
	$(CXX) $(CXXFLAGS) $(objects) -o nift $(LINK)

nsm.o: nsm.cpp GitInfo.o ProjectInfo.o Timer.h
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(LINK)

ProjectInfo.o: ProjectInfo.cpp ProjectInfo.h GitInfo.o Parser.o WatchList.o
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(LINK)

GitInfo.o: GitInfo.cpp GitInfo.h FileSystem.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Parser.o: Parser.cpp Parser.h DateTimeInfo.o FileSystem.o TrackedInfo.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

WatchList.o: WatchList.cpp WatchList.h FileSystem.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

FileSystem.o: FileSystem.cpp FileSystem.h Path.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

DateTimeInfo.o: DateTimeInfo.cpp DateTimeInfo.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

TrackedInfo.o: TrackedInfo.cpp TrackedInfo.h Path.o Title.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Path.o: Path.cpp Path.h Directory.o Filename.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Directory.o: Directory.cpp Directory.h Quoted.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Filename.o: Filename.cpp Filename.h Quoted.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Title.o: Title.cpp Title.h Quoted.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Quoted.o: Quoted.cpp Quoted.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

linux-gedit-highlighting:
	chmod 644 html.lang
	cp html.lang /usr/share/gtksourceview-3.0/language-specs/html.lang

install:
	mkdir -p ${BINDIR}
	chmod 755 nsm
	mv nift ${BINDIR}
	mv nsm ${BINDIR}

uninstall:
	rm ${BINDIR}/nift
	rm ${BINDIR}/nsm

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

