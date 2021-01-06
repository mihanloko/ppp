#include <iostream>
#include <vector>
#include <cmath> 
#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>
#include <locale.h>
#include "CImg-2.9.3_pre101320/CImg.h"
#include "mpi.h"

#define R 0
#define G 1
#define B 2
#define KERNEL_WIDTH 3
#define KERNEL_HEIGHT 3

using namespace cimg_library;
using namespace std;

CImg<unsigned char> image = CImg<>("small.bmp").normalize(0, 255);
CImg<unsigned char> result = CImg<>(image);
double kernel[KERNEL_HEIGHT][KERNEL_WIDTH] = {
        {0.75, 0.75, 0.75},
        {0.75, 0.75, 0.75},
        {0.75, 0.75, 0.75}
};


void proccessBlock(int x_start, int x_finish) {
    int kernelWidth = KERNEL_WIDTH;
    int kernelHeight = KERNEL_HEIGHT;
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
void main(int argc, char* argv[])
{
    setlocale(LC_ALL, "Russian");

    int height = image.height(), width = image.width();
    
    MPI_Status status;

    int threadsCount; // число процессов
    int currentThread; // номер процесса

    int flag;
    int* model;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &threadsCount);
    MPI_Comm_rank(MPI_COMM_WORLD, &currentThread);

    // нулевой поток
    if (currentThread == 0) {

        auto begin = chrono::high_resolution_clock::now();
        
        int step = (width / threadsCount) + 1;
        int left = 0 + step;
        int right = min(left + step, width);
        byte* r = new byte[100];
        // отправка на потоки
        

        for (int i = 1; i < threadsCount; ++i) {

            MPI_Send(&height, 1, MPI_INT, i, 50, MPI_COMM_WORLD);
            MPI_Send(&width, 1, MPI_INT, i, 51, MPI_COMM_WORLD);
            MPI_Send(&left, 1, MPI_INT, i, 52, MPI_COMM_WORLD);
            MPI_Send(&right, 1, MPI_INT, i, 53, MPI_COMM_WORLD);

            
            left += step;
            right = min(left + step, width);
        }
        // работа со своим блоком
        proccessBlock(0, min(step, width));

        for (int i = 1; i < threadsCount; ++i) {

            bool res;
            MPI_Recv(&res, 1, MPI_C_BOOL, i, 100, MPI_COMM_WORLD, &status);

        }

        result.save("result.bmp");

        auto elapsedTime = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - begin).count();
        cout << "time " << elapsedTime << " ms\n";
    }
    else {

        int left, right;
        // принимаем данные из потока 0
        MPI_Recv(&height, 1, MPI_INT, 0, 50, MPI_COMM_WORLD, &status);
        MPI_Recv(&width, 1, MPI_INT, 0, 51, MPI_COMM_WORLD, &status);
        MPI_Recv(&left, 1, MPI_INT, 0, 52, MPI_COMM_WORLD, &status);
        MPI_Recv(&right, 1, MPI_INT, 0, 53, MPI_COMM_WORLD, &status);

        
        proccessBlock(left, right);


        bool res = true;
        // посылаем локальные данные в нулевой поток
        MPI_Send(&res, 1, MPI_C_BOOL, 0, 100, MPI_COMM_WORLD);

    }



    MPI_Finalize();
}
