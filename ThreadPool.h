#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <vector>
#include <thread>

class ThreadPool {
private:
    using Task = std::function<void()>;
    struct Tasks {
        std::queue <Task> taskQueue;  // task queue
        std::mutex qLock;             // lock for the task queue
    };

    size_t threadNum;
    Tasks tasks;
    std::vector <std::thread> pool;  // thread pool
    std::condition_variable cond;    // task queue
    std::atomic<bool> terminated;    // the state of the thread pool

public:
    ThreadPool(size_t threadNum);

    // not copyable
    ThreadPool(const ThreadPool &) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;

    // unmovable
    ThreadPool(ThreadPool &&) = delete;

    ThreadPool &operator=(ThreadPool &&) = delete;

    ~ThreadPool();

    // add a task to the queue
    template<typename Func, typename... Args>
    auto addTask(Func &&func, Args &&... args) -> std::future<decltype(func(args...))>;
};

ThreadPool::ThreadPool(size_t threadNum)
        : terminated(false), threadNum(threadNum) {
    for (size_t i = 0; i < threadNum; i++) {
        pool.emplace_back(
                // thread definition
                [this]() -> void {
                    while (true) {
                        std::unique_lock <std::mutex> locker(tasks.qLock);
                        // wait until that the task queue is not empty
                        cond.wait(locker, [this]() {
                            return terminated || !tasks.taskQueue.empty();
                        });

                        // if the thread-pool is terminated,
                        // try to keep doing the task in the task queue
                        if (terminated && tasks.taskQueue.empty())
                            return;

                        // extract the front-most task from the task queue
                        Task task = std::move(tasks.taskQueue.front());
                        tasks.taskQueue.pop();

                        locker.unlock();

                        // do the task
                        task();
                    }
                });
    }
}

template<typename Func, typename... Args>
auto ThreadPool::addTask(Func &&func, Args &&... args)
-> std::future<decltype(func(args...))> {
    if (terminated)
        throw std::runtime_error("[Error] Add task to a terminated threadPool.");

    // get the return type of `func`
    // std::result_of<Func(Args...)>::type
    using retType =
    decltype(func(args...));

    // provider
    auto task = std::make_shared < std::packaged_task < retType() >> (
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

    // get the future object associated with the previous `packaged_task`
    std::future <retType> futRet = task->get_future();  // can use `.get` to take the result when the task is done

    std::unique_lock <std::mutex> locker(tasks.qLock);

    // add task to the queue
    (tasks.taskQueue).emplace([task]() {
        (*task)();  // `packaged_task` gets wrapped in `function<void()>` by using `std::bind`
    });
    locker.unlock();

    cond.notify_one();
    return futRet;
}

ThreadPool::~ThreadPool() {
    terminated = true;
    cond.notify_all();

    for (auto &worker: pool) {
        if (worker.joinable())
            worker.join();
    }
}

#endif