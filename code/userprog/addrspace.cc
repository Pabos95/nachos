// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include <iostream>
using namespace std;
//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------
#define tamPagina 128
static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable, const char* filename)
{
    NoffHeader noffH;
    unsigned int j, size;
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);
#ifdef vm
strcopy(fn);
#endif
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
	/*std::cout<<"El tamaño del ejecutable es "<<size<<std::endl;*/
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
	numPages, size);
	// first, set up the translation
	pageTable = new TranslationEntry[numPages];
	for (j = 0; j < numPages; j++) {
		pageTable[j].virtualPage = j;	// for now, virtual page # = phys page #
#ifdef VM
      pageTable[j].valid = false;
      pageTable[j].physicalPage = -1;
#else
		pageTable[j].physicalPage = mapaGlobal.Find();
		pageTable[j].valid = true;
#endif
		pageTable[j].use = false;
		pageTable[j].dirty = false;
		pageTable[j].readOnly = false;  // if the code segment was entirely on
		// a separate page, we could set its
		// pages to be read-only
	}

	bzero(machine->mainMemory, size);
 datosInicializados = divRoundUp(noffH.code.size, PageSize);
 datosNoInicializados = datosInicializados + divRoundUp(noffH.initData.size, PageSize);
 pila = numPages - divRoundUp(UserStackSize,PageSize);
//#ifndef VM
	int direccionMemoriaCodigo = noffH.code.inFileAddr;
	int direccionMemoriaDatos = noffH.initData.inFileAddr;
	int numPaginasCodigo = divRoundUp(noffH.code.size, PageSize);
	int numPaginasDatos = divRoundUp(noffH.initData.size, PageSize);

	for (int i = 0; i < numPaginasCodigo; i++) //llena el segmento de codigo
	{
		executable->ReadAt(&(machine->mainMemory[ pageTable[i].physicalPage *128 ] ),
		PageSize, direccionMemoriaCodigo );
		direccionMemoriaCodigo+=128;
	}

	if (noffH.initData.size > 0) { //llena el segmento de datos inicializados
		for (int i = numPaginasCodigo; i < numPaginasDatos; i++)
		{
			executable->ReadAt(&(machine->mainMemory[ pageTable[i].physicalPage *128 ] ),
			PageSize, direccionMemoriaDatos);
			direccionMemoriaDatos+=128;
		}
	}
//#endif 
DEBUG('u',"Termina el constructor de Addrspace ");
}
//----------------------------------------------------------------------
// AddrSpace::AddrSpace(AddrSpace* padre)
// 	Construye
//----------------------------------------------------------------------
AddrSpace::AddrSpace(AddrSpace* padre){
     DEBUG('a', "entra al constructor de addrspace hijos \n");
   this->pageTable = new TranslationEntry[padre->numPages];
   DEBUG('a', "inicializa el page Table \n ");
    //inicializa codigo y variables
    unsigned int i;
    this->numPages = padre->numPages;
    unsigned int paginasPila = padre->numPages - (UserStackSize/PageSize); //asigna el numero de paginas de la pila
    for (i = 0; i < padre->numPages; i++) {
        DEBUG('a', "Entra al primer for \n ");
     if(i<paginasPila){
			this->pageTable[i].physicalPage = padre->pageTable[i].physicalPage; 
    	}else{
    	      this->pageTable[i].physicalPage = mapaGlobal.Find();
             pageTable[i].valid = true;
    	}
		this->pageTable[i].virtualPage = padre->pageTable[i].virtualPage;	 	
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;  
}
  DEBUG('a', "sale del constructor de addrspace hijos \n");  
}


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
#ifdef VM
DEBUG('t', "Se guarda el estado del hilo: %s\n", currentThread->getName());
for(int i = 0; i < TLBSize; ++i){
		pageTable[machine->tlb[i].virtualPage].use = machine->tlb[i].use;
		pageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
	}
	machine->tlb = new TranslationEntry[ TLBSize ];
	for (int i = 0; i < TLBSize; ++i)
	{
		machine->tlb[i].valid = false;
}
#endif 
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
#ifndef VM
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
#else
machine->tlb = new TranslationEntry[TLBSize];
for (int i = 0; i < TLBSize; ++i)
	{
		machine->tlb[i].valid = false;
}
#endif
}
bool AddrSpace::PaginaEnArchivo(int page){

}
int it = 0;
void AddrSpace::Load(unsigned int vpn){
DEBUG('a', "Numero de paginas: %d, Nombre del hilo actual: %s\n", numPages, currentThread->getName());
	DEBUG('a', "\tEl segmento de còdigo va de %d a %d \n",0, datosInicializados);
	DEBUG('a',"\t El segemento de datos incializados va de %d a  %d \n", datosInicializados, datosNoInicializados);
	DEBUG('a', "\t El segmento de datos no incializados va de %d a %d \n", datosNoInicializados , pila);
DEBUG('a',"\t El segmento de pila va de %d a  %d \n", pila, numPages );
for(int i = 0; i < 4; i++){
pageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
pageTable[machine->tlb[i].virtualPage].use = machine->tlb[i].use;
}
OpenFile *exec = fileSystem->Open(fn);
int numPaginasCodigo = divRoundUp(noff.code.size, PageSize);
int numPaginasDatos = divRoundUp(noff.initData.size, PageSize);
if(vpn < numPaginasCodigo){
/* Caso1
si la página a cargar es de código Y NO es Valida Ni Sucia
*/
if(pageTable[vpn].valid == false && (pageTable[vpn].dirty == false)){
OpenFile* Executable = fileSystem->Open(fn); 
}
/* Caso2
si la página a cargar es de código Y No es valida y es sucia
*/
if(pageTable[vpn].valid == false && (pageTable[vpn].dirty == false)){
}
}
//se guarda la página en la page table
machine->tlb[it].virtualPage = pageTable[vpn].virtualPage;
			machine->tlb[vpn].physicalPage = pageTable[it].physicalPage;
			machine->tlb[it].valid = pageTable[vpn].valid;
			machine->tlb[it].use = pageTable[vpn].use;
			machine->tlb[it].dirty = pageTable[vpn].dirty;
machine->tlb[it].readOnly = pageTable[vpn].readOnly;
}
