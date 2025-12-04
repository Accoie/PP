#define _USE_MATH_DEFINES

#include <chrono>
#include <iostream>
#include <cmath>
#include <omp.h>

double computePiSequential(long long iterations) {
    double pi_approx = 0.0;

    for (long long k = 0; k < iterations; ++k) {
        double denominator = 2.0 * k + 1.0;
        double term = (k % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;
        pi_approx += term;
    }

    return pi_approx * 4.0;
}

double computePiParallelRace(long long iterations) {
    double pi_approx = 0.0;

#pragma omp parallel for
    for (long long k = 0; k < iterations; ++k) {
        double denominator = 2.0 * k + 1.0;
        double term = (k % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;
        pi_approx += term;
    }

    return pi_approx * 4.0;
}

double computePiParallelAtomic(long long iterations) {
    double pi_approx = 0.0;

#pragma omp parallel for
    for (long long k = 0; k < iterations; ++k) {
        double denominator = 2.0 * k + 1.0;
        double term = (k % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;

#pragma omp atomic
        pi_approx += term;
    }

    return pi_approx * 4.0;
}

double computePiParallelReduction(long long iterations) {
    double pi_approx = 0.0;

#pragma omp parallel for reduction(+:pi_approx)
    for (long long k = 0; k < iterations; ++k) {
        double denominator = 2.0 * k + 1.0;
        double term = (k % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;
        pi_approx += term;
    }

    return pi_approx * 4.0;
}

double computePiParallelLocal(long long iterations) {
    double pi_approx = 0.0;

#pragma omp parallel
    {
        double local_sum = 0.0;

#pragma omp for
        for (long long k = 0; k < iterations; ++k) {
            double denominator = 2.0 * k + 1.0;
            double term = (k % 2 == 0) ? 1.0 / denominator : -1.0 / denominator;
            local_sum += term;
        }

#pragma omp critical
        pi_approx += local_sum;
    }

    return pi_approx * 4.0;
}

template<typename Func>
double benchmark(Func&& computation, const std::string& label) {
    auto start_time = std::chrono::high_resolution_clock::now();
    double result = computation();
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << label << ": " << result
        << " (time: " << elapsed.count() << " s)" << std::endl;

    return result;
}

int main() {
    const long long num_iterations = 10000000;

    std::cout << "Comparison of PI calculation methods ("
        << num_iterations << " iterations):\n" << std::endl;

    benchmark([&]() { return computePiSequential(num_iterations); },
        "Sequential method");

    benchmark([&]() { return computePiParallelRace(num_iterations); },
        "Parallel (with data race)");

    benchmark([&]() { return computePiParallelAtomic(num_iterations); },
        "Parallel (atomic)");

    benchmark([&]() { return computePiParallelReduction(num_iterations); },
        "Parallel (reduction)");

    benchmark([&]() { return computePiParallelLocal(num_iterations); },
        "Parallel (local sums)");

    std::cout << "\nExact PI value: " << M_PI << std::endl;

    return 0;
}
