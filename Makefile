all: build

build:
	cd ./Builds/LinuxMakefile && make && cd ../..
run:
	cd ./Builds/LinuxMakefile/build && ./Enginine
zlib:
	./makezlib.py
user:
	./makeuser.py
clean:
	cd ./Builds/LinuxMakefile && make clean && cd ../..
