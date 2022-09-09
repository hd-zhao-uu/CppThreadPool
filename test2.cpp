// test code modified from
// https://stackoverflow.com/questions/33713129/compute-pi-value-using-monte-carlo-method-multithreading

#include <cmath>
#include <future>
#include <iostream>
#include <random>
#include <vector>
#include <limits>

#include "ThreadPool.h"

using namespace std;

long random_circle_sampling(long n_samples) {
    std::random_device rd;   // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 1.0);
    long points_inside = 0;
    for (long i = 0; i < n_samples; ++i) {
        double x = dis(gen);
        double y = dis(gen);
        if (x * x + y * y <= 1.0) {
            ++points_inside;
        }
    }
    return points_inside;
}

double approximate_pi(long tot_samples, int pool_size, int n_tasks) {
    long samples_per_thread = tot_samples / n_tasks;

    // Used to store the future results
    vector<future<long>> futures;

    ThreadPool pool(pool_size);

    for (int t = 0; t < n_tasks; ++t) {
        // Start a new asynchronous task
        futures.emplace_back(
            pool.addTask(random_circle_sampling, samples_per_thread));
    }

    long tot_points_inside = 0;
    for (future<long>& f : futures) {
        // Wait for the result to be ready
        tot_points_inside += f.get();
    }

    double pi = 4.0 * (double)tot_points_inside / (double)tot_samples;
    return pi;
}

int main() {
    cout.precision(32);

    long tot_samples = std::numeric_limits<int>::max();
    int pool_size = 16;
    int n_tasks = 1024;

    double pi = 3.14159265358979323846;
    double approx_pi = approximate_pi(tot_samples, pool_size, n_tasks);
    double abs_diff = abs(pi - approx_pi);
    cout << "pi\t\t" << pi << endl;
    cout << "approx_pi\t" << approx_pi << endl;
    cout << "abs_diff\t" << abs_diff << endl;


    return 0;
}
