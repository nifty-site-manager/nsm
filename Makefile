#basic makefile for nsm
nsm.cpp: compile	

compile:
	g++ -std=c++11 nsm.cpp -o nsm

install:
	chmod +x ./nsm
	sudo mv ./nsm /usr/local/bin
