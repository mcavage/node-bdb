all: clean configure compile test

configure:
	CXXFLAGS=-I/usr/local/BerkeleyDB.5.1/include LINKFLAGS=-L/usr/local/BerkeleyDB.5.1/lib node-waf configure

compile:
	node-waf build

test:
	node-waf tests

clean:
	node-waf clean
