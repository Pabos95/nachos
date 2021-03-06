#include "memorymanager.h"
#include "system.h"
void UpdatePageTable(TranslationEntry& pageTableEntry, int tlbIndex){
if(tlbIndex < 0 || tlbIndex >= TLBSize){

}
}
MemoryManager::MemoryManager(int numberOfFrames, bool seconChance){
memoryMap = new BitMap(numberOfFrames);
secondChanceIndex = 0;
timeStamp = 0;
scnChance = true;
for(int i = 0; i <= NumPhysPages - 1 ;i++){
InvertedPageTable[i] = NULL;
}
}
int MemoryManager::GetSecondChanceFrame(TranslationEntry& pageTableEntry){;
//primera pasada
for(int i = 0; i <= TLBSize - 1; i++){
int freeSpace = -1;
if(machine->tlb[i].valid == false){
return i;
}
}
bool find = false;
while(!find){
//si tiene el bit de uso encendido se desactiva
if(machine->tlb[secondChanceIndex].use == true){
machin->tlb[secondChanceIndex].use = false;
	}
//si no se procede a
else{
find = true;
freeSpace = secondChanceIndex;
   }
}
}
int MemoryManager::GetLRUFrame(){
//primera pasada
for(int i = 0; i <= TLBSize - 1; i++){
if(machine->tlb[i].valid == false){
return i;
}
}
int lru = lastAccessTime[0];
for(int i = 0; i <= TLBSize - 1; i++){
if(lastAccessTime[i]  <= lru){
lru = lastAccessTime[i];
} 
}
return lru;
}
void MemoryManager::UpdateTLB(int tlbIndex,TranslationEntry& PageTableEntry){
//aqui se actualiza la tlb
machine->tlb[tlbIndex].virtualPage =  PageTableEntry.virtualPage;
	machine->tlb[tlbIndex].physicalPage = PageTableEntry.physicalPage;
	machine->tlb[tlbIndex].valid = PageTableEntry.valid;
	machine->tlb[tlbIndex].use = PageTableEntry.use;
	machine->tlb[tlbIndex].dirty = PageTableEntry.dirty;
machine->tlb[tlbIndex].readOnly = PageTableEntry.readOnly; 

}
int MemoryManager::AllocateFrame(TranslationEntry& pageTableEntry, string segment, char* file){
timeStamp++;
int freeFrame = 0;
NoffHeader noffH;
DEBUG('v',"\t Buscando frame\n");
if(segment == "code"){
DEBUG('v',"\tSegmento de codigo \n");
freeFrame = memoryMap->Find();
executable = fileSystem->Open(file);
executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
DEBUG('v',"\tFrame encontrada: %d\n", freeFrame);
if(freeFrame != -1){
DEBUG('v',"\tFrame libre en memoria: %d\n", freeFrame);
				pageTableEntry.physicalPage = freeFrame;
				executable->ReadAt(&(machine->mainMemory[ ( freeFrame * PageSize ) ] ),
				PageSize, noffH.code.inFileAddr + PageSize*pageTableEntry.virtualPage );
				pageTableEntry.valid = true;
DEBUG('v',"\t Entrada en la page table marcada como valida \n");
InvertedPageTable[freeFrame] = &pageTableEntry;
DEBUG('v',"\t Page table invertida actualizada \n");
int tlbSpace;
if(scnChance == true){
	tlbSpace = GetSecondChanceFrame();
}
else{
	tlbSpace = GetLRUFrame();
}
UpdateTLB( tlbSpace,pageTableEntry);
}
else if(segment == "stack"){
freeFrame = memoryMap->Find();
//si se encontro un frame libre en memoria
if(freeFrame != -1){
	++stats->numPageFaults;
        pageTableEntry.physicalPage = freeFrame;
        pageTableEntry.valid = true;

}
}
}
return freeFrame;
}
MemoryManager::~MemoryManager(){
}
void MemoryManager::FreeFrame(int FrameNumber){
}
bool MemoryManager::FrameIsAllocated(int frameNumber){
}
int MemoryManager::GetAvailableMemory(){
}
void MemoryManager::UpdateLastAccessTime(int frameNumber){
}
