#basic makefile for nsm
objects=nsm.o ConsoleColor.o DateTimeInfo.o Directory.o Expr.o ExprtkFns.o Filename.o FileSystem.o Getline.o GitInfo.o LuaFns.o LuaJIT.o NumFns.o Parser.o Path.o ProjectInfo.o Quoted.o StrFns.o SystemInfo.o Title.o TrackedInfo.o Variables.o WatchList.o
cppfiles=nsm.cpp ConsoleColor.cpp DateTimeInfo.cpp Directory.cpp Expr.cpp ExprtkFns.cpp Filename.cpp FileSystem.cpp Getline.cpp GitInfo.cpp LuaFns.cpp LuaJIT.cpp NumFns.cpp Parser.cpp Path.cpp ProjectInfo.cpp Quoted.cpp StrFns.cpp SystemInfo.cpp Title.cpp TrackedInfo.cpp Variables.cpp WatchList.cpp

DESTDIR?=
PREFIX?=/usr/local
BINDIR=${DESTDIR}${PREFIX}/bin
LIBDIR=${DESTDIR}${PREFIX}/lib
LUAJIT?=1

ifeq ($(LUAJIT),1)
LUAJIT_INC?=-LLuaJIT/src
else
LUAJIT_INC?=
endif

CXX?=g++
CXXFLAGS=-std=c++11 -Wall -Wextra -pedantic -O3
LINK=-pthread

#https://stackoverflow.com/a/14777895
ifeq ($(OS),Windows_NT) 
	detected_OS := Windows
else
	detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(detected_OS),Darwin)        # Mac OSX
	CXXFLAGS+= -pagezero_size 10000 -image_base 10000000 -Qunused-arguments
	LINK+= -ldl -L. -lluajit 
else ifeq ($(detected_OS),Windows)  # Windows
	#use these flags for a smaller binary
	#CXXFLAGS+= -s -Wa,-mbig-obj -Wno-cast-function-type -Wno-error=cast-function-type
	#flags to use when compiling for Chocolatey & Releases
	CXXFLAGS+= -s -Wa,-mbig-obj -static-libgcc -static-libstdc++ -Wno-cast-function-type -Wno-error=cast-function-type
	LINK+= -L. -llua51
else ifeq ($(detected_OS),FreeBSD)  #FreeBSD
	# CXX=clang
	CXXFLAGS+= -s -Qunused-arguments -lstdc++
	LINK+= -ldl -lm $(LUAJIT_INC) -lluajit            #use Nift built LuaJIT
	#LINK+= -ldl -lm -L/usr/local/lib -lluajit-5.1   #use FreeBSD LuaJIT
else                                # *nix
	#use these flags for a smaller binary
	#CXXFLAGS+= -s
	#flags to use when compiling for Netlify & Releases
	CXXFLAGS+= -s -static-libgcc -static-libstdc++
	LINK+= -ldl $(LUAJIT_INC) -lluajit
endif

###

all: make-luajit nsm

###

make-luajit:
ifeq ($(LUAJIT),1)
ifeq ($(detected_OS),Darwin)        # Mac OSX
	cd LuaJIT && $(MAKE) MACOSX_DEPLOYMENT_TARGET=10.9
	cp LuaJIT/src/libluajit.a ./
else ifeq ($(detected_OS),Windows)  # Windows
	cd LuaJIT && $(MAKE)
	copy LuaJIT\src\lua51.dll .
else ifeq ($(detected_OS),FreeBSD)  #FreeBSD
	cd LuaJIT && $(MAKE)
	cp LuaJIT/src/libluajit.so ./
else                                # *nix
	cd LuaJIT && $(MAKE)
endif
endif
###

nsm: $(objects)
ifeq ($(detected_OS),Windows)  # Windows
	$(CXX) $(CXXFLAGS) $(objects) -o nsm $(LINK)
	copy nsm.exe nift.exe
else
	$(CXX) $(CXXFLAGS) $(objects) -o nsm $(LINK)
	cp nsm nift
endif

nsm.o: nsm.cpp GitInfo.o ProjectInfo.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ProjectInfo.o: ProjectInfo.cpp ProjectInfo.h GitInfo.o Parser.o WatchList.o Timer.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

GitInfo.o: GitInfo.cpp GitInfo.h ConsoleColor.o FileSystem.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Parser.o: Parser.cpp Parser.h DateTimeInfo.o Expr.o ExprtkFns.o FileSystem.o Getline.o LuaFns.o LuaJIT.o SystemInfo.o TrackedInfo.o Variables.o 
	$(CXX) $(CXXFLAGS) -c -o $@ $<

WatchList.o: WatchList.cpp WatchList.h FileSystem.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ExprtkFns.o: ExprtkFns.cpp ExprtkFns.h Expr.o Variables.o Consts.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Expr.o: Expr.cpp Expr.h exprtk/exprtk.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Getline.o: Getline.cpp Getline.h ConsoleColor.o FileSystem.o StrFns.o Consts.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

FileSystem.o: FileSystem.cpp FileSystem.h Path.o SystemInfo.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

LuaFns.o: LuaFns.cpp LuaFns.h LuaJIT.o ConsoleColor.o Path.o Quoted.o Variables.o Consts.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

LuaJIT.o: LuaJIT.cpp LuaJIT.h StrFns.o LuaJIT/src/lua.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

DateTimeInfo.o: DateTimeInfo.cpp DateTimeInfo.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

TrackedInfo.o: TrackedInfo.cpp TrackedInfo.h Path.o Title.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Variables.o: Variables.cpp Variables.h NumFns.o Path.o StrFns.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

NumFns.o: NumFns.cpp NumFns.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Path.o: Path.cpp Path.h ConsoleColor.o Directory.o Filename.o SystemInfo.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

StrFns.o: StrFns.cpp StrFns.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Directory.o: Directory.cpp Directory.h Quoted.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Filename.o: Filename.cpp Filename.h Quoted.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

SystemInfo.o: SystemInfo.cpp SystemInfo.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Title.o: Title.cpp Title.h ConsoleColor.o Quoted.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ConsoleColor.o: ConsoleColor.cpp ConsoleColor.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Quoted.o: Quoted.cpp Quoted.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

###

install:
ifeq ($(detected_OS),Windows)  # Windows
	@echo "Will need to manually add 'lua51.dll', 'nsm.exe' and "
	@echo "'nift.exe' to a location searched by your path "
	@echo "variable, for example try 'C:\Windows\System32'"
else ifeq ($(detected_OS),FreeBSD)  #FreeBSD
	mkdir -p ${BINDIR}
	chmod 755 nsm
ifeq ($(LUAJIT),1)
	mv libluajit.so ${LIBDIR}/libluajit-5.1.so.2
endif
	mv nift ${BINDIR}
	mv nsm ${BINDIR}
else                           # *nix
	mkdir -p ${BINDIR}
	chmod 755 nsm
	mv nift ${BINDIR}
	mv nsm ${BINDIR}
endif 

uninstall:
ifeq ($(detected_OS),Windows)  # Windows
	@echo "Will need to manually remove 'lua51.dll', 'nsm.exe' "
	@echo "and 'nift.exe' from install location, typically "
	@echo "'C:\Windows\System32'"
else ifeq ($(detected_OS),FreeBSD)  #FreeBSD
	rm ${LIBDIR}/libluajit-5.1.so.2
	rm ${BINDIR}/nift
	rm ${BINDIR}/nsm
else                                # *nix
	rm ${BINDIR}/nift
	rm ${BINDIR}/nsm
endif 

git-bash-install:
	chmod 755 nsm
	mv nift ~/bin
	mv nsm ~/bin

git-bash-uninstall:
	rm ~/bin/nift
	rm ~/bin/nsm

clean:
ifeq ($(detected_OS),Darwin)        # Mac OSX
	rm -f $(objects)
	cd LuaJIT && $(MAKE) clean
else ifeq ($(detected_OS),Windows)  # Windows
	del -f $(objects)
	#cd LuaJIT && $(MAKE) clean #this doesn't work for some reason
else ifeq ($(detected_OS),FreeBSD)  #FreeBSD
	rm -f $(objects)
	cd LuaJIT && $(MAKE) clean
else                                # *nix
	rm -f $(objects)
	cd LuaJIT && $(MAKE) clean
endif 

clean-all:
ifeq ($(detected_OS),Darwin)        # Mac OSX
	rm -f $(objects) nsm nift libluajit.a
	cd LuaJIT && $(MAKE) clean
else ifeq ($(detected_OS),Windows)  # Windows
	del -f $(objects) nsm.exe nift.exe lua51.dll
	#cd LuaJIT && make clean #see same line for clean
else ifeq ($(detected_OS),FreeBSD)  #FreeBSD
	rm -f $(objects) nsm nift libluajit.so
	cd LuaJIT && $(MAKE) clean
else                                # *nix
	rm -f $(objects) nsm nift
	cd LuaJIT && $(MAKE) clean
endif 
