CXXFLAGS	= -std=c++17 -Igoogletest/googletest/include -I../src
CFLAGS		= -I../src
LFLAGS		= -Lout/lib -lgtest -lgtest_main -lpthread
LIBGTEST	= out/lib/libgtest.a
OBJS			= test.o serial.o hid.o hid_dualshock3.o hid_keyboard.o \
  hid_guncon3.o hid_switch.o hid_xbox.o mock.o

test: ${LIBGTEST} ${OBJS}
	clang++ -o test ${OBJS} ${LFLAGS}

clean:
	rm -rf out *.o test

%.o: ../src/%.c ../src/*.h
	clang -c ${CFLAGS} -o $@ $<

%.o: %.cc *.h
	clang++ -c ${CXXFLAGS} -o $@ $<

${LIBGTEST}:
	(cd googletest && cmake . -B ../out && cd ../out && make)
