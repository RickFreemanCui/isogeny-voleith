CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -std=c11 -I.
LDFLAGS  =

BUILD_DIR = build

SRCS  = field.c fp2.c hash.c prg.c bavc.c rs_code.c vole_commit.c itmac.c quicksilver.c constraints.c protocol.c
OBJS  = $(SRCS:%.c=$(BUILD_DIR)/%.o)

TESTS = test_field test_fp2 test_bavc test_rs test_protocol test_itmac test_quicksilver
TEST_BINS = $(TESTS:%=$(BUILD_DIR)/%)

.PHONY: all test clean

all: $(OBJS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR) $(BUILD_DIR)/test

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# --- test field ---
$(BUILD_DIR)/test_field: $(BUILD_DIR)/field.o test/test_field.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) test/test_field.c $(BUILD_DIR)/field.o -o $@

# --- test fp2 ---
$(BUILD_DIR)/test_fp2: $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o test/test_fp2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) test/test_fp2.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o -o $@

# --- test bavc ---
$(BUILD_DIR)/test_bavc: $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/hash.o $(BUILD_DIR)/prg.o $(BUILD_DIR)/bavc.o test/test_bavc.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) test/test_bavc.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/hash.o $(BUILD_DIR)/prg.o $(BUILD_DIR)/bavc.o -o $@

# --- run targets ---
test: $(TEST_BINS)
	@failed=0; for t in $(TEST_BINS); do echo "=== $$t ==="; $$t || failed=1; done; \
	if [ $$failed -eq 0 ]; then echo "All tests passed."; else echo "SOME TESTS FAILED"; exit 1; fi

test_field: $(BUILD_DIR)/test_field
	$(BUILD_DIR)/test_field

test_fp2: $(BUILD_DIR)/test_fp2
	$(BUILD_DIR)/test_fp2

test_bavc: $(BUILD_DIR)/test_bavc
	$(BUILD_DIR)/test_bavc

# --- test rs ---
$(BUILD_DIR)/test_rs: $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/rs_code.o test/test_rs.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) test/test_rs.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/rs_code.o -o $@

test_rs: $(BUILD_DIR)/test_rs
	$(BUILD_DIR)/test_rs

# --- test protocol ---
$(BUILD_DIR)/test_protocol: $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/hash.o $(BUILD_DIR)/prg.o $(BUILD_DIR)/bavc.o $(BUILD_DIR)/rs_code.o $(BUILD_DIR)/vole_commit.o $(BUILD_DIR)/itmac.o $(BUILD_DIR)/quicksilver.o $(BUILD_DIR)/constraints.o $(BUILD_DIR)/protocol.o test/test_protocol.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) test/test_protocol.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/hash.o $(BUILD_DIR)/prg.o $(BUILD_DIR)/bavc.o $(BUILD_DIR)/rs_code.o $(BUILD_DIR)/vole_commit.o $(BUILD_DIR)/itmac.o $(BUILD_DIR)/quicksilver.o $(BUILD_DIR)/constraints.o $(BUILD_DIR)/protocol.o -o $@

test_protocol: $(BUILD_DIR)/test_protocol
	$(BUILD_DIR)/test_protocol

# --- test itmac ---
$(BUILD_DIR)/test_itmac: $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/itmac.o test/test_itmac.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) test/test_itmac.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/itmac.o -o $@

test_itmac: $(BUILD_DIR)/test_itmac
	$(BUILD_DIR)/test_itmac

# --- test quicksilver ---
$(BUILD_DIR)/test_quicksilver: $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/itmac.o $(BUILD_DIR)/quicksilver.o test/test_quicksilver.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) test/test_quicksilver.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/itmac.o $(BUILD_DIR)/quicksilver.o -o $@

test_quicksilver: $(BUILD_DIR)/test_quicksilver
	$(BUILD_DIR)/test_quicksilver

clean:
	rm -rf $(BUILD_DIR)
