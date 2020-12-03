#include <iostream>
#include <thread>
#include "CImg-2.9.3_pre101320/CImg.h"

#define R 0
#define G 1
#define B 2

using namespace cimg_library;
//using namespace std;

void test() {
    std::cout << "Hello world" << std::endl;
}

int main() {
    CImg<unsigned char> image = CImg<>("image.bmp").normalize(0, 255);
    CImg<unsigned char> result = CImg<>(image);

    std::thread t(test);


    std::cout << image.height() << " " << image.width() << std::endl;
    int height = image.height(), width = image.width();
/*    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            const unsigned int
                    red   = image(j,i,0),
                    green = image(j,i,1),
                    blue  = image(j,i,2);
            image(j, i, 0) = (i + j) % 256;
            image(j, i, 1) = i % 256;
            image(j, i, 2) = j % 256;
        }
    }*/

    double kernel[3][3] = {
            {0.75, 0.75, 0.75},
            {0.75, 0.75, 0.75},
            {0.75, 0.75, 0.75}
    };
    int kernelWidth = 2;
    int kernelHeight = 2;

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
            result(x, y, R) = (byte) rSum;
            result(x, y, G) = (byte) gSum;
            result(x, y, B) = (byte) bSum;
        }
    }

    result.save("result.bmp");

    return 0;
}
