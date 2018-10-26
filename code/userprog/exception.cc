// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "synch.h"
#include "stdio.h"
#define CODIGOERROR -1
#define RSYSTEMCALL 2 
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
Semaphore*  Console = new Semaphore("Sem", 1); //inicializa un semaforo
        void returnFromSystemCall() {

                int pc, npc;

                pc = machine->ReadRegister( PCReg );
                npc = machine->ReadRegister( NextPCReg );
                machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
                machine->WriteRegister( PCReg, npc );           // PC <- NextPC
                machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4

        }       // returnFromSystemCall


void Nachos_Halt() {                    // System call 0

        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();

}       // Nachos_Halt
void Nachos_Exit() { //System call 1
    int status = machine->ReadRegister(4);
    currentThread->tablaArchivos->delThread();
    currentThread->Finish();
}
void Nachos_Join(){ //System call 3
    int spaceId = machine->ReadRegister(4); //lee la id del proceso desde el registro 4
   int t =  0; //currentThread->getChild(spaceId);
  if (t == NULL) //si no encuentra al hijo con id SpaceId
    {
	// If NULL then spaceId was not found
        // among currentThread's children.
        machine->WriteRegister(2, CODIGOERROR); //como no existe el retorna un error al registro 2
       returnFromSystemCall();
    }
    Console->P();
    Console->V();
    returnFromSystemCall();
}
void Nachos_Open() {                    // System call 5
/*System call definition described to user
	int Open(
		char *name	// Register 4
	);
*/   DEBUG ('a', "Inicia Open\n");
printf("Entra a open");
    char name[128];
    int dir =  machine->ReadRegister(4); //lee el registro pues en ese es el que está la dirección del archivo a leer
    int caracterActual = 1;
    int i = 0;
    int idFalsa;
    while(caracterActual != 0){
        machine->ReadMem(dir +i,1 ,&caracterActual); //lee un caracter de la memoria
        name[i++] = caracterActual;
        i++;
    }
	// Read the name from the user memory, see 5 below
	// Use NachosOpenFilesTable class to create a relationship
	// between user file and unix file
	// Verify for errors
    int idArchivo = open( name, O_RDWR);//0_RDWR); //La id real  (en Unix)
    if(idArchivo != CODIGOERROR){
        idFalsa = currentThread->tablaArchivos->Open(idArchivo); //La id en Nachos
        machine->WriteRegister(2, idFalsa);
    }
    else{
        printf ("ERROR: Archivo no encontrado.\n");
		machine->WriteRegister (RSYSTEMCALL, CODIGOERROR);
    }
        returnFromSystemCall();		// Update the PC registers
DEBUG ('a', "Termina Open.\n");
}       // Nachos_Open

void Nachos_Write() {                   // System call 7
        DEBUG('a',"Entra a write \n");
        int adress = machine->ReadRegister(4); //direccion  del buffer 
          DEBUG('a',"Lee el registro 4 \n");
        size_t size = machine->ReadRegister( 5 );	// Read size to write
         char buffer[size+1];
        DEBUG('a',"Lee el registro 5 \n");
        // buffer = Read data from address given by user;
        OpenFileId id = machine->ReadRegister( 6 );	// Read file descriptor
        if(id < 0){
        DEBUG('a', "Error al leer el descriptor");
        }
 DEBUG('a',"Lee el registro 6 \n");
 DEBUG('a',"Id : ");
 int resultado;
	// Need a semaphore to synchronize access to console
    Console->P();
    buffer[ size ] = '\0'; //la ultima entrada se marca como 0 para que se indique a donde acaba la lectura de memoria
    int valorActual = machine->ReadMem(adress,1,&valorActual); //lee el primer caracter a escribir
    buffer[0] = valorActual;
    int pos = 1;
    while(valorActual != '\0'){
        machine->ReadMem(adress,1,&valorActual);
        buffer[pos] = valorActual;
        adress++; //se aumenta la direccion de memoria para leer el siguiente valor
        pos++; //se aumenta la posicion del buffer
    }
	switch (id) {
         //DEBUG('a',"Entra al switch");
        printf("Entra al switch \n");
		case  ConsoleInput:	// User could not write to standard input
			machine->WriteRegister( 2, -1 );
			break;
		case  ConsoleOutput: //en caso de que lo que se solicite sea imprimir en la terminal
			printf( "%s", buffer ); 
		break;
		case ConsoleError:	// This trick permits to write integers to console
			printf( "%d\n", machine->ReadRegister( 4 ) );
			break;
		default:
		    if(currentThread->tablaArchivos->isOpened(id) == CODIGOERROR){
				machine->WriteRegister(2, CODIGOERROR);
			}
			else{
				resultado = write(currentThread->tablaArchivos->getUnixHandle(id),buffer,size); //consegue la direccion real (de Unix) del archivo y manda a escribir el buffer
				machine->WriteRegister(2, resultado); //retorna el numero de bytes escritos
			}
			// All other opened files
			// Verify if the file is opened, if not return -1 in r2
			// Get the unix handle from our table for open files
			// Do the write to the already opened Unix file
			// Return the number of chars written to user, via r2
			break;

	}
	// Update simulation stats, see details in Statistics class in machine/stats.cc
	Console->V();
 DEBUG('a',"Saliendo del write");
        returnFromSystemCall();		// Update the PC registers

}// Nachos_Write
void NachosForkThread( void * p ) { // for 64 bits version

    AddrSpace *space;

    space = currentThread->space;
    space->InitRegisters();             // set the initial register values
    space->RestoreState();              // load page table register

// Set the return address for this thread to the same as the main thread
// This will lead this thread to call the exit system call and finish
    machine->WriteRegister( RetAddrReg, 4 );

    machine->WriteRegister( PCReg, (long) p );
    machine->WriteRegister( NextPCReg, (long) p + 4 );

    machine->Run();                     // jump to the user progam
    ASSERT(false);

}
void Nachos_Fork() {			// System call 9

	DEBUG( 'u', "Entering Fork System call\n" );
	// We need to create a new kernel thread to execute the user thread
Thread * newT = new Thread( "child to execute Fork code" );

	// We need to share the Open File Table structure with this new child
currentThread->tablaArchivos->addThread();
	// Child and father will also share the same address space, except for the stack
	// Text, init data and uninit data are shared, a new stack area must be created
	// for the new child
	// We suggest the use of a new constructor in AddrSpace class,
	// This new constructor will copy the shared segments (space variable) from currentThread, passed
	// as a parameter, and create a new stack for the new child
newT->space = new AddrSpace( currentThread->space );
	// We (kernel)-Fork to a new method to execute the child code
	// Pass the user routine address, now in register 4, as a parameter
	// Note: in 64 bits register 4 need to be casted to (void *)
void* direccion =  (void*)machine->ReadRegister( 4 );
newT->Fork( NachosForkThread, direccion);
	returnFromSystemCall();	// This adjust the PrevPC, PC, and NextPC registers
	DEBUG( 'u', "Exiting Fork System call\n" );
}	// Kernel_Fork
void Nachos_Yield(){ //SystemCall 10
    currentThread->Yield();
    returnFromSystemCall();
}
void Nachos_SemCreate(){ //SystemCall 11
   int initVal = machine->ReadRegister(4);
   Semaphore *s;
   s = new Semaphore("Sem", initVal);
}
void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
/*if((type >= 0) && (type <= 13)){
	which = SyscallException;
	}*/
    DEBUG ('a', "Tipo de syscall %d y ExceptionType %d\n", type, which);
    switch ( which ) {
       case SyscallException:
          switch (type) {		  
             case SC_Halt:
                Nachos_Halt();             // System call # 0
                break;
             case SC_Exit:   
             // System call # 1
                 Nachos_Exit();
                 break;
             case SC_Exec:  
             // System call # 2
                  break;
             case SC_Join: 
             // System call # 3
                  break;
             case SC_Create: 
             // System call # 4
                  break;        
             case SC_Open: 
                Nachos_Open();             // System call # 5
                break;
             case SC_Read:
              //system Call #6
                  break;
             case SC_Write:
                Nachos_Write();             // System call # 7
                break;
             case SC_Close:
             //System Call # 8
                  break;
             case SC_Fork:
                 Nachos_Fork();
             //System Call # 9
             break;
             case SC_Yield:
                 Nachos_Yield();
             //System Call #10
             break;
             case SC_SemCreate:
                 Nachos_SemCreate();
             //System Call #11
             break;
             case SC_SemDestroy:
             //System Call #12
             break;
             case SC_SemSignal:
             //System Call # 13
             break;
             default:
                printf("Unexpected syscall exception %d\n", type);
                ASSERT(false);
                break;
          }break;
       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT(false);
          break;
    }
}
