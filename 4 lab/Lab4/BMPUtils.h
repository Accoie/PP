#pragma once
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <windows.h>
#include <iostream>
#include <format>
#include <stdexcept>
#include <algorithm>

#pragma pack(push, 1)

struct BMPFileHeader {
    uint16_t fileType;
    uint32_t fileSize;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offsetData;
};

struct BMPInfoHeader {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t sizeImage;
    int32_t xPixelsPerMeter;
    int32_t yPixelsPerMeter;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};

#pragma pack(pop)

struct BMPImage {
    BMPFileHeader fileHeader{};
    BMPInfoHeader infoHeader{};
    std::vector<uint8_t> pixelData;
};

class ImageProcessor {
private:
    static constexpr int KERNEL_SIZE = 3;
    static constexpr double GAUSSIAN_KERNEL[3][3] = {
        {1.0, 2.0, 1.0},
        {2.0, 4.0, 2.0},
        {1.0, 2.0, 1.0}
    };

public:
    static BMPImage LoadImage(const std::string& filePath) {
        BMPImage bmpImage;

        std::ifstream file(filePath, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filePath);
        }

        file.read(reinterpret_cast<char*>(&bmpImage.fileHeader), sizeof(bmpImage.fileHeader));

        if (bmpImage.fileHeader.fileType != 0x4D42) {
            throw std::runtime_error("Not a valid BMP file.");
        }

        file.read(reinterpret_cast<char*>(&bmpImage.infoHeader), sizeof(bmpImage.infoHeader));

        if (bmpImage.infoHeader.sizeImage == 0) {
            int rowSize = ((bmpImage.infoHeader.width * bmpImage.infoHeader.bitCount + 31) / 32) * 4;
            bmpImage.infoHeader.sizeImage = rowSize * bmpImage.infoHeader.height;
        }

        bmpImage.pixelData.resize(bmpImage.infoHeader.sizeImage);
        file.seekg(bmpImage.fileHeader.offsetData, std::ios::beg);
        file.read(reinterpret_cast<char*>(bmpImage.pixelData.data()), bmpImage.infoHeader.sizeImage);

        return bmpImage;
    }

    static void SaveImage(const std::string& filePath, const BMPImage& bmpImage) {
        std::ofstream file(filePath, std::ios::binary);

        if (!file) {
            throw std::runtime_error("Cannot open file: " + filePath);
        }

        file.write(reinterpret_cast<const char*>(&bmpImage.fileHeader), sizeof(bmpImage.fileHeader));
        file.write(reinterpret_cast<const char*>(&bmpImage.infoHeader), sizeof(bmpImage.infoHeader));
        file.write(reinterpret_cast<const char*>(bmpImage.pixelData.data()), bmpImage.pixelData.size());
    }

private:
    struct ThreadContext {
        int threadId;
        BMPImage* sourceImage;
        BMPImage* resultImage;
        int startLine;
        int endLine;
        std::ofstream* performanceLog;
        HANDLE* syncLock;
        std::chrono::time_point<std::chrono::high_resolution_clock>* timeReference;
        int samplingRate;
    };

    static void HeavyComputation() {
        volatile double result = 0.0;
        for (int i = 0; i < 50; ++i) {
            result += std::sin(i * 0.1) * std::cos(i * 0.05);
        }
    }

    static int CalculatePixelOffset(int x, int y, int width, int bytesPerPixel, int stride) {
        return y * stride + x * bytesPerPixel;
    }

    static bool IsValidCoordinate(int x, int y, int width, int height) {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    static uint8_t ApplyGaussianFilter(int centerX, int centerY, const BMPImage* image, int channelOffset) {
        const int width = image->infoHeader.width;
        const int height = image->infoHeader.height;
        const int bytesPerPixel = image->infoHeader.bitCount / 8;
        const int stride = ((width * bytesPerPixel + 3) / 4) * 4;

        double weightedSum = 0.0;
        double kernelSum = 0.0;

        for (int ky = -1; ky <= 1; ++ky) {
            for (int kx = -1; kx <= 1; ++kx) {
                const int sampleX = centerX + kx;
                const int sampleY = centerY + ky;

                if (IsValidCoordinate(sampleX, sampleY, width, height)) {
                    const int pixelIndex = CalculatePixelOffset(sampleX, sampleY, width, bytesPerPixel, stride);
                    const double weight = GAUSSIAN_KERNEL[ky + 1][kx + 1];
                    weightedSum += image->pixelData[pixelIndex + channelOffset] * weight;
                    kernelSum += weight;
                }
            }
        }

        return static_cast<uint8_t>(std::clamp(weightedSum / kernelSum, 0.0, 255.0));
    }

    static DWORD WINAPI ProcessImageSegment(LPVOID context) {
        ThreadContext* data = static_cast<ThreadContext*>(context);

        const int width = data->sourceImage->infoHeader.width;
        const int height = data->sourceImage->infoHeader.height;
        const int bytesPerPixel = data->sourceImage->infoHeader.bitCount / 8;
        const int stride = ((width * bytesPerPixel + 3) / 4) * 4;

        int processedLines = 0;
        const int totalLines = data->endLine - data->startLine;

        for (int y = data->startLine; y < data->endLine; ++y) {
            for (int x = 0; x < width; ++x) {
                const int targetIndex = CalculatePixelOffset(x, y, width, bytesPerPixel, stride);

                data->resultImage->pixelData[targetIndex] =
                    ApplyGaussianFilter(x, y, data->sourceImage, 0);
                data->resultImage->pixelData[targetIndex + 1] =
                    ApplyGaussianFilter(x, y, data->sourceImage, 1);
                data->resultImage->pixelData[targetIndex + 2] =
                    ApplyGaussianFilter(x, y, data->sourceImage, 2);
            }

            processedLines++;

            if (processedLines % data->samplingRate == 0) {
                auto currentTimestamp = std::chrono::high_resolution_clock::now();
                auto timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTimestamp - *data->timeReference).count();

                HeavyComputation();

                WaitForSingleObject(*data->syncLock, INFINITE);
                *data->performanceLog << data->threadId << "\t" << timeElapsed << "\n";
                ReleaseSemaphore(*data->syncLock, 1, nullptr);
            }
        }

        return 0;
    }

public:
    static BMPImage ApplyParallelBlur(BMPImage& sourceImage, const std::vector<int>& threadConfigurations) {
        std::vector<std::ofstream> logFiles;
        std::vector<std::string> filenames = { "performance_1.txt", "performance_2.txt", "performance_3.txt" };

        for (const auto& filename : filenames) {
            logFiles.emplace_back(filename);
        }

        HANDLE synchronizationLock = CreateSemaphore(nullptr, 1, 1, nullptr);
        if (!synchronizationLock) {
            throw std::runtime_error("Failed to create synchronization object");
        }

        BMPImage processedImage = sourceImage;
        std::vector<HANDLE> workerThreads(threadConfigurations.size());
        std::vector<ThreadContext> threadContexts(threadConfigurations.size());

        const int linesPerSegment = sourceImage.infoHeader.height / static_cast<int>(threadConfigurations.size());
        auto globalStartTime = std::chrono::high_resolution_clock::now();

        std::vector<int> threadIdentifiers(threadConfigurations.size());
        for (size_t i = 0; i < threadConfigurations.size(); ++i) {
            threadIdentifiers[i] = static_cast<int>(i) + 1;

            const int segmentStart = static_cast<int>(i) * linesPerSegment;
            const int segmentEnd = (i == threadConfigurations.size() - 1) ?
                sourceImage.infoHeader.height : segmentStart + linesPerSegment;

            threadContexts[i] = {
                threadIdentifiers[i],
                &sourceImage,
                &processedImage,
                segmentStart,
                segmentEnd,
                &logFiles[i],
                &synchronizationLock,
                &globalStartTime,
                10
            };

            workerThreads[i] = CreateThread(nullptr, 0, ProcessImageSegment, &threadContexts[i], 0, nullptr);

            if (!workerThreads[i]) {
                throw std::runtime_error("Failed to create worker thread " + std::to_string(i));
            }

            int priorityLevel = THREAD_PRIORITY_NORMAL;
            if (threadConfigurations[i] > 0) {
                priorityLevel = THREAD_PRIORITY_HIGHEST;
            }
            else if (threadConfigurations[i] < 0) {
                priorityLevel = THREAD_PRIORITY_LOWEST;
            }

            if (!SetThreadPriority(workerThreads[i], priorityLevel)) {
                std::cerr << "Warning: Could not set priority for thread " << i << std::endl;
            }
        }

        WaitForMultipleObjects(static_cast<DWORD>(workerThreads.size()), workerThreads.data(), TRUE, INFINITE);

        for (auto thread : workerThreads) {
            if (thread) CloseHandle(thread);
        }

        CloseHandle(synchronizationLock);

        std::cout << "Image processing completed with " << threadConfigurations.size() << " threads\n";
        return processedImage;
    }
};