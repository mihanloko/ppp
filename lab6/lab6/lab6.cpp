﻿#include <iostream>
#include <thread>
#include <windows.h>
#include <vector>

using namespace std;

CONDITION_VARIABLE    BufferNotEmpty;
CRITICAL_SECTION           BufferLock;


class Human;
class SpaceShip;


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
    TSemaphore* free;
    TSemaphore* empty;
    TData data;
public:

    void put(TData t) {
        free->P();
        data = t;
        empty->V();
    }

    TData get() {
        TData data;
        empty->P();
        data = this->data;
        free->V();
        return data;
    }

    TChannel(string name1, string name2) {
        free = new TSemaphore(name1.c_str(), 1);
        empty = new TSemaphore(name2.c_str(), 0);
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

void run(Runnable* r) {
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
            /*logSemaphore->P();
            cout << "Отправлен билет с id " << data.id << endl;
            logSemaphore->V();*/
        }
    }
};

vector<Human*> humansQueue;

class Human : public Runnable {
private:

    int id;
    int state = 0;
    // 0 - покупка
    // 1 - полет
    // 2 - ожидание
    // 3 конференция
    // 4 перезапуск
public:
    Human(int id) {
        this->id = id;
    }

    void run() override {
        srand(time(NULL));
        TData data;
        for (;;) {
            switch (state)
            {
            case 0:
                data = ticketChanel->get();
               /* logSemaphore->P();
                cout << "Куплен билет с id " << data.id << endl;
                logSemaphore->V();*/
                state++;
                break;
            case 1:
                logSemaphore->P();
                cout << "Производитель " << id << ": Человек с id " << id << " встает в очередь на транспорт\n";
                logSemaphore->V();

                EnterCriticalSection(&BufferLock);


                // внесем товар в конец очереди 
                humansQueue.push_back(this);

                LeaveCriticalSection(&BufferLock);

                // Если потребитель ждет, разбудим его
                WakeConditionVariable(&BufferNotEmpty);
                
                state++;
                break;
            case 2:
                Sleep(50);
                break;
            case 3:
                /*data = humanChanel->get();
                logSemaphore->P();
                cout << "Человек с id " << id << " отправляется на конференцию\n";
                logSemaphore->V();*/
                conferenceChanel->put(TData(id));
                state++;
                break;
            case 4:
                freeSemaphore->P();
                /*logSemaphore->P();
                cout << "Человек с id " << id << " освободился и начинает заново\n\n";
                logSemaphore->V();*/
                state = 0;
                Sleep(1500);
                break;
            default:
                state = 0;
                break;
            }
        }
    }

    int getId() {
        return id;
    }

    void increaseState() {
        state++;
    }
};

class SpaceShip : public Runnable {
private:
    int id;
    Human *human = 0;
public:
    SpaceShip(int id) {
        this->id = id;
    }

    void run() override {
        srand(time(NULL));
        TData data;
        for (;;) {
            if (human == 0) {
                /*data = spaceshipChanel->get();
                logSemaphore->P();
                cout << "На борт принят человек с id " << data.id << endl;
                logSemaphore->V();
                humanId = data.id;*/
                EnterCriticalSection(&BufferLock);
                SleepConditionVariableCS(
                    &BufferNotEmpty, &BufferLock, INFINITE);

                // Потребитель забирает первый доступный товар
                Human* data = humansQueue.front();
                humansQueue.erase(humansQueue.begin());

                LeaveCriticalSection(&BufferLock);


                human = data;
                logSemaphore->P();
                cout << "Потребитель " << id << ": На борт принят человек с id " << human->getId() << endl;
                logSemaphore->V();

            }
            else {
                Sleep(500);
                /*logSemaphore->P();
                cout << "Человек с id " << human->getId() << " прибывает на конференцию\n";
                logSemaphore->V();*/
                human->increaseState();
                human = 0;
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
                /*logSemaphore->P();
                cout << "Принят человек с id " << data.id << endl;
                logSemaphore->V();*/
                humanId = data.id;
            }
            else {
                /*logSemaphore->P();
                cout << "Человек с id " << humanId << " отправляется на корабль\n";
                logSemaphore->V();*/
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
                /*logSemaphore->P();
                cout << "На конференцию принят человек с id " << data.id << endl;
                logSemaphore->V();*/
                humanId = data.id;
            }
            else {
                Sleep(500);
                /*logSemaphore->P();
                cout << "Человек с id " << humanId << " послушал конференцию и уходит\n";
                logSemaphore->V();*/
                freeSemaphore->V();
                humanId = -1;
            }
        }
    }
};

int main()
{
    InitializeConditionVariable(&BufferNotEmpty);

    InitializeCriticalSection(&BufferLock);

    srand(time(NULL));
    setlocale(LC_ALL, "Russian");
    vector<thread> threads;

    TicketWindow* ticketWindow;
    Spaceport* spaceport;
    Conference* conference;


    ticketWindow = new TicketWindow();
    Human *human1 = new Human(1);
    Human* human2 = new Human(2);
    Human* human3 = new Human(3);
    Human* human4 = new Human(4);
    Human* human5 = new Human(5);
    SpaceShip* spaceShip1 = new SpaceShip(1);
    SpaceShip* spaceShip2 = new SpaceShip(2);
    SpaceShip* spaceShip3 = new SpaceShip(3);
    spaceport = new Spaceport();
    conference = new Conference();

    threads.push_back(thread(run, std::ref(ticketWindow)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(human1)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(human2)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(human3)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(human4)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(human5)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(spaceShip1)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(spaceShip2)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(spaceShip3)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(spaceport)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(conference)));

    for (thread& t : threads) {
        t.join();
    }

    return 0;
}
