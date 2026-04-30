INCLUDE = include
BUILD = build
SRC = src
CC = gcc
CFLAGS = -c -O2

.PHONY: all dir stb sequential test

all: dir stb sequential

dir:
	mkdir -p $(BUILD)

stb: $(BUILD)/stb.o

sequential: dir stb $(BUILD)/sequential

test: all
	cat ./data/image/bmps.part.* > ./data/image/bmps.tar.gz
	tar -xzf ./data/image/bmps.tar.gz
	python3 -m venv .venv
	.venv/bin/pip install -r requirements.txt
	.venv/bin/python3 reference.py

$(BUILD)/stb.o: $(INCLUDE)/stb.c $(INCLUDE)/stb_image.h $(INCLUDE)/stb_image_write.h
	gcc -c ./$(INCLUDE)/stb.c -I./$(INCLUDE) -o $(BUILD)/stb.o

$(BUILD)/sequential: $(BUILD)/stb.o $(SRC)/sequential.c
	$(CC) $(CFLAGS) -I./$(INCLUDE) ./$(SRC)/sequential.c -o $(BUILD)/sequential.o
	gcc ./$(BUILD)/sequential.o ./$(BUILD)/stb.o -o $(BUILD)/sequential -lm
