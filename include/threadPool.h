#pragma once

#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept> // 添加此头文件，因为你在构造函数中使用了 std::invalid_argument

class threadPool
{
public:
    // 构造函数：初始化线程池，创建指定数量的工作线程
    explicit threadPool(size_t numThreads);

    // 提交任务到线程池
    // 任务可以是任何可调用对象（函数、lambda、函数指针等）
    // 返回一个 std::future，用于获取任务的返回值
    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>;

    // 析构函数：在线程池对象销毁时，停止所有工作线程并等待它们完成
    ~threadPool();

private:
    // 禁止拷贝构造和拷贝赋值，因为线程池管理资源（线程）不适合拷贝
    threadPool(const threadPool &) = delete;
    threadPool &operator=(const threadPool &) = delete;

    std::vector<std::thread> workers;        // 存储工作线程的容器
    std::queue<std::function<void()>> tasks; // 存储待执行任务的队列

    std::mutex queueMutex;             // 用于保护任务队列的互斥量
    std::condition_variable condition; // 用于线程间通信的条件变量
    bool stop;                         // 线程池停止标志
};

template <class F, class... Args>
inline auto threadPool::enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop)
        {
            throw std::runtime_error("enqueue on stopped threadPool");
        }
        tasks.emplace([task]()
                      { (*task)(); });
    }
    condition.notify_one();
    return res;
}