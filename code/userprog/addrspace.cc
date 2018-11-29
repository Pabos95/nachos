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
fn = (char*) filename;
DEBUG('a', "Archivo : %s\n", filename);
// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
	/*std::cout<<"El tamaño del ejecutable es "<<size<<std::endl;*/
DEBUG('a', "Tamaño del segmento de codigo %d\n", noffH.code.size);
DEBUG('a', "Tamaño del segmento de datos inicializados %d\n", noffH.initData.size);
DEBUG('a', "Tamaño del segmento de datos no inicializados %d\n", noffH.uninitData.size);
DEBUG('a', "Tamaño del segmento de datos inicializados %d\n", UserStackSize);
DEBUG('a', "Tamaño del se la pagina  %d\n", PageSize);
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
#ifndef VM
    ASSERT(numPages <= NumPhysPages);		// check we're not trying
#endif						// to run anything too big --
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
 datosInicializados = divRoundUp(noffH.code.size, PageSize);
 datosNoInicializados = datosInicializados + divRoundUp(noffH.initData.size, PageSize);
 pila = numPages - divRoundUp(UserStackSize,PageSize);
DEBUG('a', "Creado el page table");
#ifndef VM
	bzero(machine->mainMemory, size);
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
#endif 
DEBUG('a',"Termina el constructor de Addrspace ");
}
//----------------------------------------------------------------------
// AddrSpace::AddrSpace(AddrSpace* padre)
// 	Construye
//----------------------------------------------------------------------
AddrSpace::AddrSpace(AddrSpace* padre){
  numPages = padre->numPages;
	pageTable = new TranslationEntry[padre->numPages];
	for(int i = 0; i < padre->numPages-8; i++){
		pageTable[i].virtualPage = padre->pageTable[i].virtualPage;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = padre->pageTable[i].physicalPage;
		pageTable[i].valid = padre->pageTable[i].valid;
		pageTable[i].use = padre->pageTable[i].use;
		pageTable[i].dirty = padre->pageTable[i].dirty;
		pageTable[i].readOnly = padre->pageTable[i].readOnly;
	}

	for(int i = padre->numPages-8; i < padre->numPages; i++){
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
#ifndef vm
		pageTable[i].physicalPage = mapaGlobal.Find();
#else
     pageTable[i].physicalPage = padre->pageTable[i].physicalPage;
#endif
		pageTable[i].valid = true;
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;

}
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
int AddrSpace::secondChanceSwap(){
if ( indiceSWAPSecondChance < 0 || indiceSWAPSecondChance >= NumPhysPages )
	{
		DEBUG('v', "Indice swap %d \n invalido", indiceSWAPSecondChance );
		ASSERT( false );
	}
	int espacioLibre = -1;
	bool encontrado = false;

	while ( encontrado == false )
	{
		if ( pageTableInvertida[ indiceSWAPSecondChance ] == NULL )
		{
		DEBUG('v', "\Error estado de la pageTableInvertida invalido \n");
		ASSERT( false );
		}

		if ( pageTableInvertida[ indiceSWAPSecondChance ]->valid == false )
		{
			DEBUG('v', "\nEntrada invalida de la page table invertida (use es falso)");
			ASSERT( false );
		}

		if ( pageTableInvertida[ indiceSWAPSecondChance ]->use == true )
		{
				pageTableInvertida[ indiceSWAPSecondChance ]->use = false;
		}else
		{
				espacioLibre = indiceSWAPSecondChance;
				encontrado = true;
		}
		indiceSWAPSecondChance = (indiceSWAPSecondChance+1) % NumPhysPages;
	}

	if (espacioLibre < 0 || espacioLibre >= NumPhysPages )
	{
		DEBUG('v',"Informacion de page table invertida invalida \n");
		ASSERT( false );
	}
	return espacioLibre;
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
//se devuelven los indices de second chance
indiceSWAPSecondChance = 0;
indiceTLBSecondChance = 0;
//se hace una nueva TLB
machine->tlb = new TranslationEntry[TLBSize];
for (int i = 0; i < TLBSize; ++i)
	{
		machine->tlb[i].valid = false;
}
#endif
}
void AddrSpace::usarIndiceTLB( int indiceTLB, int vpn )
{
	if ( indiceTLB < 0 || indiceTLB >= TLBSize  )
	{
		DEBUG('v',"\n Error indice de TLB invalido ");
		ASSERT(false);
	}
	if ( vpn < 0 || (unsigned int) vpn >= numPages  )
	{
		DEBUG('v',"\n Error :vpn invalido %d\n",vpn);
		ASSERT(false);
	}
	machine->tlb[indiceTLB].virtualPage =  pageTable[vpn].virtualPage;
	machine->tlb[indiceTLB].physicalPage = pageTable[vpn].physicalPage;
	machine->tlb[indiceTLB].valid = pageTable[vpn].valid;
	machine->tlb[indiceTLB].use = pageTable[vpn].use;
	machine->tlb[indiceTLB].dirty = pageTable[vpn].dirty;
	machine->tlb[indiceTLB].readOnly = pageTable[vpn].readOnly;
}
void AddrSpace::escribirEnSWAP(int paginaFisicaVictima){
int paginaSwap = mapaSWAP->Find(); //busca una pagina libre en el swap
if ( paginaSwap == -1 ){
DEBUG( 'v', "Error no hay espacio en el swap\n");
ASSERT( false );
}
if ( paginaFisicaVictima < 0 || paginaFisicaVictima >= NumPhysPages ){
DEBUG( 'a', "Error al escribir en swap : numero de pagina  %d\ no valido", paginaFisicaVictima );
ASSERT( false );
	}
DEBUG('a', "Se escribira en la pagina del swap: %d\n",paginaSwap);
OpenFile *swap = fileSystem->Open("Swap.txt"); //se abre el archivo SWAP
	if( swap == NULL ){
		DEBUG( 'a', "Error no se pudo abrir el archivo de swap\n");
		ASSERT(false);
	}
mapaGlobal.Clear(indiceSWAPFIFO); //se libera un espacio en el mapa de memoria
pageTableInvertida[paginaFisicaVictima]->valid = false;
pageTableInvertida[paginaFisicaVictima]->physicalPage = paginaSwap;
swap->WriteAt((&machine->mainMemory[paginaFisicaVictima*PageSize]),PageSize, paginaSwap*PageSize);
//una vez que se ha escrito la pagina en el swap se elimina el OpenFile
delete swap;
}
int  AddrSpace::secondChanceTLB()
{
	int libre = -1;
	for ( int i = 0; i < TLBSize; i++ )
	{
//si se encuentra una entrada del tlb invalida entonces lo asigna ahi
		if ( machine->tlb[i].valid == false )
		{
			return i;
		}
	}
	bool espacioEncontrado = false;
	while ( espacioEncontrado == false )
	{
		if ( machine->tlb[ indiceTLBSecondChance ].use == true )
		{
			machine->tlb[ indiceTLBSecondChance ].use = false;
			salvarVictimaTLB( indiceTLBSecondChance, true );
		}else
		{
			espacioEncontrado = true;
			libre = indiceTLBSecondChance;
			salvarVictimaTLB( libre, false );
		}
		indiceTLBSecondChance = (indiceTLBSecondChance+1) % TLBSize;
	}
	if ( libre < 0 || libre >= TLBSize )
	{
		DEBUG('v',"\ Pagina libre invalida\n");
		ASSERT( false );
	}
	return libre;
}
void AddrSpace::actualizarVictimaSwap(int indiceSWAP)
{
	for (int i = 0; i < TLBSize; i++)
	{
		if ( machine->tlb[i].valid && (machine->tlb[i].physicalPage == pageTableInvertida[indiceSWAP]->physicalPage)  )
		{
			DEBUG('v',"%s\n", "\t\t\tSí estaba la victima en TLB" );
			machine->tlb[i].valid = false;
			pageTableInvertida[indiceSWAP]->use = machine->tlb[i].use;
			pageTableInvertida[indiceSWAP]->dirty = machine->tlb[i].dirty;
			break;
		}
	}
}
bool AddrSpace::PaginaEnArchivo(int page){

}
int it = 0;
void AddrSpace::Load(unsigned int vpn){
DEBUG('a', "Archivo de origen : %s\n", fn);
DEBUG('a', "Numero de paginas: %d, Nombre del hilo actual: %s\n", numPages, currentThread->getName());
	DEBUG('a', "\tEl segmento de còdigo va de %d a %d \n",0, datosInicializados);
	DEBUG('a',"\t El segemento de datos incializados va de %d a  %d \n", datosInicializados, datosNoInicializados);
	DEBUG('a', "\t El segmento de datos no incializados va de %d a %d \n", datosNoInicializados , pila);
DEBUG('a',"\t El segmento de pila va de %d a  %d \n", pila, numPages );
for(int i = 0; i < 4; i++){
pageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
pageTable[machine->tlb[i].virtualPage].use = machine->tlb[i].use;
}
printf("Archivo : %s\n", fn);
printf("Archivo : %s\n", fn);
OpenFile *exec = fileSystem->Open(fn);
if (exec == NULL) {
			DEBUG('a',"No se pudo abrir el archivo%s\n", fn);
			ASSERT(false);
		}
int numPaginasCodigo = divRoundUp(noff.code.size, PageSize);
int numPaginasDatos = divRoundUp(noff.initData.size, PageSize);
int libre; //aqui se guarda una direccion de memoria que este  libre
//si la pagina a cargar pertenece al segmento de codigo
if((vpn >= 0) && (vpn < datosInicializados)){
DEBUG('a', "\tLa pagina pertenece al semgneto de codigo");
/* Caso1
si la página a cargar es de código Y NO es Valida Ni Sucia
*/
NoffHeader noffH;
exec->ReadAt((char *)&noffH, sizeof(noffH), 0);
DEBUG('a', "Leido el ejecutable");
if(pageTable[vpn].valid == false && (pageTable[vpn].dirty == false)){
//busca espacio libre en la memoria para esta pagina
DEBUG('a', "\tArchivo origen del page fault: %s\n", fn);
libre = mapaGlobal.Find();
//actualiza las estadisticas sobre el numero de page faults
++stats->numPageFaults;
if (libre != -1  ){ //si se encontro espacio en memoria
				DEBUG('a',"\tSe ha econtrado espacio libre en: %d\n", libre );
				pageTable[vpn].physicalPage = libre; //se asigna una pagina fisica
				exec->ReadAt(&(machine->mainMemory[(libre * PageSize)]),
				PageSize, noff.code.inFileAddr + PageSize*vpn );
				pageTable[vpn].valid = true; //se marca como válida

				//Se debe actualizar la TLB invertida
                                pageTableInvertida[libre] = &(pageTable[ vpn ]);
				//Luego se debe actualizar el TLB
                                int espacioTLB = secondChanceTLB();
                                usarIndiceTLB( espacioTLB, vpn );
}
//si no se encontro espacio entonces busca una victima swap
else{
indiceSWAPFIFO= secondChanceTLB();
actualizarVictimaSwap(indiceSWAPFIFO);
if(pageTableInvertida[indiceSWAPFIFO]->dirty){
//como al escribir en SWAP se libero un espacio en la memoria entonces vuelve buscarlo en el mapa de memoria
libre = mapaGlobal.Find();
//si no se encontro espacio en el mapa quiere decir que la memoria esta fallando
if (libre == -1){
printf("Error al cargar la pagina a memoria %d\n", libre );
ASSERT( false );
}
//se marca la pagina fisica libre recien encontrada en la pageTable
pageTable[vpn].physicalPage = libre;
//se continua la lectura del programa cargando a la memoria la pagina siguiente
exec->ReadAt(&(machine->mainMemory[(libre* PageSize )]),PageSize, noffH.code.inFileAddr + PageSize*vpn );
//se marca la pagina como valida
pageTable[vpn].valid = true;
//se actualiza la page table invertida
pageTableInvertida[libre] =  &(pageTable[vpn]);
//se busca el siguiente espacio a usar en la TLB con second chance
int siguiente = secondChanceTLB();
usarIndiceTLB(siguiente, vpn);
}
else{ //si la victima es limpia
int pagFisicaAntigua = pageTableInvertida[indiceSWAPFIFO]->physicalPage;
//se elimina la pagina fisica y se marca esa entrada como invalida
pageTableInvertida[indiceSWAPFIFO]->physicalPage = -1;
pageTableInvertida[indiceSWAPFIFO]->valid = false;
//se libera el espacio en el mapa de memoria
mapaGlobal.Clear(pagFisicaAntigua);
//se busca un nuevo espacio libre en memoria
libre = mapaGlobal.Find();
//si no se encontro un espacio libre
if (libre == -1 ){
printf("Frame %d\n no valido", libre );
ASSERT( false );
}
pageTable[vpn].physicalPage = libre;
exec->ReadAt(&(machine->mainMemory[(libre* PageSize )]),PageSize, noffH.code.inFileAddr + PageSize*vpn );
pageTable[vpn].valid = true;
pageTableInvertida[libre] = &(pageTable [ vpn ]);
int tlbSPace = secondChanceTLB();
usarIndiceTLB(tlbSPace,vpn);
}
}
/* Caso2
si la página a cargar es de código Y No es valida y es sucia
*/
if(pageTable[vpn].valid == false && (pageTable[vpn].dirty == true)){
}
}
//se guarda la página en la page table
}
machine->tlb[it].virtualPage = pageTable[vpn].virtualPage;
			machine->tlb[vpn].physicalPage = pageTable[it].physicalPage;
			machine->tlb[it].valid = pageTable[vpn].valid;
			machine->tlb[it].use = pageTable[vpn].use;
			machine->tlb[it].dirty = pageTable[vpn].dirty;
machine->tlb[it].readOnly = pageTable[vpn].readOnly;
}
void AddrSpace::salvarVictimaTLB(int indiceTLB, bool uso){
if ( indiceTLB < 0 || indiceTLB >= TLBSize  )
	{
		DEBUG('v',"\n Error indice %d\n invalido", indiceTLB);
		ASSERT(false);
	}
pageTable[machine->tlb[indiceTLB].virtualPage].use = (uso == 1?uso:machine->tlb[indiceTLB].use);
pageTable[machine->tlb[indiceTLB].virtualPage].dirty = machine->tlb[indiceTLB].dirty;
}
//Busca un lugar en la TLB por medio del algoritmo second chance
int AddrSpace::BuscarTLBSecondChance(){
int espacioLibre = -1;
for (int i = 0; i < TLBSize; i++){
if(machine->tlb[i].valid == false){ //si no es valido se detiene aqui
return i;
}
}
// si itero toda la TLB y no encontro ninguna pagina no valida busca cual reemplezar
bool encontrado = false;
while(encontrado == false){
if ( machine->tlb[ indiceTLBSecondChance ].use == true )
		{
			machine->tlb[ indiceTLBSecondChance ].use = false;
			salvarVictimaTLB( indiceTLBSecondChance, true );
		}else
		{
			encontrado = true;
			espacioLibre = indiceTLBSecondChance;
			salvarVictimaTLB( espacioLibre, false );
		}
		indiceTLBSecondChance = (indiceTLBSecondChance+1) % TLBSize;
}
}
