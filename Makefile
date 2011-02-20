all: clean configure build

configure:
	CXXFLAGS=-I/usr/local/BerkeleyDB.5.1/include LINKFLAGS=-L/usr/local/BerkeleyDB.5.1/lib node-waf configure

build:
	node-wav build

clean:
	node-waf clean
