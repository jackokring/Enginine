all: build

build:
	cd ./Builds/LinuxMakefile && make
run: build
	cd ./Builds/LinuxMakefile/build && ./Enginine
user:
	./makeuser.py
juce:
	./makezlib.py
	../JUCE/extras/Projucer/Builds/LinuxMakefile/build/Projucer --resave Enginine.jucer
clean:
	cd ./Builds/LinuxMakefile && make clean
