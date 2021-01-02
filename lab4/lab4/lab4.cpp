#include <iostream>
#include <windows.h>

class TSemaphore {
private:
    HANDLE Sem;
public:
    void P() {
        WaitForSingleObject(Sem, INFINITE); 
    }
    void V() { 
        ReleaseSemaphore(Sem, 1, NULL); 
    }

    TSemaphore(const char* name, int startState) {
        Sem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, true, (LPCWSTR)name);
        int s = (startState > 0);
        if (Sem == NULL)    
            Sem = CreateSemaphore(NULL, s, 1, (LPCWSTR)name);
    }

    ~TSemaphore() {
        CloseHandle(Sem);
    }
};


class TChannel {
private:
    TSemaphore *free;
    TSemaphore *empty;
    HANDLE fileMem;
    void* buffer;
    TData data;  // здесь храним данные канала
public:
    void put(TData  t);
    TData get(TData* resultData);
    TChannel(char* name1, char *name2) {
        free = new TSemaphore(name1, 1);
        empty = new TSemaphore(name2, 0);
        fileMem = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,
            // все права на файл, кроме FILE_MAP_EXECUTE
            false,     //  handle  не наследуется при CreateProcess
            (LPCWSTR)"MY_NAME");
        if (fileMem == NULL)
            fileMem = CreateFileMapping(
                (HANDLE)0xFFFFFFFF,
                //   INVALID_HANDLE_VALUE --- СОЗДАЕМ НОВЫЙ
                NULL,  // LPSECURITY_ATTRIBUTES 
                PAGE_READWRITE,    //  вид доступа к данным
                0, 4096,   // размер
                (LPCWSTR)"MY_NAME");
        if (fileMem != NULL)
            buffer = MapViewOfFile(
                fileMem,   // Handle файла
                FILE_MAP_ALL_ACCESS,
                0, 0,  // смещение
                4096);   // длина данных
        else {
            printf("error: FILE_MAP \n");
            exit(1);
            // Все плохо!!!!
        }
    }
    ~TChannel() {
        delete free;
        delete empty;
    }
};

int main()
{
    std::string str = "abc";
    std::cout << sizeof(str) << std::endl;
    std::cout << "Hello World!\n";
}
