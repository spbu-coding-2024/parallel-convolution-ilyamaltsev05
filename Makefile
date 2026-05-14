INCLUDE = include
BUILD = build
SRC = src
CC = gcc
CFLAGS = -c -O2
GLIB = -I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
LIBS = -lm -lpthread -lglib-2.0

.PHONY: all dir stb sequential buffer_by_col buffer_by_row buffer_by_pixel buffer_tiles test

all: dir stb sequential buffer_by_col buffer_by_row buffer_by_pixel buffer_tiles

dir:
	mkdir -p $(BUILD)

stb: $(BUILD)/stb.o

sequential: dir stb $(BUILD)/sequential

buffer_by_col: dir stb $(BUILD)/buffer_by_col

buffer_by_row: dir stb $(BUILD)/buffer_by_row

buffer_by_pixel: dir stb $(BUILD)/buffer_by_pixel

buffer_tiles: dir stb $(BUILD)/buffer_tiles

test: all
	cat ./data/image/bmps.part.* > ./data/image/bmps.tar.gz
	tar -xzf ./data/image/bmps.tar.gz
	python3 -m venv .venv
	.venv/bin/pip install -r requirements.txt
	.venv/bin/python3 reference.py

$(BUILD)/stb.o: $(INCLUDE)/stb.c $(INCLUDE)/stb_image.h $(INCLUDE)/stb_image_write.h
	$(CC) -c ./$(INCLUDE)/stb.c -I./$(INCLUDE) -o $(BUILD)/stb.o

$(BUILD)/pipeline:

$(BUILD)/pipeline.o: $(SRC)/reader.c $(SRC)/structs.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) -I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/ ./$(SRC)/reader.c -o ./$(BUILD)/pipeline.o

$(BUILD)/sequential: $(BUILD)/stb.o $(SRC)/sequential.c $(SRC)/structs.h $(BUILD)/pipeline.o
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) ./$(SRC)/sequential.c -o $(BUILD)/sequential.o
	$(CC) ./$(BUILD)/sequential.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/sequential $(LIBS)

$(BUILD)/buffer_by_col: $(BUILD)/stb.o $(SRC)/buffer_by_col.c
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) -fopenmp ./$(SRC)/buffer_by_col.c -o $(BUILD)/buffer_by_col.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_col.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/buffer_by_col $(LIBS)

$(BUILD)/buffer_by_row: $(BUILD)/stb.o $(SRC)/buffer_by_row.c $(SRC)/structs.h $(BUILD)/pipeline.o
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) -fopenmp ./$(SRC)/buffer_by_row.c -o $(BUILD)/buffer_by_row.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_row.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/buffer_by_row $(LIBS)

$(BUILD)/buffer_by_pixel: $(BUILD)/stb.o $(SRC)/buffer_by_pixel.c $(SRC)/structs.h $(BUILD)/pipeline.o
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) -fopenmp ./$(SRC)/buffer_by_pixel.c -o $(BUILD)/buffer_by_pixel.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_pixel.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/buffer_by_pixel $(LIBS)

$(BUILD)/buffer_tiles: $(BUILD)/stb.o $(SRC)/buffer_tiles.c $(SRC)/structs.h $(BUILD)/pipeline.o
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) -fopenmp ./$(SRC)/buffer_tiles.c -o $(BUILD)/buffer_tiles.o
	$(CC) -fopenmp ./$(BUILD)/buffer_tiles.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/buffer_tiles $(LIBS)
