test_main : socket_server.c test_main.c
	gcc -DNODEBUG -g3 -Wall -o $@ $^ -lpthread

clean:
	rm test_main
