#include "bitmap.h"
#include "machine.h"
#ifndef SWAP_H
#define SWAP_H

class SwapFile {
private:
    OpenFile* swapFile;
    char * swapFileName;

    int numberOfPages;
    BitMap * swapMap;

public:
    SwapFile( int numberOfPages );
    ~SwapFile();

    int saveToSwap( int memoryFrameNumber );    // Save a frame to SWAP and return the swap frame used to store the memory frame
    void loadFromSwap( int swapFrame, int memoryFrameNumber );    // Recover a frame from SWAP and store in specified memory location
};

#endif // SWAP_H
