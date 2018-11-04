#ifndef TABLASEMAFOROS_H
#define TABLASEMAFOROS_H
#include "bitmap.h"
using namespace std;
 class NachosSemTable {
  public:
    NachosSemTable();       // Initialize 
    ~NachosSemTable();      // De-allocate
    
    int Create(long semId); // Register the file handle
    int Destroy( int SemId);      // Unregister the file handle
    long getSemaphore(long SemId); //devuelve un long que puede ser castedo a un puntero a semaforos
    void addSem();		// Aumenta en uno el contador
    void delSem();		// Disminuye en uno el contador

    void Print();               // Print contents
    
  private:
   // std::vector<Semaphore> vectorSemaforos;		// A vector with user opened files
    long arregloSemaforos[256];
      BitMap * semaphoreMap;	// A bitmap to control our vector
    int usage;			// How many threads are using this table

};
#endif 

