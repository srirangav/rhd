# Makefile for rhd

PGM_SRCS = rhd.c
PGM_OBJS = $(PGM_SRCS:.c=.o)
PGM  = rhd
PGM_REL  = 0.1.9
PGM_FILES = $(PGM_SRCS) $(PGM).1 Makefile LICENSE.txt README.txt

CC = gcc

# CFLAGS for gcc based on:
# https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc/

CFLAGS = -g -grecord-gcc-switches -O2 -pipe \
	 -Wall -Wshadow -Wpointer-arith -Wcast-qual \
	 -Wmissing-declarations -Wmissing-prototypes \
	 -Werror=format-security -Werror=implicit-function-declaration \
	 -W -DHD_REL='"$(PGM_REL)"' -D_FORTIFY_SOURCE=2 \
	 -D_GLIBCXX_ASSERTIONS -fasynchronous-unwind-tables \
	 -fexceptions -fpic -fstack-protector-all \
	 -fstack-protector-strong -fcf-protection -fwrapv

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(PGM)

$(PGM): $(PGM_OBJS)
	$(CC) $(CFLAGS) -o $(PGM) $(PGM_OBJS)

clean:
	/bin/rm -f *.o *~ core .DS_Store $(PGM) $(PGM).1.txt *.tgz

tgz: clean
	[ ! -d $(PGM)-$(PGM_REL) ] && mkdir $(PGM)-$(PGM_REL)
	cp $(PGM_FILES) $(PGM)-$(PGM_REL)
	tar -cvf $(PGM)-$(PGM_REL).tar $(PGM)-$(PGM_REL)
	gzip $(PGM)-$(PGM_REL).tar
	mv -f $(PGM)-$(PGM_REL).tar.gz $(PGM)-$(PGM_REL).tgz
	/bin/rm -rf $(PGM)-$(PGM_REL)

$(PGM).1.txt:
	nroff -man $(PGM).1 | col -b > $(PGM).1.txt

install:
	@echo "Please to the following:"
	@echo
	@echo "mkdir -p ~/bin ~/man/man1"
	@echo "cp $(PGM) ~/bin"
	@echo "cp $(PGM).1 ~/man/man1"
	@echo
	@echo "Add ~/bin to PATH and ~/man to MANPATH"

