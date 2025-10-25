#include <format>
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <stdexcept>
#include "BMPUtils.h"

struct ProgramArgs {
    std::string inputFilePath;
    std::string outputFilePath;
    unsigned coreCount;
    std::vector<int> threadPriorities;
};

ProgramArgs ParseArguments(const int argc, char** argv) {
    if (argc <= 4) {
        throw std::invalid_argument(
            std::format(
                "Usage: {} <input-file-path> <output-file-path> <core-count> <first-thread-priority> <second-thread-priority> <third-thread-priority>",
                argv[0])
        );
    }

    std::vector<int> priorities{};
    for (int i = 4; i < argc; ++i) {
        priorities.push_back(std::stoi(argv[i]));
    }

    return {
        argv[1],
        argv[2],
        static_cast<unsigned>(std::stoi(argv[3])),
        priorities
    };
}

int main(const int argc, char** argv) {
    try {
        const std::clock_t programStart = std::clock();
        constexpr int EXECUTION_COUNT = 1;

        const auto [inputFile, outputFile, cores, threadConfigs] = ParseArguments(argc, argv);
        auto sourceImage = ImageProcessor::LoadImage(inputFile);

        BMPImage processedImage;
        for (int iteration = 0; iteration < EXECUTION_COUNT; ++iteration) {
            processedImage = ImageProcessor::ApplyParallelBlur(sourceImage, threadConfigs);
        }

        ImageProcessor::SaveImage(outputFile, processedImage);

        const auto executionTime = static_cast<double>(std::clock() - programStart) / CLOCKS_PER_SEC * 1000.0;
        std::cout << cores << '\t' << threadConfigs.size() << '\t' << executionTime << std::endl;
    }
    catch (const std::exception& error) {
        std::cerr << "Application error: " << error.what() << std::endl;
        return 1;
    }
    return 0;
}