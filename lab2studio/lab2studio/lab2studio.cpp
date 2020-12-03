#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <locale.h>
#include "CImg-2.9.3_pre101320/CImg.h"

#define R 0
#define G 1
#define B 2

using namespace cimg_library;
using namespace std;

CImg<unsigned char> image = CImg<>("small.bmp").normalize(0, 255);
CImg<unsigned char> result = CImg<>(image);
CImg<unsigned char> resultParallel = CImg<>(image);
vector<vector<double>> kernel = {
        {0.75, 0.75, 0.75},
        {0.75, 0.75, 0.75},
        {0.75, 0.75, 0.75}
};


void testParallel(int x_start, int x_finish) {
    int kernelWidth = kernel[0].size();
    int kernelHeight = kernel.size();
    int height = image.height(), width = image.width();

    double rSum = 0, gSum = 0, bSum = 0, kSum = 0;

    for (int x = x_start; x < x_finish; x++) {
        for (int y = 0; y < height; y++) {
            double rSum = 0, gSum = 0, bSum = 0, kSum = 0;

            for (int i = 0; i < kernelWidth; i++) {
                for (int j = 0; j < kernelHeight; j++) {
                    int pixelPosX = x + (i - (kernelWidth / 2));
                    int pixelPosY = y + (j - (kernelHeight / 2));
                    if ((pixelPosX < 0) || (pixelPosX >= width) || (pixelPosY < 0) || (pixelPosY >= height))
                        continue;

                    byte r = image(x, y, R);
                    byte g = image(x, y, G);
                    byte b = image(x, y, B);

                    double kernelVal = kernel[i][j];

                    rSum += r * kernelVal;
                    gSum += g * kernelVal;
                    bSum += b * kernelVal;

                    kSum += kernelVal;
                }
            }

            if (kSum <= 0) kSum = 1;

            //Контролируем переполнения переменных
            rSum /= kSum;
            if (rSum < 0) rSum = 0;
            if (rSum > 255) rSum = 255;

            gSum /= kSum;
            if (gSum < 0) gSum = 0;
            if (gSum > 255) gSum = 255;

            bSum /= kSum;
            if (bSum < 0) bSum = 0;
            if (bSum > 255) bSum = 255;

            //Записываем значения в результирующее изображение
            result(x, y, R) = (byte)rSum;
            result(x, y, G) = (byte)gSum;
            result(x, y, B) = (byte)bSum;
        }
    }
}

int main() {
    setlocale(LC_ALL, "Russian");
    vector<thread> threads;

    cout << "Размер изображения: " << image.height() << " * " << image.width() << "\n";
    cout << "Размер ядра свертки: " << kernel.size() << " * " << kernel.size() << "\n";
    int height = image.height(), width = image.width();


    int kernelWidth = kernel[0].size();
    int kernelHeight = kernel.size();

    cout << "Выполняем фтльтрацию в одном потоке\n";
    auto begin = chrono::high_resolution_clock::now();
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            double rSum = 0, gSum = 0, bSum = 0, kSum = 0;

            for (int i = 0; i < kernelWidth; i++) {
                for (int j = 0; j < kernelHeight; j++) {
                    int pixelPosX = x + (i - (kernelWidth / 2));
                    int pixelPosY = y + (j - (kernelHeight / 2));
                    if ((pixelPosX < 0) || (pixelPosX >= width) || (pixelPosY < 0) || (pixelPosY >= height))
                        continue;

                    byte r = image(x, y, R);
                    byte g = image(x, y, G);
                    byte b = image(x, y, B);

                    double kernelVal = kernel[i][j];

                    rSum += r * kernelVal;
                    gSum += g * kernelVal;
                    bSum += b * kernelVal;

                    kSum += kernelVal;
                }
            }

            if (kSum <= 0) kSum = 1;

    //        //Контролируем переполнения переменных
            rSum /= kSum;
            if (rSum < 0) rSum = 0;
            if (rSum > 255) rSum = 255;

            gSum /= kSum;
            if (gSum < 0) gSum = 0;
            if (gSum > 255) gSum = 255;

            bSum /= kSum;
            if (bSum < 0) bSum = 0;
            if (bSum > 255) bSum = 255;

    //        //Записываем значения в результирующее изображение
            result(x, y, R) = (byte)rSum;
            result(x, y, G) = (byte)gSum;
            result(x, y, B) = (byte)bSum;
        }
    }
    auto elapsedTime = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - begin).count();
    cout << "Затраченное время " << elapsedTime << " милисекунд\n";

    int threadsCount = 24;
    int step = (width / threadsCount) + 1;    


    cout << "Выполняем фильтрацию в " << threadsCount << " потоках\n";
    begin = chrono::high_resolution_clock::now();
    for (int x = 0; x < width; x += step) {
        thread t(testParallel, x, min(x + step, width));
        threads.push_back(move(t));
    }

    for (thread& t : threads) {
        t.join();
    }
    elapsedTime = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - begin).count();
    
    cout << "Затраченное время " << elapsedTime << " милисекунд\n";

    resultParallel.save("resultParallel.bmp");
    result.save("result.bmp");
    
    return 0;
}
