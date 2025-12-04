#include <format>
#include <iostream>
#include <omp.h>

int main() {
    int x = 44;
    // #pragma omp parallel for private(x)
    // #pragma omp parallel for firstprivate(x)
    // #pragma omp parallel for lastprivate(x)
    for (int i = 0; i <= 10; i++) {
        x = i;
        std::cout << std::format("Thread number: {}\tx: {}\n", omp_get_thread_num(), x);
    }
    std::cout << std::format("x is {}\n", x) << std::flush;
    return 0;
}