#include <iostream>
#include <memory>
#include <vector>
#include <windows.h>
#include <mmsystem.h>
#include <fstream>
#include <sstream>
#include <cmath>

#pragma comment(lib, "winmm.lib")

DWORD WINAPI MyThreadFunction(LPVOID lpParam) {
    auto threadNumPtr = std::unique_ptr<int>(static_cast<int*>(lpParam));
    const int threadNum = *threadNumPtr;

    std::ostringstream filename;
    filename << "thread_" << threadNum << ".txt";
    std::ofstream outFile(filename.str());
    std::ostringstream output;
    if (!outFile.is_open()) {
        std::cerr << "Ошибка: невозможно открыть файл для потока " << threadNum << "\n";
        return 1;
    }

    for (int i = 0; i < 21; ++i) {
        DWORD currentTime = timeGetTime();
        output << threadNum << "|" << currentTime << "\n";
        for (int j = 0; j < 1'000'000; ++j) {
            for (int k = 0; k < 1'000; ++k) {
            }
        }
    }

    outFile << output.str();

    outFile.close();
    return 0;
}

int main(const int argc, char* argv[]) {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    if (argc < 2 || argc > 3) {
        std::cerr << "Использование: " << argv[0] << " <количество_потоков> [номер_приоритетного_потока]" << "\n";
        return 1;
    }

    int numThreads;
    try {
        numThreads = std::stoi(argv[1]);
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Неверное количество потоков: " << argv[1] << "\n";
        return 1;
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Количество потоков вне допустимого диапазона: " << argv[1] << "\n";
        return 1;
    }

    if (numThreads <= 0) {
        std::cerr << "Количество потоков должно быть положительным числом" << "\n";
        return 1;
    }

    int priorityThreadNum = -1;
    if (argc == 3) {
        try {
            priorityThreadNum = std::stoi(argv[2]);
            if (priorityThreadNum < 1 || priorityThreadNum > numThreads) {
                std::cerr << "Неверный номер приоритетного потока: " << argv[2] << "\n";
                return 1;
            }
        }
        catch (const std::invalid_argument& e) {
            std::cerr << "Неверный номер приоритетного потока: " << argv[2] << "\n";
            return 1;
        }
        catch (const std::out_of_range& e) {
            std::cerr << "Номер приоритетного потока вне допустимого диапазона: " << argv[2] << "\n";
            return 1;
        }
    }

    std::vector<HANDLE> threads(numThreads);

    for (int i = 0; i < numThreads; ++i) {
        auto threadNum = std::make_unique<int>(i + 1);
        threads[i] = CreateThread(nullptr, 0, MyThreadFunction, threadNum.release(), CREATE_SUSPENDED, nullptr);

        if (threads[i] == nullptr) {
            std::cerr << "Ошибка: невозможно создать поток " << i + 1 << "\n";
            threadNum.reset();
            return 1;
        }

        if (priorityThreadNum == i + 1 && !SetThreadPriority(threads[i], THREAD_PRIORITY_HIGHEST)) {
            std::cerr << "Ошибка: невозможно установить приоритет для потока " << i + 1 << "\n";
        }
        else {
            if (!SetThreadPriority(threads[i], THREAD_PRIORITY_NORMAL)) {
                std::cerr << "Ошибка: невозможно установить приоритет для потока " << i + 1 << "\n";
            }
        }
    }

    std::cout << "Нажмите Enter для продолжения..." << "\n";
    std::cin.get();

    for (const HANDLE& thread : threads) {
        if (ResumeThread(thread) == static_cast<DWORD>(-1)) {
            std::cerr << "Ошибка: невозможно возобновить поток" << "\n";
            return 1;
        }
    }

    WaitForMultipleObjects(numThreads, threads.data(), TRUE, INFINITE);

    for (const HANDLE& thread : threads) {
        CloseHandle(thread);
    }

    return 0;
}