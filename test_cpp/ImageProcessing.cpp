#include "ImageProcessing.h"

#include <array>
#include <cassert>
#include <limits>
#include <future>

enum RGB {R = 0, G = 1, B = 2};
constexpr auto MAX_VALUE = std::numeric_limits<uint16_t>::max();
using Histogram = std::array<size_t, MAX_VALUE>;

void calculateHistogram(Image& image, int channel, Histogram& histogram)
{
    const double scale = MAX_VALUE - 1;
    const size_t height = image.height();
    const size_t width = image.width();
    const size_t threads = std::thread::hardware_concurrency();

    std::vector<Histogram> partial(threads); // local hystograms for each thread
    std::vector<std::future<void>> tasks;

    const size_t block = (height + threads - 1) / threads;

    for (size_t t = 0; t < threads; ++t) {
        size_t y0 = t * block;
        size_t y1 = std::min(height, y0 + block);

        tasks.emplace_back(std::async(std::launch::async, [&, y0, y1, t]() {
            auto& local = partial[t];
            // more cache-friendly is going through rows
            for (size_t y = y0; y < y1; ++y)
                for (size_t x = 0; x < width; ++x) {
                    const Pixel& p = image.getPixel(x, y);
                    double v = (channel == 0 ? p.r :
                               (channel == 1 ? p.g : p.b));
                    ++local[static_cast<size_t>(v * scale)];
                }
        }));
    }

    for (auto& f : tasks) f.get();

    // merging results
    const size_t histSize = histogram.size();
    for (size_t t = 0; t < threads; ++t)
        for (size_t i = 0; i < histSize; ++i)
            histogram[i] += partial[t][i];
}

void calculateCFH(Histogram& histogram, Histogram& cfh)
{
    size_t sum = 0;

    // let's not call it every time in the cycle
    const size_t histSize = histogram.size();

    for (size_t i = 0; i < histSize; ++i) {
        sum += histogram[i];
        cfh[i] = sum;
    }
}

void equalizeChannel(Image& image, Rect roi, int channel, Histogram& cfh)
{
    // minimize cycle computations
    const int xEnd = roi.x + roi.width;
    const int yEnd = roi.y + roi.height;
    const double scale = MAX_VALUE - 1;
    const double invArea = 1.0d / image.area();
    const size_t threads = std::thread::hardware_concurrency();

    std::vector<std::future<void>> tasks;
    const int block = (roi.height + threads - 1) / threads;

    for (size_t t = 0; t < threads; ++t) {
        int y0 = roi.y + t * block;
        int y1 = std::min(roi.y + roi.height, y0 + block);

        tasks.emplace_back(std::async(std::launch::async, [&, y0, y1]() {
            for (int y = y0; y < y1; ++y)
                for (int x = roi.x; x < xEnd; ++x) {
                    auto pixel = image.getPixel(x, y);
                    double v = (channel == 0 ? pixel.r :
                               (channel == 1 ? pixel.g : pixel.b));
                    size_t idx = static_cast<size_t>(v * scale);
                    double eq = cfh[idx] * invArea;

                    if (channel == 0) pixel.r = eq;
                    else if (channel == 1) pixel.g = eq;
                    else pixel.b = eq;

                    image.setPixel(x, y, pixel);
                }
        }));
    }

    for (auto& f : tasks) f.get();
}

void doHistogramEqualization(Image& image, Rect roi)
{
    // Image include ROI ?
    assert(roi.x >= 0);
    assert(roi.x + roi.width <= image.width());
    assert(roi.y >= 0);
    assert(roi.y + roi.height <= image.height());

    auto worker = [&image, roi](int ch) {
        Histogram hist{}, cfh{};
        calculateHistogram(image, ch, hist);
        calculateCFH(hist, cfh);
        equalizeChannel(image, roi, ch, cfh);
    };

    auto fR = std::async(std::launch::async, worker, 0);
    auto fG = std::async(std::launch::async, worker, 1);
    auto fB = std::async(std::launch::async, worker, 2);

    fR.get();
    fG.get();
    fB.get();
}
