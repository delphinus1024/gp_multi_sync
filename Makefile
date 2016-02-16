PROGRAM = gp_multi_sync
CXXFLAGS=-I/usr/local/include
LDFLAGS=-L/usr/local/lib -lgphoto2 -lgphoto2_port -lm
OBJECTS=$(PROGRAM).o

.SUFFIXES: .cpp .o

$(PROGRAM): $(OBJECTS)
	$(CXX)  $^ $(LDFLAGS)  -o $(PROGRAM)

.c.o:
	$(CXX) -c $(CXXFLAGS) $<

.PHONY:clean
clean:
	rm -f $(PROGRAM) *.o
