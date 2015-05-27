#
# DX I/O library: libRBIO.a
#

ROBOIO_OBJECTS = io.o common.o pwm.o pwmdx.o spi.o spidx.o i2c.o i2cdx.o
ROBOIO_COMMINC = libsrc/defines.h libsrc/io.h libsrc/common.h libsrc/dxio.h
OPTIONS = -Wall -Wno-write-strings -m32

libRBIO.a : $(ROBOIO_OBJECTS)
	ar rcs libRBIO.a $(ROBOIO_OBJECTS)

io.o: libsrc/io.cpp $(ROBOIO_COMMINC)
	gcc -c libsrc/io.cpp $(OPTIONS)

common.o: libsrc/common.cpp $(ROBOIO_COMMINC)
	gcc -c libsrc/common.cpp $(OPTIONS)

pwm.o pwmdx.o: libsrc/pwm.cpp libsrc/pwmdx.cpp libsrc/pwm.h libsrc/pwmdx.h $(ROBOIO_COMMINC)
	gcc -c libsrc/pwm.cpp $(OPTIONS)
	gcc -c libsrc/pwmdx.cpp $(OPTIONS)

spi.o spidx.o: libsrc/spi.cpp libsrc/spidx.cpp libsrc/spi.h libsrc/spidx.h $(ROBOIO_COMMINC)
	gcc -c libsrc/spi.cpp $(OPTIONS)
	gcc -c libsrc/spidx.cpp $(OPTIONS)

i2c.o i2cdx.o: libsrc/i2c.cpp libsrc/i2cdx.cpp libsrc/i2c.h libsrc/i2cdx.h $(ROBOIO_COMMINC)
	gcc -c libsrc/i2c.cpp $(OPTIONS)
	gcc -c libsrc/i2cdx.cpp $(OPTIONS)

.PHONY : clean
clean :
	-rm libRBIO.a $(ROBOIO_OBJECTS)

