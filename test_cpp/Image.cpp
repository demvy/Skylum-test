#include "Image.h"
#include "ImageProcessing.h"

#include "jpeg_decoder.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cstring>

constexpr size_t kBufSize = 8 * sizeof(char);

const Pixel& Image::getPixel(size_t x, size_t y) const
{
    // assume, coordinates are correct each time
    return _data[_width * y + x];
}

void Image::setPixel(size_t x, size_t y, const Pixel& value)
{
    // assume, coordinates are correct each time
    _data[_width * y + x] = value;
}

size_t Image::width()  const
{
    return _width;
}

size_t Image::height() const
{
    return _height;
}

size_t Image::area() const
{
    return width() * height();
}

Image::Image(std::string const& path)
{
    auto getFileExtension = [](std::string const& path) -> std::string {
        const auto dotPos = path.find_last_of('.');
        if (dotPos == std::string::npos) return "";
        std::string ext = path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext;
    };
    
    const std::string extension = getFileExtension(path);
    
    if (extension == "jpg" || extension == "jpeg") {
        // This mess from tiny_jpeg library example...
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) {
            printf("Error opening the input file.\n");
            return;
        }
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        unsigned char* buf = static_cast<unsigned char*>(malloc(size));
        rewind(f);
        size_t read = fread(buf, 1, size, f);
        fclose(f);
        
        Jpeg::Decoder decoder(buf, size);
        free(buf);

        if (decoder.GetResult() != Jpeg::Decoder::OK) {
            printf("Error decoding the input file\n");
            return;
        }
        
        if (decoder.IsColor()) {
            _width = decoder.GetWidth();
            _height = decoder.GetHeight();

            // base pointer is the same every time, no need to call in cycle
            const unsigned char* src = decoder.GetImage();

            const size_t total = area();
            _data.resize(total);

            // doubling previous action, redundant + extra copy
            // std::fill_n(_data.begin(), area(), Pixel{});

            constexpr double inv255 = 1.0d / 255.0d;

            for (size_t i = 0; i < total; ++i) {
                const size_t j = i * 3;
                _data[i].r = src[j] * inv255;
                _data[i].g = src[j + 1] * inv255;
                _data[i].b = src[j + 2] * inv255;
            }
        }
    } else if (extension == "ppm") {
        std::ifstream file(path, std::ifstream::binary);
        if (!file.is_open()) {
            std::cerr << "Error opening the input file \"" << path << "\"" << std::endl;
            return;
        }

        std::string header;
        file >> header; 
        if (header == "P6") {

            size_t w = 0, h = 0;
            file >> w >> h;
            file.ignore(2);
            
            _width = w;
            _height = h;
            std::vector<unsigned char> buf(_width * _height * 3);
            file.read(reinterpret_cast<char*>(buf.data()), buf.size());

            const size_t total = _width * _height;
            _data.resize(total);
            // std::fill_n(_data.begin(), area(), Pixel{});

            constexpr double inv255 = 1.0d / 255.0d;

            for (size_t i = 0; i < total; ++i) {
                const size_t j = i * 3;
                _data[i].r = buf[j] * inv255;
                _data[i].g = buf[j + 1] * inv255;
                _data[i].b = buf[j + 2] * inv255;
            }
        }
    }
}

void Image::writeToFile(std::string const& path)
{
    const size_t total = area();
    std::vector<unsigned char> buffer(total * 3);

    const Pixel* src = _data.data();
    unsigned char* dst = buffer.data();

    for (size_t i = 0; i < total; ++i) {
        dst[0] = static_cast<unsigned char>(src[i].r * 255.0 + 0.5);
        dst[1] = static_cast<unsigned char>(src[i].g * 255.0 + 0.5);
        dst[2] = static_cast<unsigned char>(src[i].b * 255.0 + 0.5);
        dst += 3;
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        std::cerr << "Error: cannot write to file \"" << path << "\"\n";
        return;
    }

    file << "P6\n" << _width << " " << _height << "\n255\n";
    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}

