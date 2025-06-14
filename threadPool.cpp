#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional> // For std::function
#include <future>     // For std::future, std::packaged_task

class ThreadPool
{
public:
	// 构造函数：创建指定数量的线程
	ThreadPool(size_t numThreads) : stop(false)
	{
		for (size_t i = 0; i < numThreads; ++i)
		{
			workers.emplace_back([this]   // 使用 lambda 表达式作为线程函数，捕获当前对象
			{
				while (true)
				{
					std::function<void()> task; // 定义一个存储任务的函数对象

					{
						// 作用域开始，锁定互斥量以访问任务队列
						std::unique_lock<std::mutex> lock(this->queueMutex);

						// 等待条件：任务队列不为空，或者线程池已停止
						// 如果任务队列为空且线程池未停止，则线程会在此处阻塞
						this->condition.wait(lock, [this]
						{
							return !this->tasks.empty() || this->stop;
						});

						// 如果线程池已停止且任务队列为空，则当前工作线程退出循环
						if (this->stop && this->tasks.empty())
						{
							return;
						}

						// 从队列中取出任务，使用 std::move 避免不必要的拷贝
						task = std::move(this->tasks.front());
						this->tasks.pop();
					} // 作用域结束，互斥量自动解锁

					task(); // 执行取出的任务
				}
			});
		}
	}

	// 提交任务到线程池
	// 返回一个 std::future，可以用来获取任务的返回值
	template<class F, class... Args>
	auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
	{
		// 定义任务的返回类型
		using return_type = typename std::result_of<F(Args...)>::type;

		// 创建一个 std::packaged_task 来包装函数和参数，并获取其 future
		auto task = std::make_shared<std::packaged_task<return_type() >> (
		    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		    );

		std::future<return_type> res = task->get_future();

		{
			// 作用域开始，锁定互斥量以访问任务队列
			std::unique_lock<std::mutex> lock(queueMutex);

			// 如果线程池已停止，则抛出异常
			if (stop)
			{
				throw std::runtime_error("enqueue on stopped ThreadPool");
			}

			// 将 packaged_task 包装成一个无参数、无返回值的 lambda 函数，并加入任务队列
			tasks.emplace([task]()
			{
				(*task)();
			});
		} // 作用域结束，互斥量自动解锁

		condition.notify_one(); // 通知一个等待的线程有新任务可用
		return res; // 返回 future
	}

	// 析构函数：确保所有线程正确关闭和清理
	~ThreadPool()
	{
		{
			// 作用域开始，锁定互斥量
			std::unique_lock<std::mutex> lock(queueMutex);
			stop = true; // 设置停止标志为 true
		} // 作用域结束，互斥量自动解锁

		condition.notify_all(); // 通知所有等待的线程退出
		for (std::thread &worker : workers)
		{
			if (worker.joinable())   // 检查线程是否可连接（未被 join 或 detached）
			{
				worker.join(); // 等待所有工作线程完成它们的当前任务并退出
			}
		}
	}

private:
	std::vector<std::thread> workers;          // 存储工作线程的容器
	std::queue<std::function<void() >> tasks;  // 存储待执行任务的队列

	std::mutex queueMutex;                     // 用于保护任务队列的互斥量
	std::condition_variable condition;          // 用于线程间通信的条件变量
	bool stop;                                 // 线程池停止标志
};
