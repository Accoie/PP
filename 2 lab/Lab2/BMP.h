#pragma once
#include <string>
#include <vector>
#include <fstream>
#include "iostream"

class Bitmap {
private:
    int width = 0;
    int height = 0;
    std::vector<unsigned char> data;

public:
    unsigned char* open(std::string filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {

            return nullptr;
        }

        unsigned char header[54];
        file.read(reinterpret_cast<char*>(header), 54);

        width = *(int*)&header[18];
        height = *(int*)&header[22];

        int size = 3 * width * height;
        data.resize(size);

        file.read(reinterpret_cast<char*>(data.data()), size);
        file.close();

        // BGR -> RGB
        for (int i = 0; i < size; i += 3) {
            std::swap(data[i], data[i + 2]);
        }

        return data.data();
    }

    int getWidth() const {
        return width;
    }

    int getHeight() const {
        return height;
    }

    const unsigned char* getData() const {
        return data.data();
    }

    void Save(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            std::cout << "Could not open the file for writing!" << std::endl;
            return;
        }

        unsigned char header[54] = {
            'B', 'M', //Signature
            0, 0, 0, 0, //File size
            0, 0, 0, 0, //Reserved
            54, 0, 0, 0, //Offset to pixel data
            40, 0, 0, 0, //Size of this header
            0, 0, 0, 0, //Width
            0, 0, 0, 0, //Height
            1, 0, //Number of color planes
            24, 0, //Bits per pixel
            0, 0, 0, 0, //Compression
            0, 0, 0, 0, //Image size (can be zero for uncompressed)
            0, 0, 0, 0, //Horizontal resolution (pixels per meter, can be zero)
            0, 0, 0, 0, //Vertical resolution (pixels per meter, can be zero)
            0, 0, 0, 0, //Number of colors in palette (0 means default)
            0, 0, 0, 0  //Important colors (0 means all)
        };

        int fileSize = 54 + data.size(); //Header size + pixel data size
        header[2] = (unsigned char)(fileSize);
        header[3] = (unsigned char)(fileSize >> 8);
        header[4] = (unsigned char)(fileSize >> 16);
        header[5] = (unsigned char)(fileSize >> 24);

        header[18] = (unsigned char)(width);
        header[19] = (unsigned char)(width >> 8);
        header[20] = (unsigned char)(width >> 16);
        header[21] = (unsigned char)(width >> 24);

        header[22] = (unsigned char)(height);
        header[23] = (unsigned char)(height >> 8);
        header[24] = (unsigned char)(height >> 16);
        header[25] = (unsigned char)(height >> 24);

        file.write(reinterpret_cast<char*>(header), 54);

        file.write(reinterpret_cast<char*>(data.data()), data.size());

        file.close();
    }
};