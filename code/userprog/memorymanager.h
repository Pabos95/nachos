#include "bitmap.h"
#include "translate.h"
#include "synch.h"
#include "filesys.h"
#include <string>
#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

class MemoryManager {

private:
    Lock * memoryLock;    // To allow concurrent access to structures herein
    int numberOfFrames;    // Size of this memory manager
    BitMap * memoryMap;    // BitMap to control which frames are occupied or in use
    TranslationEntry * InvertedPageTable[NumPhysPages];    // Array to keep pointers to process' pageTable
    int * lastAccessTime;            // Array to record the last access time to each frame
    long timeStamp;                // Time counter incremented in each memory access
    int secondChanceIndex;            // Second Chance index in memory
    // Variable list is not exhaustive, you can add all you need
    bool scnChance; //bandera para saber si secondChance sera el algoritmo de reemplazo usado (caso contrario se usa LRU)
    OpenFile* executable;
public:
    MemoryManager( int numberOfFrames, bool seconChance );    // Memory manager constructor
    ~MemoryManager();                // Destructor

    int AllocateFrame( TranslationEntry& pageTableEntry, string segment, char* file);
    int GetLRUFrame();            // One allocation algorithm, return the LRU frame in the lastAccessTime array
    int GetSecondChanceFrame();    // Another allocation algorithm, return the Second Chance first available frame
    void FreeFrame( int frameNumber );    // Free a frame from this manager and reset allocation activity
    bool FrameIsAllocated( int frameNumber );
    void UpdateTLB(int tlbIndex,TranslationEntry& pageTableEntry);
    int GetAvailableMemory();
    void useSecondChanceIndex();
    void useLRUIndex();
    void UpdateLastAccessTime( int frameNumber );    // Used in hardware (translate.cc) to increment the frame's timestamp

    // Method list is not exhaustive, you can add all you need
};
#endif
