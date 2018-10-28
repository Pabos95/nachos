#include "nachostabla.h"
using namespace std;
NachosOpenFilesTable::NachosOpenFilesTable(){ //el constructor inicializa un bitMap con n items bits
    usage = 0;
    openFilesMap = new BitMap(256); //se hace un bitmap con el numero maximo de archivos que se puede tener abierto
    openFiles = new int[256]; //
    for(int i = 0; i <= 2; i++){
        openFilesMap->Mark(i);
        openFiles[i] = i;
    }
    for(int i = 0; i <= 255; i++){
    openFiles[i] = i;
    }
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
