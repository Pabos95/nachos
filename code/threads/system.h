// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H
#include "tablasemaforos.h"
#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.
extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock
#ifdef USER_PROGRAM
extern BitMap mapaGlobal;		//Mapa de bits global para saber cuales paginas están ocupadas en la memoria fisica
extern BitMap* mapaSWAP; //bitmap para saber que posiciones del swap estan ocupadas
extern Machine* machine;	// user program memory and registers
extern int indiceSWAPFIFO;
extern int indiceTLBFIFO; 
extern int indiceTLBSecondChance;  //variable global para guardar el indice de la TLB en la que se encuentra el second chance
extern int indiceSWAPSecondChance;//variable global para guardar el indice del swap en la que se encuentra el second chance
extern TranslationEntry* pageTableInvertida[NumPhysPages];
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
