#include <iostream>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <chrono>
#include <unistd.h>
#include <ctime>
#include <time.h>
#include <random>
#include <sstream>
#include <atomic>

using namespace std;
//const int UNLOCKED = 0;
//const int LOCKED = 1;
const int NUMBER_OF_PHILOSOPHERS = 3;
const int THINK_TIME_MS = 1'000'000;
const int EAT_TIME_MS = 500'000;


#define DEBUG 0
#define RUNTIME 120'000'000

class NullBuffer : public std::streambuf {
public:
    int overflow(int c) override { return c; } // Do nothing and return the character
};

NullBuffer null_buffer;
std::ostream null_stream(&null_buffer);

ostream& debug() {
    if (DEBUG) {
        return cout;
    }
    return null_stream;
}

mutex outputMutex;

std::atomic<bool> start{ false };

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

void use_timer(int amount, long& time_acc)
{
    auto start = std::chrono::high_resolution_clock::now();
    usleep(amount);
    auto stop = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    time_acc += duration.count();
}

class Philosopher : thread
{
private:
    string name;
    long thinkTime = 0;
    long eatTime = 0;
    long hungryTime = 0;
    int id;
    thread mainThread;
    Syncro& syncro;
    int left_chopstick, right_chopstick;
    status mystatus;
    bool stopping = false;
    int eatCount = 0;

public:
    Philosopher(const string& name, Syncro& t, int id) : mainThread(&Philosopher::run, this), syncro(t)
    {
        this->name = name;
        this->id = id;
        this->thinkTime = 0.0;
        this->left_chopstick = id;
        this->right_chopstick = (id + 1) % NUMBER_OF_PHILOSOPHERS;
    }

    status tell_status() { return mystatus; }

    ~Philosopher()
    {
        {
            lock_guard<std::mutex> outLock(outputMutex);
            cout << endl << name << " stats\n";
            cout << "hungrytime: " << hungryTime << "us" << endl;
            cout << "thinkingtime: " << thinkTime << "ms" << endl;
            cout << "eatingtime: " << eatTime << "ms" << endl;
            cout << "times eaten: " << eatCount << endl;
        }

        mainThread.join();
    }

    void run()
    {
        static std::random_device rd;
        static std::mt19937 mt(rd());
        static std::uniform_int_distribution<int> dist(1, 2);
        while (!start) std::this_thread::sleep_for(std::chrono::milliseconds(1));

        while (start) {
            mystatus = THINKING;
            debug() << "philosopher #" << id << " is thinking\n";
            use_timer(THINK_TIME_MS, thinkTime);

            // coin toss
            // start hungry timer
            auto start = std::chrono::high_resolution_clock::now();

            // Always pick up the lower-numbered chopstick first to prevent deadlocks
            int first = std::min(left_chopstick, right_chopstick);
            int second = std::max(left_chopstick, right_chopstick);

            debug() << id << ": up (" << first << ", " << second << ")" << endl;
            syncro.pickUpChopstick(first);
            syncro.pickUpChopstick(second);

            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            hungryTime += duration.count();

            mystatus = EATING;

            debug() << id << " started eating.\n\n";

            use_timer(EAT_TIME_MS, eatTime);
            eatCount++;

            syncro.putDownChopstick(second);
            syncro.putDownChopstick(first);

            mystatus = HUNGRY;
        }


        //debug() << id << " finished eating.\n";
    }
};

const string nameArray[] = { "Yoda", "Obi-Wan", "Rey", "Kanan", "Leia", "Luke", "Ahsoka",
                          "Mace Windu", "Ezra", "Palpatine", "Anakin", "Kylo Ren", "Dooku",
                          "Kit Fisto", "Luminara", "Plo Koon", "Revan", "Thrawn", "Zeb", "Sabine" };

void dine()
{
    Syncro syncro;
    Philosopher* philosophers[NUMBER_OF_PHILOSOPHERS];

    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; i++)
    {
        philosophers[i] = new Philosopher(nameArray[i], syncro, i);
    }
    start = true;
    usleep(RUNTIME);
    start = false;
    debug() << "Stopping philosophers and delaying for cleanup...\n";
    // 2s > eating time + hungry time
    usleep(2'000'000);

    for (auto& philosopher : philosophers) {
        delete philosopher;
    }
}

int main()
{
    cout << "intended runtime: " << (RUNTIME / 1'000'000) << " seconds" << endl;
    dine();
    return 0;
}
