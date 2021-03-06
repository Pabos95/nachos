// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H
#include "swapfile.h"
#include "copyright.h"
#include "filesys.h"
#include "noff.h"
#include <string>
#define UserStackSize		1024 	// increase this as necessary!

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable, const char* filename="");	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    AddrSpace(AddrSpace* padre);
    ~AddrSpace();			// De-allocate an address space
    bool PaginaEnArchivo(int page);   //Se encarga de ver si una pagina de memoria se encuentra en el archivo
    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code
    void Load( int vpn);
    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 
    void usarIndiceTLB(int ind, int vpn);
    int  BuscarTLBSecondChance(); //intenta buscar espacio en la TLB mediante el algoritmo de second chance
    int secondChanceSwap(); //aplica el algoritmo de second chance al archivo de swap
    int secondChanceTLB(); //aplica el algoritmo de second chance a la TLB
    void escribirEnSwap(int paginaFisicaVictima); //envia una pagina al archivo swap
    void buscarVictimaSwap(int indiceswap);
    void salvarVictimaTLB( int indiceTLB, bool uso );
void leerSwap( int physicalPage , int swapPage );
void limpiarPagFisica( int pag);
void imprimirPageTable();
void imprimirPageTableInvertida();
void imprimirTLB();
 void actualizarVictimaswap(int indiceswap);
    char* fn; //Nombre del archivo ejecutable
   static const int codigo = 0;
   static const int code = 0;
  unsigned int data;
  unsigned int initData;
  unsigned int noInitData;
  unsigned int stack;
  private:
    TranslationEntry *pageTable;	// Assume linear page table translation  
			// for now!
    unsigned int numPages;		// Number of pages in the virtual 
   				// address space
    NoffHeader noff;
    OpenFile* swap = NULL;
};

#endif // ADDRSPACE_H
