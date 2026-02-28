CC := gcc
CFLAGS := -fPIC -Wall -Wextra -Iinclude
LDFLAGS := -shared -ldl

BUILD_DIR := build
SRC_DIR := src

TARGET := $(BUILD_DIR)/R3lib.so

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

.PHONY: all clean
.DELETE_ON_ERROR:

all: $(TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	@rm -f $(OBJ)

clean:
	@rm -rf $(BUILD_DIR)