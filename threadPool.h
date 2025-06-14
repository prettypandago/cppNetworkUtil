#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional> // For std::function, std::bind
#include <future>     // For std::future, std::packaged_task
#include <stdexcept>  // For std::runtime_error

// ThreadPool 类定义
class ThreadPool
{
public:
	// 构造函数：初始化线程池，创建指定数量的工作线程
	explicit ThreadPool(size_t numThreads);

	// 提交任务到线程池
	// 任务可以是任何可调用对象（函数、lambda、函数指针等）
	// 返回一个 std::future，用于获取任务的返回值
	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

	// 析构函数：在线程池对象销毁时，停止所有工作线程并等待它们完成
	~ThreadPool();

private:
	// 禁止拷贝构造和拷贝赋值，因为线程池管理资源（线程）不适合拷贝
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	std::vector<std::thread> workers;          // 存储工作线程的容器
	std::queue<std::function<void() >> tasks;  // 存储待执行任务的队列

	std::mutex queueMutex;                     // 用于保护任务队列的互斥量
	std::condition_variable condition;          // 用于线程间通信的条件变量
	bool stop;                                 // 线程池停止标志
};

// 构造函数实现（模板函数通常需要在头文件中定义）
inline ThreadPool::ThreadPool(size_t numThreads) : stop(false)
{
	if (numThreads == 0)
	{
		throw std::invalid_argument("Number of threads cannot be zero.");
	}
	for (size_t i = 0; i < numThreads; ++i)
	{
		workers.emplace_back([this]
		{
			while (true)
			{
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->queueMutex);
					this->condition.wait(lock, [this]
					{
						return !this->tasks.empty() || this->stop;
					});

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

// enqueue 模板函数的实现 (必须在头文件中)
template<class F, class... Args>
inline auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;

	auto task = std::make_shared<std::packaged_task<return_type() >> (
	    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	    );

	std::future<return_type> res = task->get_future();

	{
		std::unique_lock<std::mutex> lock(queueMutex);
		if (stop)
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}
		tasks.emplace([task]()
		{
			(*task)();
		});
	}
	condition.notify_one();
	return res;
}

// 析构函数实现
inline ThreadPool::~ThreadPool()
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

#endif // THREAD_POOL_H
