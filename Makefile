client.o: client/main.c helper.c defs.h job_queue.c helper.h
	gcc client/main.c helper.c job_queue.c -o client.o

naming_server.o: naming_server/main.c helper.c defs.h job_queue.c helper.h
	gcc -g naming_server/main.c helper.c job_queue.c naming_server/search.c -o naming_server.o

storage_server.o: storage_server/main.c helper.c defs.h job_queue.c helper.h
	gcc -g storage_server/main.c storage_server/helper.c helper.c job_queue.c -o storage_server.o

clean:
	rm client.o naming_server.o storage_server.o
