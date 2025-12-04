#include <iostream>
#include <vector>
#include <chrono>
#include <cstdlib>

using Matrix = std::vector<std::vector<int>>;

Matrix createRandomMatrix(unsigned n, int low, int high) {
    Matrix m(n, std::vector<int>(n));

    for (unsigned i = 0; i < n; ++i) {
        for (unsigned j = 0; j < n; ++j) {
            m[i][j] = low + std::rand() % (high - low + 1);
        }
    }

    return m;
}

Matrix multiply(const Matrix& A, const Matrix& B) {
    unsigned n = A.size();
    Matrix C(n, std::vector<int>(n, 0));

#pragma omp parallel for
    for (int i = 0; i < (int)n; ++i) {
        for (int k = 0; k < (int)n; ++k) {
            for (int j = 0; j < (int)n; ++j) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    return C;
}

void printMatrix(const Matrix& M) {
    for (const auto& row : M) {
        for (int x : row)
            std::cout << x << " ";
        std::cout << "\n";
    }
}

int main() {
    std::srand(time(nullptr));

    unsigned n;
    std::cout << "Matrix size: ";
    std::cin >> n;

    Matrix A = createRandomMatrix(n, -100, 100);
    Matrix B = createRandomMatrix(n, -100, 100);

    auto start = std::chrono::steady_clock::now();

    Matrix C = multiply(A, B);

    auto finish = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = finish - start;

    std::cout << "\nResult matrix:\n";
    printMatrix(C);

    std::cout << "\nExecution time: " << elapsed.count() << " seconds\n";

    return 0;
}
