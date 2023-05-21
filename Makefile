CC=gcc
CFLAGS=-Wall $(EFLAGS)

IDIR = include
_DEPS= utilities.h parser.h pager.h row.h table.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

INC = $(foreach d, $(IDIR), -I$d)

SDIR = src

ODIR = obj
_OBJ = db.o utilities.o parser.o pager.o row.o table.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

LIBS = -lc

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) $(INC) -c -o $@ $< $(CFLAGS)

all: db test

db: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

Gemfile:
	bundle init
	echo "gem 'rspec'" >> Gemfile

test: Gemfile
	bundle exec rspec

clean:
	rm $(ODIR)/*.o db