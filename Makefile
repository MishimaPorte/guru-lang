SRC_FILES := $(wildcard ./*.c)
DIST := dist/guru
DIST_TEST := dist/guru_tests
TEST_FILES := $(wildcard test/*.c) $(filter-out ./main.c, $(wildcard ./*.c))
CC := gcc

compile:
	$(CC) -o $(DIST) $(SRC_FILES)

compile_test:
	$(CC) -o $(DIST_TEST) $(TEST_FILES) 

run: compile
	$(DIST) main.guru

test: compile_test
	$(DIST_TEST)
