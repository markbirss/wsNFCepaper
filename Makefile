TARGET  := epnfcup
WARN    := -Wall
CFLAGS  := -O2 ${WARN} `pkg-config --cflags libnfc MagickWand`
LDFLAGS := `pkg-config --libs libnfc MagickWand`
CC      := gcc

C_SRCS    = $(wildcard *.c)
OBJ_FILES = $(C_SRCS:.c=.o)

all: ${TARGET}

%.o: %.c
	${CC} ${WARN} -c ${CFLAGS}  $< -o $@

mynfc: main.o
	${CC} ${WARN} ${LDFLAGS} -o $@ mynfc.o mifare.o

#${TARGET}: ${OBJ_FILES}
#	${CC} ${WARN} ${LDFLAGS} -o $@  $(OBJ_FILES)

clean:
	rm -rf *.o ${TARGET}

mrproper: clean
	rm -rf *~
