NAME=hashtbl
LIB=lib${NAME}.a
CFLAGS=-Wall -O2

all: ${LIB}

${LIB}: ${NAME}.o
	ar r $@ ${NAME}.o

${NAME}.o: ${NAME}.c ${NAME}.h

demo: demo.c ${NAME}.o ${NAME}.h
	${CXX} ${CFLAGS} -o $@ -I. demo.c ${NAME}.o

install: ${LIB}
	install -C -m 644 ${LIB} /usr/local/lib
	install -C -m 644 ${NAME}.h /usr/local/include

clean:
	rm -f ${NAME}.o
	rm -f ${LIB}
