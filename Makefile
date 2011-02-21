all: clean configure compile

configure:
	CXXFLAGS=-I/usr/local/BerkeleyDB.5.1/include LINKFLAGS=-L/usr/local/BerkeleyDB.5.1/lib node-waf configure

compile:
	node-waf build

clean:
	node-waf clean
