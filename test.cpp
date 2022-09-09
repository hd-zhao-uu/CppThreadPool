#include <cstdio>
#include <iostream>

#include <chrono>
#include <string>
#include <vector>
#include <sstream>

#include "ThreadPool.h"

int main() {
    int threadNum = 8;
    int taskNum = 20;

    ThreadPool pool(threadNum);

    std::vector<std::future<std::string>> results;  // the results of asynchronous operations

    for (int i = 0; i < taskNum; i++) {
        results.emplace_back(
            pool.addTask(
                [i]() {
                    
                    printf("[Task %d] starts\n", i);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::stringstream ss;
                    ss << "I'm task " << i << " in thread " << std::this_thread::get_id();
                    
                    std::string ret = ss.str();
                    return ret;
                })

        );
    }

    for (auto&& result : results)
        std::cout << result.get() << std::endl;

    return 0;
    
}