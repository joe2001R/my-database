CC=gcc
CFLAGS=-Wall $(EFLAGS)
DEPS= utilities.h parser.h pager.h row.h table.h
OBJ = db.o utilities.o parser.o pager.o row.o table.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: db test

db: $(OBJ)
	$(CC) -o $@ $^ -lc $(CFLAGS)

Gemfile:
	bundle init
	echo "gem 'rspec'" >> Gemfile

test: Gemfile
	bundle exec rspec