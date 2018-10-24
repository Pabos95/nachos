#include "nachostabla.h"
using namespace std;
NachosOpenFilesTable::NachosOpenFilesTable(){ //el constructor inicializa un bitMap con n items bits
    usage = 0;
    openFilesMap = new BitMap(256);
    openFiles = new int[256];
}
NachosOpenFilesTable::~NachosOpenFilesTable(){
    delete openFilesMap;
    delete openFiles;
}
int NachosOpenFilesTable::Open(int UnixHandle){
    int resultado = openFilesMap->Find();
    if(resultado != -1){
        openFiles[resultado] = UnixHandle;
    }
    return resultado;
}
int NachosOpenFilesTable::Close(int NachosHandle){
    if(openFilesMap->Test(NachosHandle)){
        openFilesMap->Clear(NachosHandle);
        openFiles[NachosHandle] = 0;
        return 1;
    }
    return -1;
}
bool NachosOpenFilesTable::isOpened(int NachosHandle){
    return openFilesMap->Test(NachosHandle);
}
int NachosOpenFilesTable::getUnixHandle( int NachosHandle ){
return openFiles[NachosHandle];
}
void NachosOpenFilesTable::addThread(){ //si hay un nuevo hilo se aumenta la variable de uso de la  tabla
  usage++;
}
void NachosOpenFilesTable::delThread(){ //si hay un nuevo hilo se disiminuye en uno la variable de uso de la tabla
 usage--;
}
