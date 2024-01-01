test_main : socket_server.c test_main.c
	gcc -DNDEBUG -O2 -Wall -o $@ $^ -lpthread

debug : socket_server.c test_main.c
	gcc -g3 -O2 -Wall -o $@ $^ -lpthread

clean:
	rm test_main 
	rm debug
