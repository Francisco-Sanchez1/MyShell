CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDLIBS = -lreadline
yash: yash.c
	$(CC) $(CFLAGS) yash.c -o yash $(LDLIBS)