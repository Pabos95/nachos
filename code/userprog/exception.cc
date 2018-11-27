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
#include <string>
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
using namespace std;
struct SemJoin{ //struct que guarda un semaforo que sera usado para join, asi como la id y el nombre del archivo ejectubal
   Semaphore* sem; //semaforo del join
    string nombreArchivo;
   long threadId;
};
SemJoin** archivosEjecutables = new SemJoin*[128]; //aqui se guarda los nombre de los archivos ejecutables con su semaforo
BitMap* mapaEjecutables = new BitMap(128);  //mapa de bits para los archivos ejecutables
NachosSemTable* tablaSemaforos = new NachosSemTable(); //tabla de semaforos de nachos
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
  int valorSalida = machine->ReadRegister(4);
  printf("Exit value: %d\n", valorSalida  );
  Thread* nextThread;
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  DEBUG('t', "Terminando hilo \"%s\"\n", currentThread->getName());
  for(int ind = 0; ind < 128; ++ind){
    if(mapaEjecutables->Test(ind)){
      if((long)currentThread == archivosEjecutables[ind]->threadId){
        if(archivosEjecutables[ind]->sem != NULL){

          archivosEjecutables[ind]->sem->V();
        }
        else{
          delete archivosEjecutables[ind];
          mapaEjecutables->Clear(ind);
        }
      }
    }
  }

  machine->WriteRegister(2, machine->ReadRegister(4));

  nextThread = scheduler->FindNextToRun();
  if (nextThread != NULL) {
    scheduler->Run(nextThread);
  }else
  {
    currentThread->Finish();
  }
  interrupt->SetLevel(oldLevel);
}
void Nachos_Join(){ //System call 3
    long spaceId = machine->ReadRegister(4); //lee la id del proceso desde el registro 4
  if (mapaEjecutables->Test(spaceId)) //si se encuentra el ejecutable con ese spaceId
    {

        Semaphore* nuevoSemJoin = new Semaphore("semJoin", 0);
         archivosEjecutables[spaceId]->sem = nuevoSemJoin;
       nuevoSemJoin->P(); //hace esperar al proceso al que se le hace join
        machine->WriteRegister(2, 0); //avisa que el syscall se efectuo correctamente
        delete archivosEjecutables[spaceId];
        mapaEjecutables->Clear(spaceId);
       returnFromSystemCall();
    }
    else{ //No se encontro el ejecutable
        machine->WriteRegister(2, -1); //se  envia un mensaje de error al registro
printf("Ejecutable no encontrado");
    }
    returnFromSystemCall();
}

void Nachos_Close() {
  //Leemos dirección al archivo
    int closeF = machine -> ReadRegister(4);
    if(closeF != 0)
    {
      printf("Cierra archivo\n",closeF );
      delete (OpenFile *)closeF;
    }
    //Lo cerramos si existe.
    machine -> WriteRegister(PrevPCReg, machine -> ReadRegister(PCReg));
		machine -> WriteRegister(PCReg, machine -> ReadRegister(NextPCReg));
		machine -> WriteRegister(NextPCReg, machine -> ReadRegister(PCReg) + 4);


}

void Nachos_Open() {                    // System call 5
/*System call definition described to user
	int Open(
		char *name	// Register 4
	);
*/   DEBUG ('s', "Inicia Open\n");
printf("Entra a open");
    char name[128];
    int dir =  machine->ReadRegister(4); //lee el registro pues en ese es el que está la dirección del archivo a leer
    int caracterActual = 0;
    int i = 0;
    int idFalsa;
    do{
        machine->ReadMem(dir +i,1 ,&caracterActual); //lee un caracter de la memoria
        name[i++] = caracterActual;
        i++;
    }while(caracterActual != 0);
	// Read the name from the user memory, see 5 below
	// Use NachosOpenFilesTable class to create a relationship
	// between user file and unix file
	// Verify for errors
    int idArchivo = open( name, O_RDWR);//0_RDWR); //La id real  (en Unix)
    if(idArchivo != CODIGOERROR){
        idFalsa = currentThread->tablaArchivos->Open(idArchivo); //La id en Nachos
DEBUG ('s', "Archivo abierto s\n");
        machine->WriteRegister(2, idFalsa);
    }
    else{
        printf ("ERROR: Archivo no encontrado.\n");
		machine->WriteRegister (RSYSTEMCALL, CODIGOERROR);
    }
        returnFromSystemCall();		// Update the PC registers
DEBUG ('s', "Termina Open.\n");
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

void Nachos_Read(){
  //Leemos el id para ver si es una entreada de consola o de un archivo
      int id = machine->ReadRegister(6);
      if( id == ConsoleInput ){
        int buffer = machine->ReadRegister(4);
        int size  = machine->ReadRegister(5);
        char ch;
        //En caso de que sea de consola, leemos el puntero al inicio del búfer y su tamaño
        //Y luego leemos hasta que ya no haya
        for( int i = 0; i < size; i++){
            if( scanf("%c",&ch) != EOF )
                  if( machine->WriteMem(buffer + i , 1, (int)ch ) == true )
                        break;
        }

        machine->WriteRegister(2,0);
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(PCReg)+4);
        }
      else{
        //En caso de que sea un archivo igual leemos todas esas variables
        OpenFile* file = (OpenFile*)id;

        	int buffer = machine -> ReadRegister(4);
        	int size = machine -> ReadRegister(5);
          int result = 0;

          //Y leemos el a rchivo con la función SC_Read
        	if (size > 0 && size <= 128)
        	{
        		char temp[128];
        		result = file -> Read(temp, size);
        		for (int i = 0; i < size; ++ i)
        		{
        			machine -> WriteMem(buffer + i, 1, (int)(temp[i]));
        		}
        		printf("buffer:%s.\n", temp);
        	}

        	  machine -> WriteRegister(2, result);
        		machine -> WriteRegister(PrevPCReg, machine -> ReadRegister(PCReg));
        		machine -> WriteRegister(PCReg, machine -> ReadRegister(NextPCReg));
        		machine -> WriteRegister(NextPCReg, machine -> ReadRegister(PCReg) + 4);
        //return result;
      }
}

void Nachos_Create(){
DEBUG('s', "Entra a create");
      //Leemos el puntero al búfer y el tamaño
      int bufOffset = machine -> ReadRegister(4);
  	  char fileName[256] = {0};
       int c;
int i = 0;
      //Luego leemos el nombre del archivo a crear
do{
    machine->ReadMem( bufOffset , 1 , &c ); // read from nachos mem
    bufOffset++;
    fileName[i] = c;
    ++i;
  }while (c != 0 ); //si c es quiere decir que ya termino la lectura del nombre

      //Y usamos el SC de unix create para crearlo
  		printf("Creamos archivo == %s.\n", fileName);
  		int result = creat (fileName, O_CREAT|S_IRWXU );
  		DEBUG('s', "[]Creación de archivo: %s", fileName);

  if (result == -1 ) //si no sirve el creat
  {
    printf("No se pudo crear el archivo");

  }		
close(result);
returnFromSystemCall();
}
void NachosExecThread( void* id)
{
  SemJoin* info = archivosEjecutables[(long)id];

  OpenFile *executable = fileSystem->Open(info->nombreArchivo.c_str());
  AddrSpace *space;

  if (executable == NULL) {
    printf("Unable to open file %s\n",info->nombreArchivo);
    return;
  }
  space = new AddrSpace( executable );
  delete currentThread->space; // i dont need may space anymore
  currentThread->space = space;

  delete executable;			        // close file
  space->InitRegisters();		// set the initial register values
  space->RestoreState();		// load page table register
  machine->Run();			// jump to the user progam
  printf("\t\t\t\t\tError\n");
  ASSERT(false);			// machine->Run never returns;
}
void Nachos_Exec(){
  //2
DEBUG('j', "Enter Exec\n" );

//Leemos el nombre del archivo a ejecutar
DEBUG( 't', "Entering EXEC System call\n" );

  long registro4 = machine->ReadRegister( 4 ); // read from register 4
  char fileName[256] = {0}; // nombre del archivo a ejecutar
  int c, i; // contadores
  i = 0;
  do{
    machine->ReadMem( registro4 , 1 , &c ); // read from nachos mem
    registro4++;
    fileName[i++] = c;
}while (c != 0 );
string s = fileName;
DEBUG( 't', "Se ejecutara el archivo %s\n", fileName);
//OpenFile *executable = fileSystem->Open(fileName);
  SemJoin* nuevo = new SemJoin();
  // We need to create a new kernel thread to execute the user thread
  Thread * newT = new Thread( "HILO EXEC" );
  long fileToExec = mapaEjecutables->Find();
  if(fileToExec == -1){
    machine->WriteRegister(2, fileToExec );
printf("Unable to open file %s\n", fileName);    
return;
}
//Abrimos dicho archivo desde Nachso exec thread
nuevo->threadId = (long) newT;
  nuevo->nombreArchivo = s;
  archivosEjecutables[fileToExec] = nuevo;

  newT->Fork( NachosExecThread, (void*) fileToExec ); 
  machine->WriteRegister(2, fileToExec );
  returnFromSystemCall();	// Se actualizan los registros

DEBUG( 't', "Exiting EXEC System call\n" );
}


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
delete newT->tablaArchivos;
newT->tablaArchivos = currentThread->tablaArchivos; //se copia la tabla de archivos abiertos en el nuevo holo
newT->tablaArchivos->addThread(); //como hay un nuevo hilo usando la tabla se aumenta el usage
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
void* direccion =  (void*)(long)machine->ReadRegister( 4 );
newT->Fork( NachosForkThread, direccion);
    currentThread->Yield();
	returnFromSystemCall();	// This adjust the PrevPC, PC, and NextPC registers
	DEBUG( 'u', "Exiting Fork System call\n" );
}	// Kernel_Fork
void Nachos_Yield(){ //SystemCall 10
    currentThread->Yield();
    returnFromSystemCall();
}
void Nachos_SemCreate(){ //SystemCall 11
   int initVal = machine->ReadRegister(4);
   Semaphore* s = new Semaphore("Semaforo", initVal);
   long idSemaforo  = (long) s;
   tablaSemaforos->Create(idSemaforo);
   machine->WriteRegister(2, idSemaforo);
}

void Nachos_SemDestroy(){ //SystemCall 12
   int initVal = machine->ReadRegister(4);
   tablaSemaforos->Destroy(initVal);
}


void Nachos_SemSignal(){//SystemCall 13
    int numSemaforo = machine->ReadRegister(4);  //se lee la id del semaforo
    if(tablaSemaforos->getSemaphore(numSemaforo) != -1){
       machine->WriteRegister(2, 1); //se escribe 1 para  indicar que el semaforo existe
        Semaphore* s = (Semaphore*) tablaSemaforos->getSemaphore(numSemaforo);
    s->V();
    }
   else{
       machine->WriteRegister(2,-1);
   }
}

void Nachos_SemWait(){//SystemCall 14
    int numSemaforo = machine->ReadRegister(4);  //se lee la id del semaforo
    if(tablaSemaforos->getSemaphore(numSemaforo) != -1){
       machine->WriteRegister(2, 1); //se escribe 1 para  indicar que el semaforo existe
        Semaphore* s = (Semaphore*) tablaSemaforos->getSemaphore(numSemaforo);
    s->P();
    }
   else{
       machine->WriteRegister(2,-1);
   }
}

void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    unsigned int dirLogica; //se usa para almacenar la direccion logica de la pagina en la que ocurrio el page fault
    unsigned int vpn; // se usa para averiguar el numero de pagina virtual en caso de que haya una page fault exception
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
                  Nachos_Exec();
                  break;
             case SC_Join:
             // System call # 3
                  Nachos_Join();
                  break;
             case SC_Create:
             // System call # 4
                  break;
             case SC_Open:
                Nachos_Open();
             // System call # 5
                break;
             case SC_Read:
              //system Call #6
                  break;
             case SC_Write:
                Nachos_Write();
            // System call # 7
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
             case SC_SemWait:
             //System Call # 14
             break;
             default:
                printf("Unexpected syscall exception %d\n", type);
                ASSERT(false);
                break;
          }break;
               case PageFaultException:
                    printf("Excepcion de page fault \n");   // No valid translation found
                    dirLogica = machine->ReadRegister(39);
                    vpn = dirLogica/PageSize;
                    printf("Ocurre en la direccion: %d \n", dirLogica);
                    printf(" En la pagina : %d \n", vpn);
                  currentThread->space->Load(vpn);
                    break;
	       case  ReadOnlyException:     // Write attempted to page marked 
		     printf("Excepcion de read only");			    // "read-only"
                     ASSERT(false);
                     break;
		case  BusErrorException:     // Translation resulted in an 
		      printf("Execepcion de Bus Error");	
		    // invalid physical address
                       ASSERT(false);
                       break;
		case AddressErrorException: // Unaligned reference or one that
					    // was beyond the end of the
					    // address space
                     printf("Excepcion de Adress Error");
                     ASSERT(false);
                       break;
		case OverflowException:     // Integer overflow in add or sub.
                     printf("Excepcion de over flow");
                     ASSERT(false);
                     break;
		case IllegalInstrException:
                     printf("Excepcion de instruccion ilegal");
                     ASSERT(false);
                     break;
                default:
                       printf( "Unexpected exception %d\n", which );
                       ASSERT(false);
                       break;
    }
}
