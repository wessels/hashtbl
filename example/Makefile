INCDIRS?=
LDDIRS?=

all: top-n-strings


top-n-strings: top-n-strings.c
	${CC} -O2 ${INCDIRS} -o $@ ${@}.c ${LDDIRS} -lhashtbl

clean:
	rm -f top-n-strings
