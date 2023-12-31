DIST := dist/guru
DIST_TEST := dist/guru_tests
SRC_FILES := $(wildcard ./*.c)
TEST_FILES := $(wildcard test/*.c) $(filter-out ./main.c, $(wildcard ./*.c))
CC := gcc

.PHONY: clean
clean:
	rm -rf coredump*

.PHONY: compile
compile_debug:
	$(CC) -o $(DIST) $(SRC_FILES) -D DEBUG_MODE -ggdb

.PHONY: compile
compile:
	$(CC) -o $(DIST) $(SRC_FILES)

.PHONY: compile_test
compile_test:
	$(CC) -o $(DIST_TEST) $(TEST_FILES) 

.PHONY: run_debug
run_debug: clean compile_debug
	$(DIST) main.guru

.PHONY: run
run: compile
	$(DIST) main.guru

.PHONY: test
test: compile_test
	$(DIST_TEST)
