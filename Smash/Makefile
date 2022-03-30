CC=gcc
CXX=g++
CFLAGS=-g -O -MMD -Werror -Wall -Wextra -std=c89 -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
CXXFLAGS= -g -Wall -Werror
LIBS=-lreadline -lncurses

PROGS=mydash

all: $(PROGS)

mydash: mydash.o log.o version.o list.o
	$(CXX) $(CXXFLAGS) -o $@  $^ $(LIBS)

##
## on every build, record the working copy revision string
##
version.c: FORCE
		echo '#include "version.h"\n' > version.c
		echo 'const char* version(void) { const char* GIT_Version = "'unk'"; return GIT_Version; }' >> version.c
	##	git rev-parse HEAD | cut -c1-6 | tr -d '\n'  >> version.c
##		echo '"; return GIT_Version; }' >> version.c


-include *.d

FORCE:
##
## Then any executable that links in git_version.o will be able
## to call the function git_version() to get a string that
## describes exactly what revision was built.

clean:
	/bin/rm -f *.o $(PROGS) a.out core  *.d version.c
