# OCCE Makefile
CC = gcc
TARGET = occe
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
PLUGIN_DIR = plugins

# Try to detect system Lua first, fall back to bundled version
LUA_CFLAGS := $(shell pkg-config --cflags lua5.4 2>/dev/null || pkg-config --cflags lua 2>/dev/null)
LUA_LDFLAGS := $(shell pkg-config --libs lua5.4 2>/dev/null || pkg-config --libs lua 2>/dev/null)

# If pkg-config didn't find Lua, use bundled version
ifeq ($(LUA_CFLAGS),)
    LUA_DIR = lua-5.4.7/install
    LUA_CFLAGS = -I$(LUA_DIR)/include
    LUA_LDFLAGS = -L$(LUA_DIR)/lib -llua
    $(info Using bundled Lua from $(LUA_DIR))
else
    $(info Using system Lua)
endif

CFLAGS = -Wall -Wextra -std=c11 -Iinclude $(LUA_CFLAGS) -Os -flto
LDFLAGS = $(LUA_LDFLAGS) -lm -ldl

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Debug build
DEBUG_CFLAGS = -Wall -Wextra -std=c11 -Iinclude -g -O0 -DDEBUG

.PHONY: all clean debug install uninstall run

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	strip $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS = $(DEBUG_CFLAGS)
debug: $(BUILD_DIR) $(OBJS)
	$(CC) $(DEBUG_CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

install: $(TARGET)
	@echo "Installing occe binary..."
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Setting up user configuration directory..."
	mkdir -p $(HOME)/.config/occe/plugins
	@echo "Copying plugins..."
	cp -r $(PLUGIN_DIR)/* $(HOME)/.config/occe/plugins/
	@if [ ! -f "$(HOME)/.config/occe/init.lua" ]; then \
		echo "Creating default init.lua..."; \
		cp init.lua.example $(HOME)/.config/occe/init.lua; \
	else \
		echo "init.lua already exists, skipping..."; \
	fi
	@echo "Installation complete!"
	@echo "Configuration: $(HOME)/.config/occe/"

uninstall:
	@echo "Removing occe binary..."
	rm -f /usr/local/bin/$(TARGET)
	@echo "Note: Configuration directory $(HOME)/.config/occe/ was NOT removed"
	@echo "Remove manually if desired: rm -rf $(HOME)/.config/occe/"

run: $(TARGET)
	./$(TARGET)
