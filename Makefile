INCLUDE = include
BUILD = build
SRC = src
CC = gcc
CFLAGS = -c -O2

.PHONY: all dir stb sequential buffer_by_col buffer_by_row buffer_by_pixel buffer_random_grid test

all: dir stb sequential buffer_by_col buffer_by_row buffer_by_pixel buffer_random_grid

dir:
	mkdir -p $(BUILD)

stb: $(BUILD)/stb.o

sequential: dir stb $(BUILD)/sequential

buffer_by_col: dir stb $(BUILD)/buffer_by_col

buffer_by_row: dir stb $(BUILD)/buffer_by_row

buffer_by_pixel: dir stb $(BUILD)/buffer_by_pixel

buffer_random_grid: dir stb $(BUILD)/buffer_random_grid

test: all
	cat ./data/image/bmps.part.* > ./data/image/bmps.tar.gz
	tar -xzf ./data/image/bmps.tar.gz
	python3 -m venv .venv
	.venv/bin/pip install -r requirements.txt
	.venv/bin/python3 reference.py

$(BUILD)/stb.o: $(INCLUDE)/stb.c $(INCLUDE)/stb_image.h $(INCLUDE)/stb_image_write.h
	$(CC) -c ./$(INCLUDE)/stb.c -I./$(INCLUDE) -o $(BUILD)/stb.o

$(BUILD)/sequential: $(BUILD)/stb.o $(SRC)/sequential.c
	$(CC) $(CFLAGS) -I./$(INCLUDE) ./$(SRC)/sequential.c -o $(BUILD)/sequential.o
	$(CC) ./$(BUILD)/sequential.o ./$(BUILD)/stb.o -o $(BUILD)/sequential -lm

$(BUILD)/buffer_by_col: $(BUILD)/stb.o $(SRC)/buffer_by_col.c
	$(CC) $(CFLAGS) -I./$(INCLUDE) -fopenmp ./$(SRC)/buffer_by_col.c -o $(BUILD)/buffer_by_col.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_col.o ./$(BUILD)/stb.o -o $(BUILD)/buffer_by_col -lm

$(BUILD)/buffer_by_row: $(BUILD)/stb.o $(SRC)/buffer_by_row.c
	$(CC) $(CFLAGS) -I./$(INCLUDE) -fopenmp ./$(SRC)/buffer_by_row.c -o $(BUILD)/buffer_by_row.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_row.o ./$(BUILD)/stb.o -o $(BUILD)/buffer_by_row -lm

$(BUILD)/buffer_by_pixel: $(BUILD)/stb.o $(SRC)/buffer_by_pixel.c
	$(CC) $(CFLAGS) -I./$(INCLUDE) -fopenmp ./$(SRC)/buffer_by_pixel.c -o $(BUILD)/buffer_by_pixel.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_pixel.o ./$(BUILD)/stb.o -o $(BUILD)/buffer_by_pixel -lm

$(BUILD)/buffer_random_grid: $(BUILD)/stb.o $(SRC)/buffer_random_grid.c
	$(CC) $(CFLAGS) -I./$(INCLUDE) -fopenmp ./$(SRC)/buffer_random_grid.c -o $(BUILD)/buffer_random_grid.o
	$(CC) -fopenmp ./$(BUILD)/buffer_random_grid.o ./$(BUILD)/stb.o -o $(BUILD)/buffer_random_grid -lm
