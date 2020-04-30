#basic makefile for nsm
objects=nsm.o ConsoleColor.o DateTimeInfo.o Directory.o Expr.o ExprtkFns.o Filename.o FileSystem.o Getline.o GitInfo.o HashTk.o Lolcat.o LuaFns.o Lua.o NumFns.o Pagination.o Parser.o Path.o ProjectInfo.o Quoted.o StrFns.o SystemInfo.o Title.o TrackedInfo.o Variables.o WatchList.o
cppfiles=nsm.cpp ConsoleColor.cpp DateTimeInfo.cpp Directory.cpp Expr.cpp ExprtkFns.cpp Filename.cpp FileSystem.cpp Getline.cpp GitInfo.cpp hashtk/HashTk.cpp Lolcat.cpp LuaFns.cpp Lua.cpp NumFns.cpp Pagination.cpp Parser.cpp Path.cpp ProjectInfo.cpp Quoted.cpp StrFns.cpp SystemInfo.cpp Title.cpp TrackedInfo.cpp Variables.cpp WatchList.cpp

DESTDIR?=
PREFIX?=/usr/local
BINDIR=${DESTDIR}${PREFIX}/bin
LIBDIR=${DESTDIR}${PREFIX}/lib

CXX?=g++
CXXFLAGS=-std=c++11 -Wall -Wextra -pedantic -O3
LINK=-pthread

ifeq ($(OS),Windows_NT) 
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(detected_OS),Darwin)        # Mac OSX
	CXXFLAGS+= -pagezero_size 10000 -image_base 10000000 -Qunused-arguments
else ifeq ($(detected_OS),Windows)  # Windows
	#use these flags for a smaller binary
	CXXFLAGS+= -s -Wa,-mbig-obj -Wno-cast-function-type -Wno-error=cast-function-type
	#flags to use when compiling for Chocolatey & Releases
	#this likely causes problems with lua throwing errors
	#CXXFLAGS+= -s -Wa,-mbig-obj -static-libgcc -static-libstdc++ -Wno-cast-function-type -Wno-error=cast-function-type
else ifeq ($(detected_OS),FreeBSD)  # FreeBSD
	CXX=clang
	CXXFLAGS+= -s -Qunused-arguments -lstdc++
else                                # *nix
	#use these flags for a smaller binary
	CXXFLAGS+= -s
	#flags to use when compiling for Netlify & Releases
	#this does not work well with lua throwing errors!
	#CXXFLAGS+= -s -static-libgcc -static-libstdc++
	#CXXFLAGS+= -s -unwind-tables -funwind-tables -fexceptions -static-libgcc -static-libstdc++
endif

ifeq ($(VERCEL),1)
	CXXFLAGS+= -D__NO_CLEAR_LINES -D__NO_COLOUR -D__NO_PROGRESS
else
	ifeq ($(NO_CLEAR_LINES),1)
		CXXFLAGS+= -D__NO_CLEAR_LINES
	endif

	ifeq ($(NO_COLOR),1)
		CXXFLAGS+= -D__NO_COLOUR
	endif

	ifeq ($(NO_PROGRESS),1)
		CXXFLAGS+= -D__NO_PROGRESS
	endif
endif

ifeq ($(BUNDLED),0)
	_BUNDLED_=0
	ifeq ($(LUA_VERSION),5.3) 
	    ifeq ($(detected_OS),Windows)  # Windows
			_BUNDLED_=1
			WAS_UNBUNDLED=1
	    	CXXFLAGS+= -D__BUNDLED__ -D__LUA_VERSION_5_3__
	    	LINK+= -LLua-5.3/src -llua
	    	#LINK+= -LLua-5.3/src -llua
		else ifeq ($(detected_OS),FreeBSD)  # FreeBSD
			CXXFLAGS+= -D__LUA_VERSION_5_3__
	    	LINK+= -L/usr/local/lib -llua -lm -ldl  
		else                                # *nix
			CXXFLAGS+= -D__LUA_VERSION_5_3__
	    	LINK+= -L/usr/local/lib -llua -ldl
		endif
	else #do we need -ldl below?
		ifeq ($(detected_OS),Windows)  # Windows
			_BUNDLED_=1
			WAS_UNBUNDLED=1
			CXXFLAGS+= -D__BUNDLED__ -D__LUA_VERSION_JIT__
			LINK+= -LLuaJIT/src -llua51
			#LINK+= -L. -llua51
		else ifeq ($(detected_OS),FreeBSD)  # FreeBSD
			CXXFLAGS+= -D__LUA_VERSION_JIT__
			LINK+= -ldl -lm -L/usr/local/lib -lluajit-5.1  
		else                                # *nix
			CXXFLAGS+= -D__LUA_VERSION_JIT__
			LINK+= -ldl -L/usr/local/lib -lluajit-5.1 
		endif
	endif
else
	_BUNDLED_=1
	ifeq ($(LUA_VERSION),5.3) 
		CXXFLAGS+= -D__BUNDLED__ -D__LUA_VERSION_5_3__
	    ifeq ($(detected_OS),Windows)  # Windows
	    	LINK+= -LLua-5.3/src -llua
	    	#LINK+= -LLua-5.3/src -llua
		else ifeq ($(detected_OS),FreeBSD)  # FreeBSD
	    	LINK+= -LLua-5.3/src -llua -ldl -lm
		else                                # *nix/Vercel
	    	LINK+= -LLua-5.3/src -llua -ldl
		endif
	else
		CXXFLAGS+= -D__BUNDLED__ -D__LUA_VERSION_JIT__
		ifeq ($(detected_OS),Windows)  # Windows
	    	LINK+= -LLuaJIT/src -llua51
			#LINK+= -L. -llua51
		else ifeq ($(detected_OS),FreeBSD)  # FreeBSD
			LINK+= ./LuaJIT/src/libluajit.a -ldl -lm
			#LINK+= -ldl -lm -LLuaJIT/src -lluajit
		else                                # *nix/Vercel
			LINK+= ./LuaJIT/src/libluajit.a -ldl
			#LINK+= -ldl -LLuaJIT/src -lluajit
		endif
	endif
endif


#ifeq ($(detected_OS),Linux)              # Linux
#else ifeq ($(detected_OS),GNU)           # Debian GNU Hurd
#else ifeq ($(detected_OS),GNU/kFreeBSD)  # Debian kFreeBSD
#else ifeq ($(detected_OS),FreeBSD)       # FreeBSD
#else ifeq ($(detected_OS),NetBSD)        # NetBSD
#else ifeq ($(detected_OS),DragonFly)     # DragonFly
#else ifeq ($(detected_OS),Haiku)         # Haiku
#endif

###

all: bundled-msg make-lua nsm

###

bundled-msg:
ifeq ($(WAS_UNBUNDLED) $(LUA_VERSION),1 5.3)
	@echo "only bundled option is available on Windows"
	@echo "compiling with bundled Lua 5.3"
else ifeq ($(WAS_UNBUNDLED),1)
	@echo "only bundled option is available on Windows"
	@echo "compiling with bundled LuaJIT"
endif

###

make-lua:
ifeq ($(_BUNDLED_),0)
else ifeq ($(WAS_UNBUNDLED),1)
else ifeq ($(LUA_VERSION) $(detected_OS),5.3 FreeBSD)       # FreeBSD 
	cd Lua-5.3 && make freebsd
else ifeq ($(LUA_VERSION) $(detected_OS),5.3 Linux)         # Linux
	cd Lua-5.3 && make linux
else ifeq ($(LUA_VERSION) $(detected_OS),5.3 Darwin)        # Mac OSX
	cd Lua-5.3 && make macosx
else ifeq ($(LUA_VERSION) $(detected_OS),5.3 Windows)       # Windows
	cd Lua-5.3 && make mingw
	copy Lua-5.3\src\lua53.dll .
else ifeq ($(LUA_VERSION),5.3)                              # Generic
	cd Lua-5.3 && make generic  # <- could have posix here
else ifeq ($(detected_OS),Darwin)   # Mac OSX
	cd LuaJIT && make MACOSX_DEPLOYMENT_TARGET=10.9
else ifeq ($(detected_OS),Windows)  # Windows
	cd LuaJIT && make
	copy LuaJIT\src\lua51.dll .
else ifeq ($(detected_OS),FreeBSD)  #FreeBSD
	cd LuaJIT && gmake
else                                # *nix
	cd LuaJIT && make
endif

###

HashTk.o: hashtk/HashTk.cpp hashtk/HashTk.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

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

Parser.o: Parser.cpp Parser.h DateTimeInfo.o Expr.o ExprtkFns.o Getline.o HashTk.o LuaFns.o Lua.o Pagination.o SystemInfo.o TrackedInfo.o Variables.o 
	$(CXX) $(CXXFLAGS) -c -o $@ $<

WatchList.o: WatchList.cpp WatchList.h FileSystem.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Getline.o: Getline.cpp Getline.h ConsoleColor.o FileSystem.o Lolcat.o StrFns.o Consts.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Lolcat.o: Lolcat.cpp Lolcat.h FileSystem.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

LuaFns.o: LuaFns.cpp LuaFns.h Lua.o ConsoleColor.o ExprtkFns.o FileSystem.o Path.o Quoted.o Variables.o Consts.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Lua.o: Lua.cpp Lua.h StrFns.o LuaJIT/src/lua.hpp Lua-5.3/src/lua.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ExprtkFns.o: ExprtkFns.cpp ExprtkFns.h Expr.o FileSystem.o Quoted.o Variables.o Consts.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Expr.o: Expr.cpp Expr.h exprtk/exprtk.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

FileSystem.o: FileSystem.cpp FileSystem.h Path.o SystemInfo.o
	$(CXX) $(CXXFLAGS) -c -o $@ $<

Pagination.o: Pagination.cpp Pagination.h Path.o
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
ifeq ($(detected_OS) $(LUA_VERSION),Windows 5.3) # Windows with Lua 5.3
	@echo "Will need to manually copy/move 'lua53.dll', 'nsm.exe' "
	@echo "and 'nift.exe' to a location searched by your path "
	@echo "variable, for example try 'C:\Windows\System32'"
else ifeq ($(detected_OS),Windows)  # Windows with LuaJIT
	@echo "Will need to manually copy/move 'lua51.dll', 'nsm.exe' "
	@echo "and 'nift.exe' to a location searched by your path "
	@echo "variable, for example try 'C:\Windows\System32'"
else                                # FreeBSD/Linux/OSX/Posix/Unix
	mkdir -p ${BINDIR}
	chmod 755 nsm
	mv nift ${BINDIR}
	mv nsm ${BINDIR}
endif 

uninstall:
ifeq ($(detected_OS) $(LUA_VERSION),Windows 5.3) # Windows with Lua 5.3
	@echo "Will need to manually remove 'lua53.dll', 'nsm.exe' "
	@echo "and 'nift.exe' from install location, typically "
	@echo "'C:\Windows\System32'"
else ifeq ($(detected_OS),Windows)       # Windows
	@echo "Will need to manually remove 'lua51.dll', 'nsm.exe' "
	@echo "and 'nift.exe' from install location, typically "
	@echo "'C:\Windows\System32'"
else                                     # FreeBSD/Linux/OSX/Posix/Unix
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
ifeq ($(detected_OS),Windows)       # Windows
	del $(objects)
	cd Lua-5.3 && make clean
	#cd LuaJIT && make clean        #has been fixed in development version, soon!
else ifeq ($(detected_OS),FreeBSD)  # FreeBSD
	rm -f $(objects)
	cd Lua-5.3 && gmake clean
	cd LuaJIT && gmake clean
else                                # Linux/OSX/Posix/Unix
	rm -f $(objects)
	cd Lua-5.3 && make clean
	cd LuaJIT && make clean
endif 

clean-all: clean
ifeq ($(detected_OS),Windows)       # Windows
	del nsm.exe nift.exe lua51.dll
else                                # *nix
	rm -f nsm nift
endif 
