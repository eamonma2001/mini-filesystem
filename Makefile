CC=gcc
OPTIONS=-Wextra -Wall -O2 -g

# all c programs in current folder
ALL_C = $(wildcard *.c)
# exclude getParams (not used in current hw)
ALL_C := $(filter-out getParams.c, $(ALL_C))

# Uncomment for degug
#$(info VAR="$(ALL_C)")


# Macros: $@ = name of the file on the left of the colon (:)
#         $< = first item in the dependancy list
#					$^ = right side of the colon (:)
# Compile:

all:	vsfs

vsfs:	$(ALL_C)
	$(CC) $(OPTIONS) $(ALL_C) -o $@

.PHONY : all clean

all: $(TARGET)

clean:
	rm -rf *.o vsfs