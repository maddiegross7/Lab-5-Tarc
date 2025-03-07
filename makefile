# CC = gcc 
# INCLUDES = -I/home/mrjantz/cs360/include
# CFLAGS = $(INCLUDES)
# LIBDIR = /home/mrjantz/cs360/lib
# LIBS = $(LIBDIR)/libfdr.a 

# EXECUTABLES = bin/tarc

# all: $(EXECUTABLES)

# bin/tarc: src/tarc.c
# 	$(CC) $(CFLAGS) -o bin/tarc src/tarc.c $(LIBS)

# clean:
# 	rm -f $(EXECUTABLES)


CC = gcc 
INCLUDES = -I/home/mrjantz/cs360/include
CFLAGS = $(INCLUDES) -fsanitize=address -g
LIBDIR = /home/mrjantz/cs360/lib
LIBS = $(LIBDIR)/libfdr.a 

EXECUTABLES = bin/tarc

all: $(EXECUTABLES)

bin/tarc: src/tarc.c
	$(CC) $(CFLAGS) -o bin/tarc src/tarc.c $(LIBS)

clean:
	rm -f $(EXECUTABLES)
