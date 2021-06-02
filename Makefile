INCLUDE:=-Iinclude \
    -I3party/zlog/src \
    -I3party \

SOURCE:=${wildcard source/*.c} ${wildcard 3party/zlog/src/*.c} ${wildcard 3party/cjson/*.c}

CFLAGS:= -w #-fpermissive

LDFLAGS:=-lpthread -lcurl

all:
	make clean
	make obj;make shared
	make clean

obj:
	mkdir -p objs
	gcc -c -fPIC ${INCLUDE} ${CFLAGS} ${SOURCE} ${LDFLAGS}
	mv *.o objs/

shared:
	make obj
	mkdir -p lib
	gcc --shared objs/*.o -o lib/libsimple_speech.so

all:
	make shared

install:
	mkdir -p /usr/local/simple_speech/include
	mkdir -p /usr/local/simple_speech/lib
	cp -rf include/* /usr/local/simple_speech/include
	cp -rf lib/* /usr/local/simple_speech/lib

test:
	gcc -g ${INCLUDE} ${SOURCE} ${CFLAGS} ${LDFLAGS} tests/test.c -o tests/a.out

clean:
	rm -rf objs
	rm -rf *.o
