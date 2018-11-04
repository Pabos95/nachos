 
#include "syscall.h"

void pHijo(int);
int id;

int main(){
	Fork(pHijo);
	Write("Proceso Padre", 6, 1);
}


void nada(int dummy){
	Write( "Procesos hijo\n", 4, 1 );
}
