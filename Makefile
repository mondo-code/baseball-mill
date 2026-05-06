CC = cc
CFLAGS = -Ivendor -Wall -g 
LDFLAGS = -Lvendor/lib -lm -lSDL2 -lSDL2_ttf

SRCS = $(wildcard src/*.c)
TEST_SRC = $(wildcard tests/*.c) 

OBJECTS = $(SRCS:.c=.o)
TEST_OBJ = $(TEST_SRC:.c=.o)

TARGET = bin/baseballmill
TEST_BIN = bin/test_runner

.PHONY: all test clean

all: $(TARGET) 

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_BIN): $(filter-out %/main.o,$(OBJECTS)) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BIN)
	@echo "Executing unit tests..."
	./$(TEST_BIN) -r 

clean:
	rm -f $(OBJECTS) $(TEST_OBJ) $(TARGET) $(TEST_BIN) 
