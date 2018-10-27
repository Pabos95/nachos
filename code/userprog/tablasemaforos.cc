#include "tablasemaforos.h"
using namespace std;
NachosSemTable::NachosSemTable() { //el constructor inicializa un bitMap con n items bits
    usage = 0;
   semaphoreMap = new BitMap(256);
}
NachosSemTable::~NachosSemTable(){
    delete semaphoreMap;
}
int NachosSemTable::Create(){
    int resultado = semaphoreMap->Find();
    //if(resultado != -1){
      // arregloSemaforos[resultado] = sem;
    //}
    return resultado;
}
int NachosSemTable::Destroy(int semId){
    if(semaphoreMap->Test(semId)){
        semaphoreMap->Clear(semId);
      // arregloSemaforos[semId]->Destroy();
        return 1;
    }
    return -1; //si el semaforo no existe retorna -1
}
void NachosSemTable::addThread(){ //si hay un nuevo hilo se aumenta la variable de uso de la  tabla
  usage++;
}
void NachosSemTable::delThread(){ //si hay un nuevo hilo se disiminuye en uno la variable de uso de la tabla
 usage--;
}
