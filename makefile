CC = gcc
SRC = main.c kv.c
BFLAGS = -Ofast -march=native
DFLAGS = -O0 -g --debug -fsanitize=address -DDEBUG
TAR = main
release: $(SRC)
	$(CC) $(SRC) $(BFLAGS) -o $(TAR)

debug: $(SRC)
	$(CC) $(SRC) $(DFLAGS) -o $(TAR)

test: config.kv main
	./main config.kv

clean:
	rm -rf $(TAR)