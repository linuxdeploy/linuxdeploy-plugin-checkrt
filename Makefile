CFLAGS ?= -O2 -Wall -Wextra
LDFLAGS += -s
LIB = exec.so
EXEC_TEST = exec_test
ENV_TEST = env_test

checkrt: $(LIB)

test: $(EXEC_TEST) $(ENV_TEST)

all: checkrt test

clean:
	-rm -f $(LIB) $(EXEC_TEST) $(ENV_TEST) *.o

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

.PHONY: checkrt test run_tests all clean

tarball: checkrt
	mkdir usr/optional/ -p
	cp exec.so usr/optional/
	tar cfvz checkrt.tar.gz usr/ AppRun.sh
	rm -rf usr/
