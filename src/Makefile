log: main.o log.o utils.o log/log.h utils/utils.h
	gcc main.o log.o utils.o log/log.h utils/utils.h
utils.o: utils/utils.c utils/utils.h
	gcc -c utils/utils.c
log.o: log/log.c log/log.h utils/utils.h
	gcc -c log/log.c log/log.h utils/utils.h
main.o: main.c log/log.h
	gcc -c main.c log/log.h

