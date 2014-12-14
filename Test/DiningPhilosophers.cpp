//
// Dining Philosophers implementation that uses
// thread-safe static variables and thread_local
// and avoid deadlocks - common problem for this
//

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <atomic>

class Fork
{
private:
    const int position;
    std::mutex m;
    
public:
    Fork(int position) : position(position) {}
    
    bool pickup(int position) {
        if (m.try_lock()) {
            std::cout << "Philosopher " << position << " picked up fork " << this->position << std::endl;
            return true;
        }
        return false;
    }
    
    void drop(int position) {
        m.unlock();
        std::cout << "Philosopher " << position << " dropped fork " << this->position << std::endl;
    }
};

// a per-thread/per-philosopher record of the amount of food consumed
//thread_local is not support by this compiler verion: thread_local int eaten = 0;
//using atomic instead
std::atomic<int> eaten;

class Philosopher
{
private:
    const int position;
    const std::vector<Fork *> *forks;
    
    void _sleep(uint64_t sleep)
    {
        std::chrono::milliseconds dura( sleep );
        std::this_thread::sleep_for( dura );
    }
    
public:
    Philosopher(int position, std::vector<Fork *> *forks) : position(position), forks(forks) {}
    
    void think() {
        std::cout << "Philosopher " << position << " is thinking" << std::endl;
        _sleep(10);
    }
    
    void eat() {
        std::cout << "Philosopher " << position << " is eating" << std::endl;
        _sleep(10);
    }
    
    void start() {
        // a per-program record of the amount of food available
        static int remaining = 100;
        
        // initialization is safe, but observing and modifying still need manual synchronization
        static std::mutex m;
        
        // assume that the number of forks matches the number of philosophers
        int left = position;
        int right = (position + 1) % forks->size();
        
        for (bool done = false; !done;) {
            think();
            
            // try to pick up the lower-numbered fork
            Fork *lower;
            if (left < right)
                lower = forks->at(left);
            else
                lower = forks->at(right);
            
            if (lower->pickup(position)) {
                // lower-numbered fork picked up, so now must try to get the higher-numbered fork
                Fork *higher;
                if (left < right)
                    higher = forks->at(right);
                else
                    higher = forks->at(left);
                
                if (higher->pickup(position)) {
                    // both forks acquired; eat!
                    eat();
                    
                    m.lock();
                    if (remaining == 0) {
                        done = true;
                    }
                    else {
                        ++eaten;
                        --remaining;
                    }
                    m.unlock();
                    
                    // drop forks after eating
                    higher->drop(position);
                }
                
                // drop lower-numbered fork and go back to thinking
                lower->drop(position);
            }
        }
    }
};

void run(int i, Philosopher &philosopher)
{
    std::cout << "Philosopher " << i << " sat down at the table" << std::endl;
    philosopher.start();
    std::cout << "Philosopher " << i << " ate " << eaten.load() << " noodles." << std::endl;
}

int DiningPhilosophers()
{
    // the number of philosophers and the number of forks
    const int param = 5;
    
    //temporary
    eaten = 0;
    
    std::vector<Fork *> forks;
    for (int i = 0; i < param; ++i)
        forks.push_back(new Fork(i));
    
    std::vector<Philosopher> philosophers;
    std::vector<std::thread> threads;
    for (int i = 0; i < param; ++i) {
        philosophers.push_back(Philosopher(i, &forks));
        threads.push_back(std::thread(run, i, philosophers.at(i)));
    }
    
    for (auto& thread : threads)
        thread.join();
    
    return 0;
}