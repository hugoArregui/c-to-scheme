# using gnu17 instead of c17 due to: https://stackoverflow.com/questions/71801110/how-to-fix-error-or-warning-in-c-implicit-declaration-of-function-inet-aton
CFLAGS = -Wall -Wextra -std=gnu17 -pedantic -ggdb -Wno-gnu-empty-initializer -Wno-unused-function
# CFLAGS += `pkg-config --cflags libsodium`
# LIBS = -lpthread
# LIBS += `pkg-config --libs libsodium`

build: clean
	@mkdir -p build/
	$(CC) $(CFLAGS) $(DEBUG) $(LIBS) -o build/main src/main.c
	$(CC) $(CFLAGS) $(DEBUG) $(LIBS) -o build/test_parser src/test_parser.c
	$(CC) $(CFLAGS) $(DEBUG) $(LIBS) -o build/test test.c

format:
	clang-format --sort-includes -i src/*.c src/*.h

clean:
	rm -f build/*

.DEFAULT_GOAL := build
.PHONY: build
