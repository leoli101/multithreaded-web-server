# Copyright ©2019 Aaron Johnston.  All rights reserved.  Permission is
# hereby granted to students registered for University of Washington
# CSE 333 for use solely during Summer Quarter 2019 for purposes of
# the course.  No other use, copying, distribution, or modification
# is permitted without prior written consent. Copyrights for
# third-party components of this work must be honored.  Instructors
# interested in reusing these course materials should contact the
# author.

# define the commands we will use for compilation and library building
AR = ar
ARFLAGS = rcs
CC = gcc
CXX = g++

# define useful flags to cc/ld/etc.
CFLAGS += -g -Wall -Wpedantic -I. -I.. -O0
LDFLAGS += -L. -lhw1
CPPUNITFLAGS = -L../gtest -lgtest

# define common dependencies
OBJS = LinkedList.o HashTable.o Verify333.o
HEADERS = LinkedList.h HashTable.h CSE333.h
TESTOBJS = test_linkedlist.o test_hashtable.o test_suite.o
TESTHEADERS = test_linkedlist.h test_hashtable.h

# compile everything; this is the default rule that fires if a user
# just types "make" in the same directory as this Makefile
all: test_suite example_program_ll example_program_ht FORCE

example_program_ll: example_program_ll.o libhw1.a $(HEADERS) FORCE
	$(CC) $(CFLAGS) -o example_program_ll example_program_ll.o $(LDFLAGS)

example_program_ht: example_program_ht.o libhw1.a $(HEADERS) FORCE
	$(CC) $(CFLAGS) -o example_program_ht example_program_ht.o $(LDFLAGS)

libhw1.a: $(OBJS) $(HEADERS) FORCE
	$(AR) $(ARFLAGS) libhw1.a $(OBJS)

test_suite: $(TESTOBJS) $(TESTHEADERS) libhw1.a FORCE
	$(CXX) $(CFLAGS) -o test_suite $(TESTOBJS) \
	$(CPPUNITFLAGS) $(LDFLAGS) -lpthread $(LDFLAGS)

%.o: %.cc $(HEADERS) FORCE
	$(CXX) $(CFLAGS) -std=c++11 -c $<

%.o: %.c $(HEADERS) FORCE
	$(CC) $(CFLAGS) -c -std=c11 $<

clean: FORCE
	/bin/rm -f *.o *~ *.gcno *.gcda *.gcov test_suite libhw1.a \
    example_program_ll example_program_ht

FORCE:
