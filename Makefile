INCLUDE = include
BUILD = build
SRC = src
CC = gcc
CFLAGS = -c -O2
GLIB = -I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/
LIBS = -lm -lpthread -lglib-2.0

.PHONY: all dir stb test pipeline seqload

all: dir stb pipeline seqload

dir:
	mkdir -p $(BUILD)

stb: $(BUILD)/stb.o

pipeline: dir stb $(BUILD)/pipeline_seq $(BUILD)/pipeline_row $(BUILD)/pipeline_col $(BUILD)/pipeline_pixel $(BUILD)/pipeline_tiles

seqload: dir stb $(BUILD)/seq_seq $(BUILD)/seq_row $(BUILD)/seq_col $(BUILD)/seq_pixel $(BUILD)/seq_tiles

test: all
	cat ./data/image/bmps.part.* > ./data/image/bmps.tar.gz
	tar -xzf ./data/image/bmps.tar.gz
	python3 -m venv .venv
	.venv/bin/pip install -r requirements.txt
	.venv/bin/python3 perf.py

$(BUILD)/stb.o: $(INCLUDE)/stb.c $(INCLUDE)/stb_image.h $(INCLUDE)/stb_image_write.h
	$(CC) -c ./$(INCLUDE)/stb.c -I./$(INCLUDE) -o $(BUILD)/stb.o

$(BUILD)/seqload.o: $(SRC)/seqload.c
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(SRC)/seqload.c -o $(BUILD)/seqload.o

$(BUILD)/pipeline.o: $(SRC)/pipeline.c $(SRC)/structs.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) ./$(SRC)/pipeline.c -o ./$(BUILD)/pipeline.o

$(BUILD)/sequential.o: $(SRC)/sequential.c $(SRC)/structs.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) ./$(SRC)/sequential.c -o $(BUILD)/sequential.o

$(BUILD)/buffer_by_col.o: $(SRC)/buffer_by_col.c $(SRC)/structs.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) -fopenmp ./$(SRC)/buffer_by_col.c -o $(BUILD)/buffer_by_col.o

$(BUILD)/buffer_by_row.o: $(SRC)/buffer_by_row.c $(SRC)/structs.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) -fopenmp ./$(SRC)/buffer_by_row.c -o $(BUILD)/buffer_by_row.o

$(BUILD)/buffer_by_pixel.o: $(SRC)/buffer_by_pixel.c $(SRC)/structs.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) -fopenmp ./$(SRC)/buffer_by_pixel.c -o $(BUILD)/buffer_by_pixel.o

$(BUILD)/buffer_tiles.o: $(SRC)/buffer_tiles.c $(SRC)/structs.h
	$(CC) $(CFLAGS) -I./$(INCLUDE) $(GLIB) -fopenmp ./$(SRC)/buffer_tiles.c -o $(BUILD)/buffer_tiles.o

$(BUILD)/pipeline_seq: $(BUILD)/sequential.o $(BUILD)/pipeline.o $(BUILD)/stb.o
	$(CC) ./$(BUILD)/sequential.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/pipeline_seq $(LIBS)

$(BUILD)/pipeline_row: $(BUILD)/buffer_by_row.o $(BUILD)/pipeline.o $(BUILD)/stb.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_row.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/pipeline_row $(LIBS)

$(BUILD)/pipeline_col: $(BUILD)/buffer_by_col.o $(BUILD)/pipeline.o $(BUILD)/stb.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_col.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/pipeline_col $(LIBS)

$(BUILD)/pipeline_pixel: $(BUILD)/buffer_by_pixel.o $(BUILD)/pipeline.o $(BUILD)/stb.o
	$(CC) -fopenmp ./$(BUILD)/buffer_by_pixel.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/pipeline_pixel $(LIBS)

$(BUILD)/pipeline_tiles: $(BUILD)/buffer_tiles.o $(BUILD)/pipeline.o $(BUILD)/stb.o
	$(CC) -fopenmp ./$(BUILD)/buffer_tiles.o ./$(BUILD)/stb.o ./$(BUILD)/pipeline.o -o $(BUILD)/pipeline_tiles $(LIBS)

$(BUILD)/seq_seq: $(BUILD)/sequential.o $(BUILD)/seqload.o $(BUILD)/stb.o
	$(CC) ./$(BUILD)/seqload.o ./$(BUILD)/sequential.o ./$(BUILD)/stb.o -o $(BUILD)/seq_seq $(LIBS)

$(BUILD)/seq_row: $(BUILD)/buffer_by_row.o $(BUILD)/seqload.o $(BUILD)/stb.o
	$(CC) -fopenmp ./$(BUILD)/seqload.o ./$(BUILD)/buffer_by_row.o ./$(BUILD)/stb.o -o $(BUILD)/seq_row $(LIBS)

$(BUILD)/seq_col: $(BUILD)/buffer_by_col.o $(BUILD)/seqload.o $(BUILD)/stb.o
	$(CC) -fopenmp ./$(BUILD)/seqload.o ./$(BUILD)/buffer_by_col.o ./$(BUILD)/stb.o -o $(BUILD)/seq_col $(LIBS)

$(BUILD)/seq_pixel: $(BUILD)/buffer_by_pixel.o $(BUILD)/seqload.o $(BUILD)/stb.o
	$(CC) -fopenmp ./$(BUILD)/seqload.o ./$(BUILD)/buffer_by_pixel.o ./$(BUILD)/stb.o -o $(BUILD)/seq_pixel $(LIBS)

$(BUILD)/seq_tiles: $(BUILD)/buffer_tiles.o $(BUILD)/seqload.o $(BUILD)/stb.o
	$(CC) -fopenmp ./$(BUILD)/seqload.o ./$(BUILD)/buffer_tiles.o ./$(BUILD)/stb.o -o $(BUILD)/seq_tiles $(LIBS)
