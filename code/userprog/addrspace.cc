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


//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

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

AddrSpace::AddrSpace( AddrSpace* padre)
{
	initData = padre->initData;
	noInitData = padre->noInitData;
	stack = padre->stack;

	numPages = padre->numPages;
	pageTable = new TranslationEntry[ numPages ];
	fn = padre->fn;
	// iterar menos las 8 paginas de pila
	long dataAndCodePages = numPages - 8;
	long i;
	for (i = 0; i < dataAndCodePages; ++ i )
	{
		pageTable[i].virtualPage =  i;
		pageTable[i].physicalPage = padre->pageTable[i].physicalPage;
		pageTable[i].valid = padre->pageTable[i].valid;
		pageTable[i].use = padre->pageTable[i].use;
		pageTable[i].dirty = padre->pageTable[i].dirty;
		pageTable[i].readOnly = padre->pageTable[i].readOnly;
	}
	// 8 paginas para la pila

	for (i = dataAndCodePages; i < numPages ; ++ i )
	{
		pageTable[i].virtualPage =  i;	// for now, virtual page # = phys page #
		#ifndef VM
		pageTable[i].physicalPage = mapaGlobal->Find();
		pageTable[i].valid = true;
		#else
		pageTable[i].physicalPage = -1;
		pageTable[i].valid =false;
		#endif
		pageTable[i].use = false;
		pageTable[i].dirty = false;
		pageTable[i].readOnly = false;
	}
}

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
		pageTable[j].physicalPage = mapaGlobal->Find();
		pageTable[j].valid = true;
#endif
		pageTable[j].use = false;
		pageTable[j].dirty = false;
		pageTable[j].readOnly = false;  // if the code segment was entirely on
		// a separate page, we could set its
		// pages to be read-only

    DEBUG('a', "Iteracion page Table %d\n",
	j);
	}
imprimirPageTable();
initData = divRoundUp(noffH.code.size, PageSize);
	noInitData = initData + divRoundUp(noffH.initData.size, PageSize);
	stack = numPages - divRoundUp(UserStackSize,PageSize);
DEBUG('a', "Creado el page table");
imprimirPageTable();
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
//	to this address space, that needs saving//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState()
{
	#ifdef VM
	DEBUG ( 't', "\nSe guarda el estado del hilo %s\n", currentThread->getName() );
	for(int i = 0; i < TLBSize; ++i){
		pageTable[machine->tlb[i].virtualPage].use = machine->tlb[i].use;
		pageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
	}
	/*
	if (machine->tlb != NULL)
	{
		delete [] machine->tlb;
		machine->tlb = NULL;
	}
	*/
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
	DEBUG ( 't', "\nSe restaura el estado del hilo: %s\n", currentThread->getName() );
	#ifndef VM
	machine->pageTable = pageTable;
	machine->pageTableSize = numPages;
	#else
//se reinician los indices
	indiceTLBFIFO = 0;
	indiceTLBSecondChance = 0;
	for (int i = 0; i < TLBSize; ++i)
	{
		machine->tlb[i].valid = false;
	}
	#endif
}

void AddrSpace::limpiarPagFisica( int pag)
{
	if ( pag < 0 || pag >= NumPhysPages )
	{
			printf("Error : Numero de pagina menor a 0 o mayor al numero de paginas fisicas : %d\n", pag);
			ASSERT( false );
	}
	for (int i = 0; i < PageSize; ++i )
	{
		machine->mainMemory[ pag*PageSize + i ] = 0;
	}
}

void AddrSpace::imprimirPageTableInvertida()
{
	DEBUG('v',"\n");
	for (unsigned int x = 0; x < NumPhysPages; ++x)
	{
		if( pageTableInvertida[x] != NULL)
		DEBUG('v',"PhysicalPage = %d, VirtualPage = %d, Valid = %d, Dirty = %d, Use = %d \n", pageTableInvertida[x]->physicalPage, pageTableInvertida[x]->virtualPage, pageTableInvertida[x]->valid, pageTableInvertida[x]->dirty, pageTableInvertida[x]->use );
	}
}

void AddrSpace::imprimirPageTable()
{
	for (unsigned int x = 0; x < numPages; ++x)
	{
		DEBUG('v',"i [%d] .virtualPage = %d, .physicalPage = %d, .use = %d, .dirty = %d, valid = %d\n"
		,x,pageTable[x].virtualPage, pageTable[x].physicalPage, pageTable[x].use, pageTable[x].dirty, pageTable[x].valid );
	}
}

void AddrSpace::imprimirTLB()
{
	printf("TLB status\t\t\t\t secondChanceTLBce i = %d\n", indiceTLBSecondChance);
	for (int i = 0; i < TLBSize; ++i )
	{
		DEBUG('a',"TLB[%d].paginaFisica = %d, paginaVirtual = %d, usada = %d, sucia = %d, valida = %d\n",
		i, machine->tlb[ i ].physicalPage,
		machine->tlb[ i ].virtualPage, machine->tlb[ i ].use, machine->tlb[ i ].dirty, machine->tlb[ i ].valid );
	}
	printf("\n");
}

void AddrSpace::escribirEnSwap( int physicalPageVictim ){
	int swapPage = mapaSWAP->Find();
	if ( physicalPageVictim < 0 || physicalPageVictim >= NumPhysPages )
	{
			DEBUG( 'v', "Error(escribirEnSwap): Direccion fisica de memoria invalida: %d\n", physicalPageVictim );
			ASSERT( false );
	}
	DEBUG('h', "\t\t\t\tSe escribe en el swap en la posición: %d\n",swapPage );
	if ( swapPage == -1 )
	{
		DEBUG( 'v', "Error(escribirEnSwap): Espacio en SWAP NO disponible\n");
		ASSERT( false );
	}
	OpenFile *swapFile = fileSystem->Open( SWAPFILENAME );
	if( swapFile == NULL ){
		DEBUG( 'v', "Error(escribirEnSwap): No se pudo habir el archivo de SWAP\n");
		ASSERT(false);
	}
	pageTableInvertida[physicalPageVictim]->valid = false;
	pageTableInvertida[physicalPageVictim]->physicalPage = swapPage;
	swapFile->WriteAt((&machine->mainMemory[physicalPageVictim*PageSize]),PageSize, swapPage*PageSize);
	mapaGlobal->Clear( indiceSWAPFIFO );
	delete swapFile;
}
void AddrSpace::leerSwap( int physicalPage , int swapPage ){
	DEBUG('h', "\t\t\t\tSe lee en el swap en la posición: %d\n",swapPage );
	mapaSWAP->Clear(swapPage);
	OpenFile *swapFile = fileSystem->Open(SWAPFILENAME);
	if ( (swapPage >=0 && swapPage < tamSWAP) == false )
	{
			DEBUG( 'v',"leerSwap: invalid swap position = %d\n", swapPage );
			ASSERT( false );
	}
	if( swapFile == NULL ){
		DEBUG( 'v', "Error(escribirEnSwap): No se pudo habir el archivo de SWAP\n");
		ASSERT(false);
	}
	if ( physicalPage < 0 || physicalPage >= NumPhysPages )
	{
			DEBUG( 'v', "Error(leerSwap): Direccion fisica de memoria inválida: %d\n", physicalPage );
			ASSERT( false );
	}
	swapFile->ReadAt((&machine->mainMemory[physicalPage*PageSize]), PageSize, swapPage*PageSize);
	++stats->numPageFaults;
	//++stats->numDiskReads;
	delete swapFile;
}


//////////////A partir de aquí comienzan los métodos para el uso de secondChanceTLBce///////////////////////////////////
int  AddrSpace::secondChanceTLB()
{
	int freeSpace = -1;
	// para una primera pasada
	for ( int x = 0; x < TLBSize; ++x )
	{
		if ( machine->tlb[ x ].valid == false )
		{
			return x;
		}
	}
	// si llega aquí ya no es la primera pasada
	bool find = false;
	while ( find == false )
	{
		if ( machine->tlb[indiceTLBSecondChance].use == true )
		{
			machine->tlb[indiceTLBSecondChance].use = false;
			salvarVictimaTLB(indiceTLBSecondChance, true );
		}else
		{
			find = true;
			freeSpace = indiceTLBSecondChance;
			salvarVictimaTLB( freeSpace, false );
		}
		indiceTLBSecondChance = (indiceTLBSecondChance+1) % TLBSize;
	}
	if ( freeSpace < 0 || freeSpace >= TLBSize )
	{
		DEBUG('v',"\nsecondChanceTLBceTLBTLB: Invalid tlb information\n");
		imprimirTLB();
		ASSERT( false );
	}
	return freeSpace;
}

void AddrSpace::usarIndiceTLB( int tlbi, int vpn )
{
	if ( tlbi < 0 || tlbi >= TLBSize  )
	{
		DEBUG('v',"\nusarIndiceTLB: invalid params: tlbi = %d, vpn = %d\n", tlbi, vpn);
		imprimirTLB();
		ASSERT(false);
	}
	if ( vpn < 0 || (unsigned int) vpn >= numPages  )
	{
		DEBUG('v',"\nusarIndiceTLB: invalid params: tlbi = %d, vpn = %d\n", tlbi, vpn);
		imprimirPageTable();
		ASSERT(false);
	}
	machine->tlb[tlbi].virtualPage =  pageTable[vpn].virtualPage;
	machine->tlb[tlbi].physicalPage = pageTable[vpn].physicalPage;
	machine->tlb[tlbi].valid = pageTable[vpn].valid;
	machine->tlb[tlbi].use = pageTable[vpn].use;
	machine->tlb[tlbi].dirty = pageTable[vpn].dirty;
	machine->tlb[tlbi].readOnly = pageTable[vpn].readOnly;
}

void AddrSpace::salvarVictimaTLB(int indiceTLB, bool uso)
{
	if ( indiceTLB < 0 || indiceTLB >= TLBSize  )
	{
		DEBUG('v',"\n salvarVictimaTLB: Parametros inválidos = %d\n", indiceTLB);
		ASSERT(false);
	}
	pageTable[machine->tlb[indiceTLB].virtualPage].use = (uso == 1?uso:machine->tlb[indiceTLB].use);
	pageTable[machine->tlb[indiceTLB].virtualPage].dirty = machine->tlb[indiceTLB].dirty;
}

int  AddrSpace::secondChanceSwap()
{
	if ( indiceSWAPSecondChance < 0 || indiceSWAPSecondChance >= NumPhysPages )
	{
		DEBUG('v', "secondChanceSwap:: Invalid indiceSWAPSecondChance value = %d \n", indiceSWAPSecondChance );
		ASSERT( false );
	}
	int freeSpace = -1;
	bool find = false;

	while ( find == false )
	{
		if ( pageTableInvertida[ indiceSWAPSecondChance ] == NULL )
		{
			DEBUG('v', "\nsecondChanceTLBceSWAP:: Invalid pageTableInvertida state\n");
			imprimirPageTableInvertida();
			ASSERT( false );
		}

		if ( pageTableInvertida[ indiceSWAPSecondChance ]->valid == false )
		{
			DEBUG('v', "\nsecondChanceTLBceSWAP:: Invalid pageTableInvertida[%d].valid values, is false\n");
			imprimirPageTableInvertida();
			ASSERT( false );
		}

		if ( pageTableInvertida[ indiceSWAPSecondChance ]->use == true )
		{
				pageTableInvertida[ indiceSWAPSecondChance ]->use = false;
		}else
		{
				freeSpace = indiceSWAPSecondChance;
				find = true;
		}
		indiceSWAPSecondChance = (indiceSWAPSecondChance+1) % NumPhysPages;
	}

	if ( freeSpace < 0 || freeSpace >= NumPhysPages )
	{
		DEBUG('v',"\nsecondChanceTLBceSWAP: Invalid pageTable Index\n");
		imprimirPageTableInvertida();
		ASSERT( false );
	}
	return freeSpace;
}
void AddrSpace::buscarVictimaSwap(int swapi)
{
	for ( int i = 0; i < TLBSize; ++i )
	{
		if ( machine->tlb[ i ].valid && (machine->tlb[ i ].physicalPage == pageTableInvertida[swapi]->physicalPage)  )
		{
			DEBUG('v',"%s\n", "\t\t\tSí estaba la victima en TLB" );
			machine->tlb[ i ].valid = false;
			pageTableInvertida[swapi]->use = machine->tlb[ i ].use;
			pageTableInvertida[swapi]->dirty = machine->tlb[ i ].dirty;
			break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//VM
void AddrSpace::Load(int vpn)
{
imprimirPageTable();
	int libre;
	DEBUG('v', "Numero de paginas: %d, hilo actual: %s\n", numPages, currentThread->getName());
	DEBUG('v', "\tCodigo va de [%d, %d[ \n", 0, initData);
	DEBUG('v',"\tDatos incializados va de [%d, %d[ \n", initData, noInitData);
	DEBUG('v', "\tDatos no incializados va de [%d, %d[ \n", noInitData , stack);
	DEBUG('v',"\tPila va de [%d, %d[ \n", stack, numPages );
if(vpn >= 0 && vpn < initData){ //si la pagina pertenece al segmento de código
DEBUG('v',"\t Pagina de codigo \n");
if(pageTable[vpn].valid == false){ //si la pagina no es valida se debe buscar un frame en memoria
DEBUG('v',"\t Pagina no valida \n");
memoryMan->AllocateFrame(pageTable[vpn], "code", fn);
}
else{ //si ya es valida solamente se debe actualizar el TLB
DEBUG('v',"\t Pagina valida \n");
}
}
else if(vpn >= stack && vpn < numPages){
memoryMan->AllocateFrame(pageTable[vpn], "stack", fn);
}
}
/*
	//Si la pagina no es valida ni esta sucia.
	if ( !pageTable[vpn].valid && !pageTable[vpn].dirty ){
		//Entonces dependiendo del segmento de la pagina, debo tomar la decisión de ¿donde cargar esta pagina?
		DEBUG('v', "\t1-La pagina es invalida y limpia\n");
		DEBUG('v', "\tArchivo fuente: %s\n", fn);
		++stats->numPageFaults;
		OpenFile* executable = fileSystem->Open( fn );
		if (executable == NULL) {
			DEBUG('v',"Unable to open source file %s\n", fn );
			ASSERT(false);
		}
		NoffHeader noffH;
		executable->ReadAt((char *)&noffH, sizeof(noffH), 0);

		//Nesecito verificar a cual segemento pertenece la pagina.
		if(vpn >= 0 && vpn < initData){ //segemento de Codigo
			DEBUG('v', "\t1.1 Página de código\n");
			//Se debe cargar la pagina del archivo ejecutable.
			libre = mapaGlobal->Find();

			if ( libre != -1  )
			{
				DEBUG('v',"\tFrame libre en memoria: %d\n", libre );
				pageTable[ vpn ].physicalPage = libre;
				executable->ReadAt(&(machine->mainMemory[ ( libre * PageSize ) ] ),
				PageSize, noffH.code.inFileAddr + PageSize*vpn );
				pageTable[ vpn ].valid = true;
				//pageTable[ vpn ].readOnly = true;

				//Se actualiza la TLB invertida
				pageTableInvertida[libre] = &(pageTable[ vpn ]);
				//Se debe actualizar el TLB
				int tlbSPace = secondChanceTLB();
				usarIndiceTLB( tlbSPace, vpn );

			}else
			{		//victima swap
				indiceSWAPFIFO = secondChanceSwap();
				buscarVictimaSwap( indiceSWAPFIFO );
				///////////fin de secondChanceTLBce para el SWAP/////////////////
				bool victimDirty = pageTableInvertida[indiceSWAPFIFO]->dirty;

				if ( victimDirty )
				{
					DEBUG('v',"\t\t\tVictima f=%d,l=%d y  sucia\n",pageTableInvertida[indiceSWAPFIFO]->physicalPage, pageTableInvertida[indiceSWAPFIFO]->virtualPage );
					escribirEnSwap( pageTableInvertida[indiceSWAPFIFO]->physicalPage );

					// pedir el nuevo libre
					libre = mapaGlobal->Find();
					// verificar que sea distinto de -1
					if ( -1 == libre )
					{
						printf("No se encontro un frame libre %d\n", libre );
						ASSERT( false );
					}
					// actualizar la pagina física para la nueva virtual vpn
					pageTable[ vpn ].physicalPage = libre;
					//  cargar el código a la memoria
					executable->ReadAt(&(machine->mainMemory[ ( libre * PageSize ) ] ),
					PageSize, noffH.code.inFileAddr + PageSize*vpn );
					// actualizar la validez
					pageTable[ vpn ].valid = true;
					// actualizar la tabla de paginas invertidas
					pageTableInvertida[ libre ] = &( pageTable[ vpn ] );
					// finalmente, actualizar tlb
					int tlbSPace = secondChanceTLB();
					usarIndiceTLB( tlbSPace, vpn );
					//ASSERT(false);
				}else
				{
					DEBUG('v',"\t\t\tVictima f=%d,l=%d y limpia\n",pageTableInvertida[indiceSWAPFIFO]->physicalPage, pageTableInvertida[indiceSWAPFIFO]->virtualPage );
					int oldPhysicalPage = pageTableInvertida[indiceSWAPFIFO]->physicalPage;
					pageTableInvertida[indiceSWAPFIFO]->valid = false;
					pageTableInvertida[indiceSWAPFIFO]->physicalPage = -1;
					mapaGlobal->Clear( oldPhysicalPage );
					//clearPhysicalPage( oldPhysicalPage );

					// cargamos pargina nueva en memoria
					libre = mapaGlobal->Find();
					if ( libre == -1 )
					{
						printf("No se encontro un frame libre %d\n", libre );
						ASSERT( false );
					}
					//++stats->numPageFaults;
					pageTable[ vpn ].physicalPage = libre;
					executable->ReadAt(&(machine->mainMemory[ ( libre * PageSize ) ] ),
					PageSize, noffH.code.inFileAddr + PageSize*vpn );
					pageTable[ vpn ].valid = true;
					pageTableInvertida[ libre ] = &(pageTable [ vpn ]);
					int tlbSPace = secondChanceTLB();
					usarIndiceTLB( tlbSPace, vpn );
				}
				//ASSERT(false);
			}
		}
		else if(vpn >= initData && vpn < noInitData){ //segmento de Datos Inicializados.
			//Se debe cargar la pagina del archivo ejecutable.
			DEBUG('v', "\t1.2 Página de datos Inicializados\n");
			libre = mapaGlobal->Find();

			if ( libre != -1  )
			{
				DEBUG('v',"Frame libre en memoria: %d\n", libre );
				//++stats->numPageFaults;
				pageTable[ vpn ].physicalPage = libre;
				executable->ReadAt(&(machine->mainMemory[ ( libre * PageSize ) ] ),
				PageSize, noffH.code.inFileAddr + PageSize*vpn );
				pageTable[ vpn ].valid = true;

				//Se actualiza la TLB invertida
				pageTableInvertida[libre] = &(pageTable[ vpn ]);
				//Se debe actualizar el TLB
				int tlbSPace = secondChanceTLB();
				usarIndiceTLB( tlbSPace, vpn );
			}else
			{
				//Se debe selecionar una victima para enviar al SWAP
				indiceSWAPFIFO = secondChanceSwap();
				buscarVictimaSwap( indiceSWAPFIFO );

				bool victimDirty = pageTableInvertida[indiceSWAPFIFO]->dirty;
				// revisar si la victima está sucia
				if ( victimDirty )
				{
					//si sí
						//envíarla al swap
						DEBUG('v',"\t\t\tVictima f=%d,l=%d y  sucia\n",pageTableInvertida[indiceSWAPFIFO]->physicalPage, pageTableInvertida[indiceSWAPFIFO]->virtualPage );
						escribirEnSwap( pageTableInvertida[indiceSWAPFIFO]->physicalPage );
						//pedir el nuevo libre
						libre = mapaGlobal->Find();
						//validar ese nuevo libre
						if ( -1 == libre )
						{
							printf("No se encontro un frame libre %d\n", libre );
							ASSERT( false );
						}
						// asignar al pageTable[vpn] es libre
						pageTable[ vpn ].physicalPage = libre;
						// leer del archivo ejecutable
						executable->ReadAt(&(machine->mainMemory[ ( libre * PageSize ) ] ),
						PageSize, noffH.code.inFileAddr + PageSize*vpn );
						//poner valida la paginas
						pageTable[ vpn ].valid = true;
						//actualiza la tabla de paginas invertidas
						pageTableInvertida[ libre ] = &( pageTable[ vpn ] );
						//hacer la actualización en la tlb
						int tlbSPace = secondChanceTLB();
						usarIndiceTLB( tlbSPace, vpn );
				}else
				{
					//si no esta sucia
					DEBUG('v',"\t\t\tVictima f=%d,l=%d y limpia\n",pageTableInvertida[indiceSWAPFIFO]->physicalPage, pageTableInvertida[indiceSWAPFIFO]->virtualPage );
						// rescatar la antigua fisica de la victima
						int oldPhysicalPage = pageTableInvertida[indiceSWAPFIFO]->physicalPage;
						mapaGlobal->Clear( oldPhysicalPage );
						// poner a la victima en  valid = false
						pageTableInvertida[indiceSWAPFIFO]->valid = false;
						//ponerle la pagina fisica en -1
						pageTableInvertida[indiceSWAPFIFO]->physicalPage = -1;
						// pedir el nuevo libre
						libre = mapaGlobal->Find();
						//validar ese nuevo libre
						if ( libre == -1 )
						{
							printf("No se encontro un frame libre %d\n", libre );
							ASSERT( false );
						}
						//asignar ese libre al pageTable[vpn]
						pageTable[ vpn ].physicalPage = libre;
						//leer del archivo ejecutable
						executable->ReadAt(&(machine->mainMemory[ ( libre * PageSize ) ] ),
						PageSize, noffH.code.inFileAddr + PageSize*vpn );
						//valida dicha pageTable[vpn]
						pageTable[ vpn ].valid = true;
						//actualizar tabla de paginas invertidas
						pageTableInvertida[ libre ] = &(pageTable [ vpn ]);
						// actualizar la tlp
						int tlbSPace = secondChanceTLB();
						usarIndiceTLB( tlbSPace, vpn );
				}
				//ASSERT(false);
			}
		}
		else if(vpn >= noInitData && vpn < numPages){ //segemento de Datos No Inicializados o segmento de Pila.
			DEBUG('v',"\t1.3 Página de datos no Inicializado o página de pila\n");
			libre = mapaGlobal->Find();
			DEBUG('v',"\t\t\tSe busca una nueva página para otorgar\n" );
			if ( libre != -1 )
			{
				//DEBUG('v', "Se le otorga una nueva página en memoria\n" );
				pageTable[ vpn ].physicalPage = libre;
				pageTable[ vpn ].valid = true;

				//Se actualiza la TLB invertida
				//clearPhysicalPage(libre);
				pageTableInvertida[libre] = &(pageTable[ vpn ]);

				//Se debe actualizar el TLB
				int tlbSPace = secondChanceTLB();
				usarIndiceTLB( tlbSPace, vpn );
			}else{// usar swap
				indiceSWAPFIFO = secondChanceSwap();
				buscarVictimaSwap( indiceSWAPFIFO );

				bool victimDirty = pageTableInvertida[indiceSWAPFIFO]->dirty;
				// revisamos con la información actualizada a la victima
				if ( victimDirty )
				{
					DEBUG('v',"\t\t\tVictima f=%d,l=%d sucia\n",pageTableInvertida[indiceSWAPFIFO]->physicalPage, pageTableInvertida[indiceSWAPFIFO]->virtualPage );
					escribirEnSwap( pageTableInvertida[indiceSWAPFIFO]->physicalPage );

					// pedir el nuevo libre
					libre = mapaGlobal->Find();

					// verificar que sea distinto de -1
					if ( -1 == libre )
					{
						printf("No se encontro un frame libre %d\n", libre );
						ASSERT( false );
					}

					// actualizar la pagina física para la nueva virtual vpn
					pageTable [ vpn ].physicalPage = libre;
					pageTable [ vpn ].valid = true;

					//actualizo invertida
					pageTableInvertida[ libre ] = &(pageTable [ vpn ]);

					//actualizo el tlb
					int tlbSPace = secondChanceTLB();
					usarIndiceTLB( tlbSPace, vpn );
					//ASSERT(false);
				}else
				{
					DEBUG('v',"\t\t\tVictima f=%d,l=%d limpia\n",pageTableInvertida[indiceSWAPFIFO]->physicalPage, pageTableInvertida[indiceSWAPFIFO]->virtualPage );
					int pagVieja = pageTableInvertida[indiceSWAPFIFO]->physicalPage;
					pageTableInvertida[indiceSWAPFIFO]->valid = false;
					pageTableInvertida[indiceSWAPFIFO]->physicalPage = -1;
					mapaGlobal->Clear(pagVieja);
				        limpiarPagFisica(pagVieja);

					// cargamos pargina nueva en memoria
					libre = mapaGlobal->Find();

					if ( libre == -1 )
					{
						DEBUG('v',"No se encontro un frame libre %d\n", libre );
						ASSERT( false );
					}
					pageTable [ vpn ].physicalPage = libre;
					pageTable [ vpn ].valid = true;

					pageTableInvertida[ libre ] = &(pageTable [ vpn ]);
					// finalmente se actualiza la tlb
					int tlbSPace = secondChanceTLB();
					usarIndiceTLB( tlbSPace, vpn );
				}
				//ASSERT(false);
			}
		}
		else{
			printf("%s %d\n", "Algo muy malo paso, el numero de pagina invalido!", vpn);
			ASSERT(false);
		}
		// Se cierra el Archivo
		delete executable;
	}
	//Si la pagina no es valida y esta sucia.
	else if(!pageTable[vpn].valid && pageTable[vpn].dirty){
		//Debo traer la pagina del area de SWAP.
		DEBUG('v', "\t2- Pagina invalida y sucia\n");
		DEBUG('v', "\t\tPagina física: %d, pagina virtual= %d\n", pageTable[vpn].physicalPage, pageTable[vpn].virtualPage );
		libre = mapaGlobal->Find();
		if(libre != -1)
		{ //Si hay especio en memoria
			DEBUG('v',"%s\n", "Si hay espacio en memoria, solo leemos de SWAP\n" );

			//actualizar su pagina física de la que leo del swap
			int oldSwapPageAddr = pageTable [ vpn ].physicalPage;
			pageTable [ vpn ].physicalPage = libre;
			// cargarla
			leerSwap( libre, oldSwapPageAddr );
			// actualizar tambien su validez
			pageTable [ vpn ].valid = true;
			// actualiza tabla de paginas invertidas
			pageTableInvertida[ libre ] = &(pageTable [ vpn ]);
			//actualizar su posición en la tlb
			int tlbSPace = secondChanceTLB();
			usarIndiceTLB( tlbSPace, vpn );
			//ASSERT(false);
		}
		else
		{
			//Se debe selecionar una victima para enviar al SWAP
			DEBUG('v',"\n%s\n", "Memoria principal agotada.\n" );
			indiceSWAPFIFO = secondChanceSwap();
			buscarVictimaSwap( indiceSWAPFIFO );

			bool victimDirty = pageTableInvertida[indiceSWAPFIFO]->dirty;
			if ( victimDirty )
			{
				DEBUG('v',"\t\t\tVictima f=%d,l=%d sucia\n",pageTableInvertida[indiceSWAPFIFO]->physicalPage, pageTableInvertida[indiceSWAPFIFO]->virtualPage );
				escribirEnSwap( pageTableInvertida[indiceSWAPFIFO]->physicalPage );
				// pido el libre
				libre = mapaGlobal->Find();
				if ( libre == -1 )
				{
					printf("No se encontro un frame libre %d\n", libre );
					ASSERT( false );
				}

				int oldSwapPageAddr = pageTable [ vpn ].physicalPage;
				pageTable [ vpn ].physicalPage = libre;
				// cargamos la pagina que desea desde el SWAP
				leerSwap( libre, oldSwapPageAddr );
				pageTable [ vpn ].valid = true;
				pageTableInvertida[ libre ] = &(pageTable [ vpn ]);

				// finalmente actualizacom tlb
				int tlbSPace = secondChanceTLB();
				usarIndiceTLB( tlbSPace, vpn );
				//ASSERT(false);
			}else
			{
				DEBUG('v',"\t\t\tVictima f=%d,l=%d limpia\n",pageTableInvertida[indiceSWAPFIFO]->physicalPage, pageTableInvertida[indiceSWAPFIFO]->virtualPage );
				int oldPhysicalPage = pageTableInvertida[indiceSWAPFIFO]->physicalPage;
				pageTableInvertida[indiceSWAPFIFO]->valid = false;
				pageTableInvertida[indiceSWAPFIFO]->physicalPage = -1;
				mapaGlobal->Clear( oldPhysicalPage );
				//clearPhysicalPage( oldPhysicalPage );
				libre = mapaGlobal->Find();
				if ( libre == -1 )
				{
					printf("No se encontro un frame libre %d\n", libre );
					ASSERT( false );
				}
				int oldSwapPageAddr = pageTable [ vpn ].physicalPage;
				pageTable [ vpn ].physicalPage = libre;
				// cargamos la pagina que desea desde el SWAP
				leerSwap( libre, oldSwapPageAddr );
				pageTable [ vpn ].valid = true;
				pageTableInvertida[ libre ] = &(pageTable [ vpn ]);
				// finalmente actualizacom tlb
				int tlbSPace = secondChanceTLB();
				usarIndiceTLB( tlbSPace, vpn );
			}
		}
		//ASSERT(false);
	}
DEBUG('v',"Pagina fisica encontrada %d /n", pageTable[vpn].physicalPage);
//imprimirTLB();
//imprimirPageTable();
//imprimirPageTableInvertida();
}*/
