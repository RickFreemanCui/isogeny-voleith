CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -std=c11 -I.
LDFLAGS  =

BUILD_DIR = build

SRCS  = field.c fp2.c hash.c prg.c bavc.c rs_code.c vole_commit.c itmac.c quicksilver.c constraints.c protocol.c shake.c aes128.c
OBJS  = $(SRCS:%.c=$(BUILD_DIR)/%.o)

# XKCP reference Keccak
KECCAK_SRC = xkcp/lib/low/KeccakP-1600/ref-64bits/KeccakP-1600-reference.c
KECCAK_OBJ = $(BUILD_DIR)/keccak.o
KECCAK_INC = -Ixkcp/lib/common -Ixkcp/lib/low/KeccakP-1600/ref-64bits -Ixkcp/lib/low/KeccakP-1600/ref-64bits/SnP

# Common objects needed by most tests
CORE_OBJS = $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/aes128.o $(BUILD_DIR)/shake.o $(KECCAK_OBJ)
ALL_OBJS  = $(CORE_OBJS) $(BUILD_DIR)/hash.o $(BUILD_DIR)/prg.o $(BUILD_DIR)/bavc.o \
            $(BUILD_DIR)/rs_code.o $(BUILD_DIR)/vole_commit.o $(BUILD_DIR)/itmac.o \
            $(BUILD_DIR)/quicksilver.o $(BUILD_DIR)/constraints.o $(BUILD_DIR)/protocol.o

TESTS = test_field test_fp2 test_bavc test_rs test_protocol test_itmac test_quicksilver
TEST_BINS = $(TESTS:%=$(BUILD_DIR)/%)

.PHONY: all test clean

all: $(OBJS) $(KECCAK_OBJ)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR) $(BUILD_DIR)/test

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# XKCP reference Keccak (needs stub for debug header + SnP includes)
$(KECCAK_OBJ): $(KECCAK_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(KECCAK_INC) -c $(KECCAK_SRC) -o $@

$(BENCH_KECCAK): $(KECCAK_SRC) | $(BUILD_DIR)
	$(CC) $(BENCH_CFLAGS) $(KECCAK_INC) -c $(KECCAK_SRC) -o $@

# --- test rules ---
$(BUILD_DIR)/test_field: $(CORE_OBJS) test/test_field.c
	$(CC) $(CFLAGS) test/test_field.c $(BUILD_DIR)/field.o -o $@

$(BUILD_DIR)/test_fp2: $(CORE_OBJS) test/test_fp2.c
	$(CC) $(CFLAGS) test/test_fp2.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o -o $@

$(BUILD_DIR)/test_bavc: $(ALL_OBJS) test/test_bavc.c
	$(CC) $(CFLAGS) test/test_bavc.c $(ALL_OBJS) -o $@

$(BUILD_DIR)/test_rs: $(CORE_OBJS) $(BUILD_DIR)/rs_code.o test/test_rs.c
	$(CC) $(CFLAGS) test/test_rs.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/rs_code.o -o $@

$(BUILD_DIR)/test_protocol: $(ALL_OBJS) test/test_protocol.c
	$(CC) $(CFLAGS) test/test_protocol.c $(ALL_OBJS) -o $@

$(BUILD_DIR)/test_itmac: $(CORE_OBJS) $(BUILD_DIR)/itmac.o test/test_itmac.c
	$(CC) $(CFLAGS) test/test_itmac.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/itmac.o -o $@

$(BUILD_DIR)/test_quicksilver: $(CORE_OBJS) $(BUILD_DIR)/itmac.o $(BUILD_DIR)/quicksilver.o test/test_quicksilver.c
	$(CC) $(CFLAGS) test/test_quicksilver.c $(BUILD_DIR)/field.o $(BUILD_DIR)/fp2.o $(BUILD_DIR)/itmac.o $(BUILD_DIR)/quicksilver.o -o $@

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

test_rs: $(BUILD_DIR)/test_rs
	$(BUILD_DIR)/test_rs

test_protocol: $(BUILD_DIR)/test_protocol
	$(BUILD_DIR)/test_protocol

test_itmac: $(BUILD_DIR)/test_itmac
	$(BUILD_DIR)/test_itmac

test_quicksilver: $(BUILD_DIR)/test_quicksilver
	$(BUILD_DIR)/test_quicksilver

# --- benchmark (max optimisation) ---
BENCH_CFLAGS = -Wall -Wextra -O3 -std=c11 -I. -mcpu=native -flto
BENCH_OBJS   = $(SRCS:%.c=$(BUILD_DIR)/bench_%.o)
BENCH_KECCAK = $(BUILD_DIR)/bench_keccak.o

$(BUILD_DIR)/bench_%.o: %.c | $(BUILD_DIR)
	$(CC) $(BENCH_CFLAGS) -c $< -o $@

$(BENCH_KECCAK): $(KECCAK_SRC) | $(BUILD_DIR)
	$(CC) $(BENCH_CFLAGS) $(KECCAK_INC) -include displayIntermediateValues.h -c $(KECCAK_SRC) -o $@

$(BUILD_DIR)/test_benchmark: $(BENCH_OBJS) $(BENCH_KECCAK) test/test_benchmark.c | $(BUILD_DIR)
	$(CC) $(BENCH_CFLAGS) test/test_benchmark.c $(BENCH_OBJS) $(BENCH_KECCAK) -o $@

bench: $(BUILD_DIR)/test_benchmark
	$(BUILD_DIR)/test_benchmark

clean:
	rm -rf $(BUILD_DIR)
