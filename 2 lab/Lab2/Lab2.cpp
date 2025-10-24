#include "BMP.h"
#include <iostream>
#include <vector>
#include <windows.h>
#include <chrono>
#include <cstring>
#include <thread>
#include <random>
#include <algorithm>

using namespace std;

struct Params {
    Bitmap* in = nullptr;
    vector<pair<uint32_t, uint32_t>> squares = {};
    uint32_t squareSize = 0;
};

// ==================== Blur Processing Functions ====================

void ProcessPixel(uint32_t x, uint32_t y, int radius, Bitmap* bmp,
    const unsigned char* blurredData, unsigned char* outputData) {
    int width = bmp->getWidth();
    int height = bmp->getHeight();
    int channels = 3;

    int red = 0, green = 0, blue = 0;
    int count = 0;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int newX = static_cast<int>(x) + dx;
            int newY = static_cast<int>(y) + dy;

            if (newX >= 0 && newX < width && newY >= 0 && newY < height) {
                size_t index = (static_cast<size_t>(newY) * width + newX) * channels;
                red += blurredData[index + 0];
                green += blurredData[index + 1];
                blue += blurredData[index + 2];
                count++;
            }
        }
    }

    if (count > 0) {
        size_t index = (static_cast<size_t>(y) * width + x) * channels;
        outputData[index + 0] = static_cast<unsigned char>(blue / count);
        outputData[index + 1] = static_cast<unsigned char>(green / count);
        outputData[index + 2] = static_cast<unsigned char>(red / count);
    }
}

void ProcessSquare(const pair<uint32_t, uint32_t>& square, uint32_t squareSize,
    int radius, Bitmap* bmp, const unsigned char* blurredData,
    unsigned char* outputData) {
    int width = bmp->getWidth();
    int height = bmp->getHeight();

    uint32_t startX = square.first;
    uint32_t startY = square.second;
    uint32_t endX = min(startX + squareSize, static_cast<uint32_t>(width));
    uint32_t endY = min(startY + squareSize, static_cast<uint32_t>(height));

    for (uint32_t y = startY; y < endY; y++) {
        for (uint32_t x = startX; x < endX; x++) {
            ProcessPixel(x, y, radius, bmp, blurredData, outputData);
        }
    }
}

void Blur(int radius, Params* params) {
    Bitmap* bmp = params->in;
    int width = bmp->getWidth();
    int height = bmp->getHeight();
    unsigned char* data = const_cast<unsigned char*>(bmp->getData());

    int channels = 3;
    unsigned char* blurredData = new unsigned char[width * height * channels];
    memcpy(blurredData, data, static_cast<size_t>(width) * height * channels);

    for (const auto& square : params->squares) {
        ProcessSquare(square, params->squareSize, radius, bmp, blurredData, data);
    }

    delete[] blurredData;
}

// ==================== Thread Management Functions ====================

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    Params* params = static_cast<Params*>(lpParam);
    Blur(4, params);
    return 0;
}

HANDLE CreateThreadWithAffinity(Params* params, int threadIndex, int coresCount) {
    HANDLE threadHandle = CreateThread(NULL, 0, &ThreadProc, params, CREATE_SUSPENDED, NULL);

    if (threadHandle != NULL) {
        DWORD_PTR affinityMask = (static_cast<DWORD_PTR>(1) << (threadIndex % coresCount));
        SetThreadAffinityMask(threadHandle, affinityMask);
        ResumeThread(threadHandle);
    }

    return threadHandle;
}

void WaitForAllThreads(HANDLE* handles, int threadsCount) {
    WaitForMultipleObjects(static_cast<DWORD>(threadsCount), handles, TRUE, INFINITE);
}

void CleanupThreadResources(HANDLE* handles, Params* params, int threadsCount) {
    for (int i = 0; i < threadsCount; i++) {
        if (handles[i] != NULL) {
            CloseHandle(handles[i]);
        }
    }
    delete[] handles;
    delete[] params;
}

// ==================== Work Distribution Functions ====================

vector<pair<uint32_t, uint32_t>> GenerateAllSquares(int width, int height, int gridSize) {
    uint32_t squareWidth = static_cast<uint32_t>((width + gridSize - 1) / gridSize);
    uint32_t squareHeight = static_cast<uint32_t>((height + gridSize - 1) / gridSize);

    vector<pair<uint32_t, uint32_t>> allSquares;

    for (int row = 0; row < gridSize; row++) {
        for (int col = 0; col < gridSize; col++) {
            uint32_t startX = col * squareWidth;
            uint32_t startY = row * squareHeight;

            if (startX < static_cast<uint32_t>(width) && startY < static_cast<uint32_t>(height)) {
                allSquares.push_back({ startX, startY });
            }
        }
    }

    return allSquares;
}

void ShuffleSquares(vector<pair<uint32_t, uint32_t>>& squares) {
    random_device rd;
    mt19937 g(rd());
    shuffle(squares.begin(), squares.end(), g);
}

Params* DistributeWorkAmongThreads(const vector<pair<uint32_t, uint32_t>>& allSquares,
    Bitmap* bmp, uint32_t squareSize, int threadsCount) {
    Params* paramsArray = new Params[threadsCount];
    int squaresPerThread = static_cast<int>(allSquares.size()) / threadsCount;
    int remainingSquares = static_cast<int>(allSquares.size()) % threadsCount;

    size_t squareIndex = 0;

    for (int i = 0; i < threadsCount; i++) {
        paramsArray[i].in = bmp;
        paramsArray[i].squareSize = squareSize;

        int currentSquares = squaresPerThread + (i < remainingSquares ? 1 : 0);

        for (int j = 0; j < currentSquares && squareIndex < allSquares.size(); j++) {
            paramsArray[i].squares.push_back(allSquares[squareIndex++]);
        }
    }

    return paramsArray;
}

// ==================== Main Orchestration Functions ====================

void Run(Bitmap* bmp, int threadsCount, int coresCount) {
    int width = bmp->getWidth();
    int height = bmp->getHeight();

    vector<pair<uint32_t, uint32_t>> allSquares = GenerateAllSquares(width, height, threadsCount);
    ShuffleSquares(allSquares);

    uint32_t squareWidth = static_cast<uint32_t>((width + threadsCount - 1) / threadsCount);
    uint32_t squareHeight = static_cast<uint32_t>((height + threadsCount - 1) / threadsCount);
    uint32_t squareSize = max(squareWidth, squareHeight);

    Params* paramsArray = DistributeWorkAmongThreads(allSquares, bmp, squareSize, threadsCount);

    HANDLE* handles = new HANDLE[threadsCount];
    for (int i = 0; i < threadsCount; i++) {
        handles[i] = CreateThreadWithAffinity(&paramsArray[i], i, coresCount);
    }

    WaitForAllThreads(handles, threadsCount);
    CleanupThreadResources(handles, paramsArray, threadsCount);
}

// ==================== Utility & Validation Functions ====================

bool ValidateArguments(int argc, char* argv[]) {
    if (argc != 4) {
        cout << "Args count error\n";
        cout << "Usage: " << argv[0] << " <input.bmp> <threads_count> <cores_count>" << endl;
        return false;
    }

    int threadsCount = atoi(argv[2]);
    int coresCount = atoi(argv[3]);

    if (threadsCount <= 0) {
        cout << "Threads count can't be 0" << endl;
        return false;
    }

    if (coresCount <= 0) {
        cout << "Cores count can't be 0" << endl;
        return false;
    }

    return true;
}

int CalculateIterations(chrono::milliseconds testDuration) {
    if (testDuration.count() < 500) {
        return max(2, 500 / max(1, static_cast<int>(testDuration.count())));
    }
    return 1;
}

void PrintResults(chrono::milliseconds totalDuration, int iterations) {
    cout << "Total execution time: " << totalDuration.count() << " ms" << endl;
    if (iterations > 1) {
        cout << "Applied blur " << iterations << " times" << endl;
        cout << "Average time per blur: " << totalDuration.count() / iterations << " ms" << endl;
    }
}

// ==================== Main Function ====================

int main(int argc, char* argv[]) {
    auto startTime = chrono::high_resolution_clock::now();

    if (!ValidateArguments(argc, argv)) {
        return 1;
    }

    char* imageName = argv[1];
    int threadsCount = atoi(argv[2]);
    int coresCount = atoi(argv[3]);

    Bitmap bmp;
    if (!bmp.open(imageName)) {
        cout << "Failed to open image: " << imageName << endl;
        return 1;
    }

    int iterations = 1;
    auto testStart = chrono::high_resolution_clock::now();
    Run(&bmp, threadsCount, coresCount);
    auto testEnd = chrono::high_resolution_clock::now();

    auto testDuration = chrono::duration_cast<chrono::milliseconds>(testEnd - testStart);
    iterations = CalculateIterations(testDuration);

    if (iterations > 1) {
        cout << "Execution too fast (" << testDuration.count() << "ms), applying blur "
            << iterations << " times" << endl;

        bmp.open(imageName);
        for (int i = 0; i < iterations; i++) {
            Run(&bmp, threadsCount, coresCount);
        }
    }

    string newImageName = string(imageName) + "Blured.bmp";
    bmp.Save(newImageName.c_str());

    auto endTime = chrono::high_resolution_clock::now();
    auto totalDuration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    PrintResults(totalDuration, iterations);

    return 0;
}