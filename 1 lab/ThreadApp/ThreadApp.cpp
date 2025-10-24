#include "stdafx.h"
#include <windows.h>
#include <string>
#include <iostream>

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
    unsigned int threadNumber = *(unsigned int*)lpParam;

    std::cout << "Поток #" << threadNumber << " выполняет свою работу" << std::endl;

    ExitThread(0);
}

int main(int argc, char* argv[])
{
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    if (argc != 2)
    {
        std::cout << "Использование: " << argv[0] << " <количество потоков>" << std::endl;

        return 1;
    }

    int N = std::stoi(argv[1]);

    if (N <= 0)
    {
        std::cout << "Количество потоков должно быть положительным числом" << std::endl;

        return 1;
    }

    HANDLE* handles = new HANDLE[N];
    unsigned int* threadNumbers = new unsigned int[N]; 

    for (int i = 0; i < N; i++)
    {
        threadNumbers[i] = i + 1; 
        handles[i] = CreateThread(NULL, 0, &ThreadProc, &threadNumbers[i], CREATE_SUSPENDED, NULL);

        if (handles[i] == NULL)
        {
            std::cout << "Ошибка при создании потока " << (i + 1) << std::endl;

            return 1;
        }
    }

    for (int i = 0; i < N; i++)
    {
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(N, handles, TRUE, INFINITE);

    for (int i = 0; i < N; i++)
    {
        CloseHandle(handles[i]);
    }

    delete[] handles;
    delete[] threadNumbers;

    std::cout << "Все потоки завершили работу. Программа завершается." << std::endl;

    return 0;
}