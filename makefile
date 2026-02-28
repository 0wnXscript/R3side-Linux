CC := gcc

CFLAGS := -Wall -Wextra -g -Iinclude \
          -MMD -MP \
          `pkg-config --cflags gtk4`

LDFLAGS := `pkg-config --libs gtk4`

BUILD_DIR := build
SRC_DIR   := src

TARGET := $(BUILD_DIR)/Build

SRC := $(shell find $(SRC_DIR) -type f -name "*.c")
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

.PHONY: all clean re

all: $(TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	@echo "Build successful"
	@rm -f $(OBJ) $(DEP)  

-include $(DEP)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned."

re: clean all
