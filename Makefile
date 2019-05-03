CC=clang
VERSION=0.1.1

all: build

clean:
	rm -f caffeine

build: clean
	${CC} caffeine.c -o caffeine -DCAFFEINE_VERSION=\"${VERSION}\"


