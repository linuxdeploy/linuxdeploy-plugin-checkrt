CFLAGS ?= -O2 -Wall -Wextra
LDFLAGS += -s
LIB = exec.so
EXEC_TEST = exec_test
ENV_TEST = env_test
BIN = checkrt copy_libs

all: $(BIN) $(LIB) test

clean:
	-rm -f $(BIN) $(LIB) $(EXEC_TEST) $(ENV_TEST) *.o

checkrt: checkrt.o env.o
	$(CC) $(LDFLAGS) -o $@ $^ -ldl

copy_libs: copy_libs.o
	$(CC) $(LDFLAGS) -o $@ $^ -ldl

test: $(EXEC_TEST) $(ENV_TEST)

$(LIB): exec.o env.o
	$(CC) -shared $(LDFLAGS) -o $@ $^ -ldl

exec.o env.o: CFLAGS += -fPIC

$(EXEC_TEST): CFLAGS += -DEXEC_TEST
$(EXEC_TEST): exec.c env.c
	$(CC) -o $@ $(CFLAGS) $^ -ldl

$(ENV_TEST): CFLAGS += -DENV_TEST
$(ENV_TEST): env.c
	$(CC) -o $@ $(CFLAGS) $^

run_tests: $(EXEC_TEST) $(ENV_TEST)
	./$(ENV_TEST)
	./$(EXEC_TEST)

.PHONY: $(BIN) test run_tests all clean

tarball: $(BIN) $(LIB)
	mkdir -p checkrt.d/
	cp checkrt $(LIB) checkrt.d/
	tar cfvz checkrt.tar.gz copy_libs apprun-hooks/ checkrt.d/
	rm -rf checkrt.d/
