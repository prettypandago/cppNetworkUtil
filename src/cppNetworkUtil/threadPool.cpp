#include "threadPool.h"

// 构造函数实现
threadPool::threadPool(size_t numThreads) : stop(false)
{
    if (numThreads == 0)
    {
        throw std::invalid_argument("Number of threads cannot be zero.");
    }
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this] {
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] { return !this->tasks.empty() || this->stop; });

                    if (this->stop && this->tasks.empty())
                    {
                        return;
                    }
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}

// 析构函数实现
threadPool::~threadPool()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}