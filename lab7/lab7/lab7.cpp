#include <iostream>
#include <thread>
#include <windows.h>
#include <vector>
#include <set>

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

TSemaphore* e = new TSemaphore("SemEEE", 1);
// вход в неделимое действие
TSemaphore* r = new TSemaphore("SemRRR", 0);
// работают  читатели и писатели:

TSemaphore* w = new TSemaphore("SemWWW", 0);
//  ждут читатели и писатели:

int  numR = 0, numW = 0, waitR = 0, waitW = 0;
// количество работающих и ожидающих
// читателей и писателей 

set<int> workingConferences;

void estafeta() {
    if (((numW > 0) || (numR > 0)) && (waitW > 0)) {
        e->V(); // завершить читателей или писателя    
    }
    else if ((numR == 0) && (numW == 0) && (waitW > 0)) {
        waitW--;
            w->V();  // работа писателя               
    }
    else if ((numW == 0) && (waitW == 0) && (waitR > 0)) {
        waitR--;
            r->V();   // читатель  
    }
    else {
        e->V();    // запрос ввода     
    }
}


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

                e->P(); // ждем эстафету на ввод
                if (numW > 0) {
                    waitR++; // появился читатель
                    e->V(); 
                    r->P(); // ждем эстафету на чтение	
                }
                numR++;
                estafeta();   //эстафету получили – работаем!
                
                // read
                logSemaphore->P();
                cout << "Читатель " << id << ": видит что работают конференции ";
                for (int i : workingConferences) {
                    cout << i << " ";
                }
                cout << endl;
                logSemaphore->V();

                e->P(); //  ждем эстафету  -  отметить конец работы
                numR--;
                estafeta();


                data = ticketChanel->get();
                /* logSemaphore->P();
                 cout << "Куплен билет с id " << data.id << endl;
                 logSemaphore->V();*/
                state++;
                break;
            case 1:
                /*logSemaphore->P();
                cout << "Производитель " << id << ": Человек с id " << id << " встает в очередь на транспорт\n";
                logSemaphore->V();*/

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
                //conferenceChanel->put(TData(id));
                state++;
                break;
            case 4:
                //freeSemaphore->P();
                /*logSemaphore->P();
                cout << "Человек с id " << id << " освободился и начинает заново\n\n";
                logSemaphore->V();*/
                state = 0;
                Sleep(100);
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
    Human* human = 0;
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
                /*logSemaphore->P();
                cout << "Потребитель " << id << ": На борт принят человек с id " << human->getId() << endl;
                logSemaphore->V();*/

            }
            else {
                Sleep(10);
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
    int id;
    int lastFetch = 0;
    int step;
    int currentStep = 0;
public:
    Conference(int id, int step) {
        this->id = id;
        this->step = step;
    }

    void run() override {
        srand(id);
        TData data;
        for (;;) {

            //if (humanId == -1) {
            //    data = conferenceChanel->get();
            //    /*logSemaphore->P();
            //    cout << "На конференцию принят человек с id " << data.id << endl;
            //    logSemaphore->V();*/
            //    humanId = data.id;
            //}
            //else {
            //    //Sleep(500);
            //    /*logSemaphore->P();
            //    cout << "Человек с id " << humanId << " послушал конференцию и уходит\n";
            //    logSemaphore->V();*/
            //    freeSemaphore->V();
            //    humanId = -1;
            //}
            /*freeSemaphore->V();*/


            if (lastFetch % 50 == 0) {
                e->P(); // ждем эстафету на ввод запроса
                if (numR > 0 || numW > 0) {
                    waitW++;
                    // появился еще один ожидающий писатель
                    e->V();  w->P(); // ждем эстафету на запись	  
                }
                numW++;

                estafeta();    //эстафету получили – работаем!

                logSemaphore->P();

                currentStep++;
                if (currentStep % step == 0) {
                    workingConferences.erase(id);
                    cout << "Писатель " << id << ": пишет что конференция НЕ работает false\n";
                }
                else {
                    workingConferences.insert(id);
                    cout << "Писатель " << id << ": пишет что конференция работает true\n";
                }
                logSemaphore->V();



                //  ждем эстафету - отметить конец
                e->P();
                numW--;
                estafeta();
            }
            Sleep(10);
            lastFetch++;

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


    ticketWindow = new TicketWindow();
    Human* human1 = new Human(1);
    Human* human2 = new Human(2);
    Human* human3 = new Human(3);
    Human* human4 = new Human(4);
    Human* human5 = new Human(5);
    SpaceShip* spaceShip1 = new SpaceShip(1);
    SpaceShip* spaceShip2 = new SpaceShip(2);
    SpaceShip* spaceShip3 = new SpaceShip(3);
    SpaceShip* spaceShip4 = new SpaceShip(4);
    SpaceShip* spaceShip5 = new SpaceShip(5);
    spaceport = new Spaceport();

    Conference *conference1 = new Conference(1, 2);
    Conference *conference2 = new Conference(2, 3);
    Conference *conference3 = new Conference(3, 4);
    Conference *conference4 = new Conference(4, 5);


    threads.push_back(thread(run, std::ref(conference1)));
    Sleep(3);
    threads.push_back(thread(run, std::ref(conference2)));
    Sleep(5);
    threads.push_back(thread(run, std::ref(conference3)));
    Sleep(8);
    threads.push_back(thread(run, std::ref(conference4)));
    Sleep(1);
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
    threads.push_back(thread(run, std::ref(spaceShip4)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(spaceShip5)));
    Sleep(1);
    threads.push_back(thread(run, std::ref(spaceport)));

    for (thread& t : threads) {
        t.join();
    }

    return 0;
}
