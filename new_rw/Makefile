CC ?= cc
AR ?= ar

BUILD_DIR ?= build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin

CPPFLAGS ?=
CPPFLAGS += -I. -Ivendor/glad/include -Ivendor/glfw/include
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic -O2
LDFLAGS ?=
LDLIBS ?= -lm

LIB := $(BUILD_DIR)/librw.a
LIB_SRCS := \
	rw_engine.c \
	rw_frame.c \
	rw_geometry.c \
	rw_gl.c \
	rw_material.c \
	rw_pipeline.c \
	rw_raster.c \
	rw_render.c \
	rw_scene.c \
	rw_skin.c
LIB_OBJS := $(LIB_SRCS:%.c=$(OBJ_DIR)/%.o)
LIB_DEPS := $(LIB_OBJS:.o=.d)

TEST_NAMES := math frame material geometry scene skin
TEST_BINS := $(TEST_NAMES:%=$(BIN_DIR)/test_%)

GL_TEST_NAMES := render im2d gta
GL_TEST_BINS := $(GL_TEST_NAMES:%=$(BIN_DIR)/test_%)
GL_TEST_LDLIBS := -lglfw -ldl

.PHONY: all test check clean

all: $(LIB) $(TEST_BINS)

$(LIB): $(LIB_OBJS) | $(BUILD_DIR)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c rw.h | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BIN_DIR)/test_%: tests/test_%.c $(LIB) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(GL_TEST_BINS): $(BIN_DIR)/test_%: tests/test_%.c $(LIB) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP $< $(LIB) $(LDFLAGS) $(LDLIBS) $(GL_TEST_LDLIBS) -o $@

test check: $(TEST_BINS) $(GL_TEST_BINS)
	@set -e; for test in $(TEST_BINS) $(GL_TEST_BINS); do printf '%s\n' "==> $$test"; "$$test"; done

$(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o *.a test_* tests/*.d tests/*.o

-include $(LIB_DEPS)
