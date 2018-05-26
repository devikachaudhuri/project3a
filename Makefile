lab3a: lab3a.c
	gcc -std=gnu11 -Wall -Wextra lab3a.c -o lab3a
dist:
	tar -czf lab3a-304597339.tar.gz lab3a.c Makefile README ext2_fs.h

clean:
ifneq ($(wildcard ./lab3a),)
	rm lab3a
endif
ifneq ($(wildcard ./lab3a-304597339.tar.gz),)
	rm lab3a-304597339.tar.gz
endif
