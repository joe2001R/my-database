CC:=gcc
CFLAGS:=-Wall $(EFLAGS)

SRC_DIRS := ./src
BUILD_DIR := ./build
SRCS := $(shell find $(SRCS_DIRS) -name '*.c')

OBJS := $(SRCS:$(SRC_DIRS)/%.c=$(BUILD_DIR)/%.o)

DEPS := $(OBJS:.o=.d)
INC_DIRS := include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS :=$(INC_FLAGS) -MMD -MP

LDFLAGS := -lc

all: db test

force: clean all

debug:
	EFLAGS="-g" make force

$(BUILD_DIR)/%.o: $(SRC_DIRS)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

db: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

Gemfile:
	sudo apt-get install bundler
	bundle init
	echo "gem 'rspec'" >> Gemfile

test: Gemfile
	bundle exec rspec

clean:
	-@rm -r $(BUILD_DIR) *.db 2>/dev/null || true

.PHONY: all force clean debug test

-include $(DEPS)