#include <iostream>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <chrono>
#include <unistd.h>
#include <ctime>
#include <time.h>
#include <random>
using namespace std;
//const int UNLOCKED = 0;
//const int LOCKED = 1;
const int NUMBER_OF_PHILOSOPHERS = 5;

mutex outputMutex;

class Chopstick
    {
private:
    mutex chopTex;
    int status;
public:
    Chopstick() {}
    void lockChopstick()
    {
        chopTex.lock();
    }
    void unlockChopstick()
    {
        chopTex.unlock();
    }
};

class Syncro
{
private:
    bool dining;
    Chopstick chopsticks[NUMBER_OF_PHILOSOPHERS];
public:
    Syncro()
    {
    }
    void putDownChopstick(int id)
    {
        chopsticks[id].unlockChopstick();
    }
    void pickUpChopstick(int id)
    {
        chopsticks[id].lockChopstick();
    }
};

enum status
{
    THINKING,
    EATING,
    HUNGRY
};

void use_timer(int amount, long &time_acc)
{
    auto start = std::chrono::high_resolution_clock::now();
    usleep(amount);
    auto stop = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    time_acc += duration.count();
}

class Philosopher: thread
{
private:
    string name;
    long thinkTime = 0;
    long eatTime = 0;
    long hungryTime = 0;
    int id;
    thread mainThread;
    Syncro &syncro;
    int left_chopstick, right_chopstick;
    status mystatus;

public:
    Philosopher(const string& name, Syncro &t, int id): mainThread(&Philosopher::run, this), syncro(t)
    {
        this->name = name;
        this->id = id;
        this->thinkTime = 0.0;
        this->left_chopstick = id;
        this->right_chopstick = (id + 1) % NUMBER_OF_PHILOSOPHERS;
    }

    status tell_status(){return mystatus;}

    ~Philosopher()
    {
        {
            lock_guard<std::mutex> outLock(outputMutex);
            cout << endl << id << " stats\n";
            cout << "hungrytime: " << hungryTime << endl;
            cout << "thinkingtime: " << thinkTime << endl;
            cout << "eatingtime: " << eatTime << endl;
        }

        mainThread.join();
    }

    void run()
    {
        static std::random_device rd;
        static std::mt19937 mt(rd());
        static std::uniform_int_distribution<int> dist(1, 2);

        while(true) {
            mystatus = THINKING;
            cout << "philosopher #" << id << " is thinking\n";
            use_timer(1000000, thinkTime);
            // coin toss
            // start hungry timer
            auto start = std::chrono::high_resolution_clock::now();
            if (dist(mt) == 1) // if 1, pick up left first
            {
                cout << id << ": up LR (" << left_chopstick << ", " << right_chopstick << ")" << endl;
                syncro.pickUpChopstick(left_chopstick);
                syncro.pickUpChopstick(right_chopstick);
            } else // pick up right first
            {
                cout << id << ": up RL (" << right_chopstick << ", " << left_chopstick << ")" << endl;
                syncro.pickUpChopstick(right_chopstick);
                syncro.pickUpChopstick(left_chopstick);
            }
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
            hungryTime += duration.count();

            mystatus = EATING;
            

            cout << id << " started eating.\n\n";

            if (dist(mt) == 1) // if 1, set down left first
            {
                cout << id << ": down LR (" << left_chopstick << ", " << right_chopstick << ")" << endl;
                syncro.putDownChopstick(left_chopstick);
                syncro.putDownChopstick(right_chopstick);
            } else {
                cout << id << ": down RL (" << right_chopstick << ", " << left_chopstick << ")" << endl;
                syncro.putDownChopstick(right_chopstick);
                syncro.putDownChopstick(left_chopstick);
            }
            mystatus = HUNGRY;
        }


        //cout << id << " finished eating.\n";
    }
};

const string nameArray[] = {"Yoda", "Obi-Wan", "Rey", "Kanan", "Leia", "Luke", "Ahsoka",
                          "Mace Windu", "Ezra", "Palpatine", "Anakin", "Kylo Ren", "Dooku",
                          "Kit Fisto", "Luminara", "Plo Koon", "Revan", "Thrawn", "Zeb", "Sabine"};

void dine()
{
    Syncro syncro;
    Philosopher* philosophers[NUMBER_OF_PHILOSOPHERS];

    for(int i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
    {
        philosophers[i] = new Philosopher(nameArray[i], syncro, i);
    }
    usleep(10'000'000);

    for(auto & philosopher : philosophers)
    {
        delete philosopher;
    }
}

int main()
{
    dine();
    return 0;
}
