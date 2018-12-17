CC = gcc
CCFLAGS = -g -std=c99 -Wall
EXEC = get
SRC = get.c

${EXEC}: ${SRC}
	${CC} ${CCFLAGS} ${SRC} -o ${EXEC}
	chmod u+s ${EXEC}

clean:
	rm ${EXEC}
