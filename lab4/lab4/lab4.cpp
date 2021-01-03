#include <iostream>
#include <thread>
#include <windows.h>

using namespace std;

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


//class TChannel {
//private:
//    TSemaphore *free;
//    TSemaphore *empty;
//    HANDLE fileMem;
//    void* buffer;
//    TData data;  // здесь храним данные канала
//public:
//    void put(TData  t);
//    TData get(TData* resultData);
//    TChannel(char* name1, char *name2) {
//        free = new TSemaphore(name1, 1);
//        empty = new TSemaphore(name2, 0);
//        fileMem = OpenFileMapping(
//            FILE_MAP_ALL_ACCESS,
//            // все права на файл, кроме FILE_MAP_EXECUTE
//            false,     //  handle  не наследуется при CreateProcess
//            (LPCWSTR)"MY_NAME");
//        if (fileMem == NULL)
//            fileMem = CreateFileMapping(
//                (HANDLE)0xFFFFFFFF,
//                //   INVALID_HANDLE_VALUE --- СОЗДАЕМ НОВЫЙ
//                NULL,  // LPSECURITY_ATTRIBUTES 
//                PAGE_READWRITE,    //  вид доступа к данным
//                0, 4096,   // размер
//                (LPCWSTR)"MY_NAME");
//        if (fileMem != NULL)
//            buffer = MapViewOfFile(
//                fileMem,   // Handle файла
//                FILE_MAP_ALL_ACCESS,
//                0, 0,  // смещение
//                4096);   // длина данных
//        else {
//            printf("error: FILE_MAP \n");
//            exit(1);
//            // Все плохо!!!!
//        }
//    }
//    ~TChannel() {
//        delete free;
//        delete empty;
//    }
//};

class Runnable {
public:
    virtual void run() = 0;
};

class test: public Runnable {
public:
    int a = 0;
    double str = 0;
    void run() override {
        cout << "Hello world from method";
    }
};

void run(Runnable& r) {
    r.run();
}


int main()
{
    test t;
    t.a = 5;
    t.str = 4;
    test b;
    CopyMemory(&b, &t, sizeof(t));
    cout << b.a << b.str << endl;
    thread tr([](Runnable& r) {r.run(); }, std::ref(t));
    thread tr1(run, std::ref(t));
    tr.join();
    tr1.join();
}
