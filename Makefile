MAIN=graph.c
OUTPUT=graph
FLAGS=-Wall -Werror -Wextra
CC=gcc -std=c11
DEBUG=graph_debug.out
RELEASE=graph_release.otu
build:$(MAIN)
	$(CC)   $^ -o $(DEBUG) -D DEBUG && ./$(DEBUG)
release:$(MAIN)
	$(CC) $^ -o $(RELEASE) -D RELEASE && ./$(RELEASE)
clean:
	rm -f $(DEBUG)
	rm -f $(RELEASE)
log:
	gcc -std=c11 -Wall -Werror -Wextra graph.c -o ../build/graph_log -D LOG


