#include <iostream>
#include <thread>
#include <windows.h>
#include <vector>

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

class TData {
public:
    int id;

    TData() {
        id = 0;
    }

    TData(int id) {
        this->id = id;
    }
};

class TChannel {
private:
    TSemaphore *free;
    TSemaphore *empty;
    HANDLE fileMem;
    void* buffer;
public:

    void put(TData t) {
        free->P();
        CopyMemory((PVOID)buffer, &t, sizeof(TData));
        empty->V();
    }

    TData get() {
        TData data;
        empty->P();
        CopyMemory(&data, (PVOID)buffer, sizeof(TData));
        free->V();
        return data;
    }

    TChannel(string name1, string name2) {
        //data = TData();
        free = new TSemaphore(name1.c_str(), 1);
        empty = new TSemaphore(name2.c_str(), 0);
        fileMem = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,
            // все права на файл, кроме FILE_MAP_EXECUTE
            false,     //  handle  не наследуется при CreateProcess
            (LPCWSTR) (name1 + name2).c_str());
        if (fileMem == NULL)
            fileMem = CreateFileMapping(
                (HANDLE)0xFFFFFFFF,
                //   INVALID_HANDLE_VALUE --- СОЗДАЕМ НОВЫЙ
                NULL,  // LPSECURITY_ATTRIBUTES 
                PAGE_READWRITE,    //  вид доступа к данным
                0, 4096,   // размер
                (LPCWSTR) (name1 + name2).c_str());
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


class Runnable {
public:
    virtual void run() = 0;
};

void run(Runnable *r) {
    r->run();
}


TChannel* ticketChanel = new TChannel("ticket1", "ticket2");
TChannel* portChanel = new TChannel("port1", "port2");
TChannel* spaceshipChanel = new TChannel("spaceship1", "spaceship2");
TChannel* humanChanel = new TChannel("human1", "human2");
TChannel* conferenceChanel = new TChannel("conf1", "conf2");
TSemaphore* logSemaphore = new TSemaphore("log", 1);
TSemaphore* freeSemaphore = new TSemaphore("free1", 0);

class TicketWindow : public Runnable {
public:
    void run() override {
        srand(time(NULL));
        for (;;) {
            TData data(rand());
            ticketChanel->put(data);
            logSemaphore->P();
            cout << "Отправлен билет с id " << data.id << endl;
            logSemaphore->V();
        }
    }
};


class Human : public Runnable {
private:
    
    int id = rand();
    int state = 0;
    // 0 - покупка
    // 1 - полет
    // 2 конференция
    // 3 перезапуск
public:
    void run() override {
        srand(time(NULL));
        TData data;
        for (;;) {
            switch (state)
            {
            case 0:
                data = ticketChanel->get();
                logSemaphore->P();
                cout << "Куплен билет с id " << data.id << endl;
                logSemaphore->V();
                state++;
                break;
            case 1:
                logSemaphore->P();
                cout << "Человек с id " << id << " отправляется в порт\n";
                logSemaphore->V();
                portChanel->put(TData(id));
                state++;
                break;
            case 2:
                data = humanChanel->get();
                logSemaphore->P();
                cout << "Человек с id " << id << " отправляется на конференцию\n";
                logSemaphore->V();
                conferenceChanel->put(TData(id));
                state++;
                break;
            case 3:
                freeSemaphore->P();
                logSemaphore->P();
                cout << "Человек с id " << id << " освободился и начинает заново\n\n";
                logSemaphore->V();
                state = 0;
                Sleep(500);
                break;
            default:
                state = 0;
                break;
            }
        }
    }
};

class SpaceShip : public Runnable {
private:
    int humanId = -1;
public:
    void run() override {
        srand(time(NULL));
        TData data;
        for (;;) {
            if (humanId == -1) {
                data = spaceshipChanel->get();
                logSemaphore->P();
                cout << "На борт принят человек с id " << data.id << endl;
                logSemaphore->V();
                humanId = data.id;
            }
            else {
                Sleep(500);
                logSemaphore->P();
                cout << "Человек с id " << humanId << " прибывает на конференцию\n";
                logSemaphore->V();
                humanChanel->put(TData(humanId));
                humanId = -1;
            }
        }
    }
};

class Spaceport : public Runnable {
private:
    int humanId = -1;
public:
    void run() override {
        srand(time(NULL));
        TData data;
        for (;;) {
            if (humanId == -1) {
                data = portChanel->get();
                logSemaphore->P();
                cout << "Принят человек с id " << data.id << endl;
                logSemaphore->V();
                humanId = data.id;
            }
            else {
                logSemaphore->P();
                cout << "Человек с id " << humanId << " отправляется на корабль\n";
                logSemaphore->V();
                spaceshipChanel->put(TData(humanId));
                humanId = -1;
            }
        }
    }
};


class Conference : public Runnable {
private:
    int humanId = -1;
public:
    void run() override {
        srand(time(NULL));
        TData data;
        for (;;) {
            if (humanId == -1) {
                data = conferenceChanel->get();
                logSemaphore->P();
                cout << "На конференцию принят человек с id " << data.id << endl;
                logSemaphore->V();
                humanId = data.id;
            }
            else {
                Sleep(500);
                logSemaphore->P();
                cout << "Человек с id " << humanId << " послушал конференцию и уходит\n";
                logSemaphore->V();
                freeSemaphore->V();
                humanId = -1;
            }
        }
    }
};

int main()
{
    srand(time(NULL));
    setlocale(LC_ALL, "Russian");
    vector<thread> threads;

    TicketWindow* ticketWindow;
    Human* human;
    SpaceShip* spaceShip;
    Spaceport* spaceport;
    Conference* conference;


    ticketWindow = new TicketWindow();
    human = new Human();
    spaceShip = new SpaceShip();
    spaceport = new Spaceport();
    conference = new Conference();

    threads.push_back(thread(run, std::ref(ticketWindow)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(human)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(spaceShip)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(spaceport)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(conference)));

    for(thread &t: threads) {
        t.join();
    }

    return 0;
}
