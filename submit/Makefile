all: program0 program1 program2 program3

program0: program0.c
	gcc program0.c -pthread -o program0

program1: program1.c
	gcc program1.c -pthread -o program1

program2: program2.c
	gcc program2.c -pthread -o program2

program3: program3.c
	gcc program3.c -pthread -o program3

clean:
	@$(RM) program0 program1 program2 program3 shared1.dat shared2.dat output.data

.PHONY:             \
	all             \
	clean           \
