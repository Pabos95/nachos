#include "swapfile.h"
#include "system.h"
SwapFile::SwapFile(int numberOfPages){
swapMap = new BitMap(numberOfPages);
}
int SwapFile::saveToSwap(int memoryFramNumber){
int result = swapMap->Find();
return result;
}
void SwapFile::loadFromSwap(int swapFrame, int memoryFrameNumber){
swapFile = fileSystem->Open(SWAPFILENAME);
}
