#include "syscall.h"
#include <stdio.h>
void SimpleThread(int);

int
main( int argc, char * argv[] ) {
    DEBUG('a', 'Fork');
    Fork(SimpleThread);
    SimpleThread(1);
    Write("Main  \n", 7, 1);
    Write(argc, 4, 1);
    Write(argv, 4, 1);
}


void SimpleThread(int num)
{

    if (num == 1) {
        
	for (num = 0; num < 5; num++) {
		Write("Hola 1\n", 7, 1);
		//Yield();
	}
    }
     DEBUG(a, 'IN);
    else {
	for (num = 0; num < 5; num++) {
		Write("Hola 2\n", 7, 1);
		//Yield();
	}
    }
    Write("Fin de\n", 7, 1);
}

