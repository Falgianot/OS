all:first
first: sorter_thread.c
	gcc -Wall -Werror -g -fsanitize=address sorter_thread.c -o sorter -lpthread
clean:
	rm -rf sorter
