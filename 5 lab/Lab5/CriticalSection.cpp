#include <windows.h>
#include <string>
#include <iostream>
#include "tchar.h"
#include <fstream>

CRITICAL_SECTION FileLockingCriticalSection;

int ReadFromFile() {
    std::fstream myfile("balance.txt", std::ios_base::in);
    int result;
    myfile >> result;
    myfile.close();
    return result;
}

void WriteToFile(int data) {
    std::fstream myfile("balance.txt", std::ios_base::out);
    myfile << data << std::endl;
    myfile.close();
}

int GetBalance() {
    EnterCriticalSection(&FileLockingCriticalSection);
    int balance = ReadFromFile();
    LeaveCriticalSection(&FileLockingCriticalSection);
    return balance;
}

void Deposit(int money) {
    EnterCriticalSection(&FileLockingCriticalSection);

    int balance = ReadFromFile();
    balance += money;
    WriteToFile(balance);
    printf("Balance after deposit: %d\n", balance);

    LeaveCriticalSection(&FileLockingCriticalSection);
}

void Withdraw(int money) {
    EnterCriticalSection(&FileLockingCriticalSection);

    int balance = ReadFromFile();
    if (balance < money) {
        printf("Cannot withdraw money, balance lower than %d\n", money);
    }
    else {
        Sleep(20);
        balance -= money;
        WriteToFile(balance);
        printf("Balance after withdraw: %d\n", balance);
    }

    LeaveCriticalSection(&FileLockingCriticalSection);
}

DWORD WINAPI DoDeposit(CONST LPVOID lpParameter) {
    Deposit((int)(INT_PTR)lpParameter);
    return 0;
}

DWORD WINAPI DoWithdraw(CONST LPVOID lpParameter) {
    Withdraw((int)(INT_PTR)lpParameter);
    return 0;
}

int _tmain(int argc, _TCHAR* argv[]) {
    HANDLE* handles = new HANDLE[50];

    InitializeCriticalSection(&FileLockingCriticalSection);

    WriteToFile(0);

    SetProcessAffinityMask(GetCurrentProcess(), 1);
    for (int i = 0; i < 50; i++) {
        handles[i] = (i % 2 == 0)
            ? CreateThread(NULL, 0, &DoDeposit, (LPVOID)(INT_PTR)230, CREATE_SUSPENDED, NULL)
            : CreateThread(NULL, 0, &DoWithdraw, (LPVOID)(INT_PTR)1000, CREATE_SUSPENDED, NULL);
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(50, handles, TRUE, INFINITE);

    int finalBalance = GetBalance();
    printf("Final Balance: %d\n", finalBalance);

    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();

    DeleteCriticalSection(&FileLockingCriticalSection);
    delete[] handles;

    return 0;
}