DIST := dist/guru
DIST_TEST := dist/guru_tests
SRC_FILES := $(wildcard ./*.c)
TEST_FILES := $(wildcard test/*.c) $(filter-out ./main.c, $(wildcard ./*.c))
CC := gcc

.PHONY: compile
compile:
	$(CC) -o $(DIST) $(SRC_FILES)

.PHONY: compile_test
compile_test:
	$(CC) -o $(DIST_TEST) $(TEST_FILES) 

.PHONY: run
run: compile
	$(DIST) main.guru

.PHONY: test
test: compile_test
	$(DIST_TEST)
