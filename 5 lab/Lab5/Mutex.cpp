//#include <windows.h>
//#include <string>
//#include <iostream>
//#include "tchar.h"
//#include <fstream>
//
//HANDLE FileMutex;
//
//int ReadFromFile() {
//    std::fstream myfile("balance.txt", std::ios_base::in);
//    int result;
//    myfile >> result;
//    myfile.close();
//    return result;
//}
//
//void WriteToFile(int data) {
//    std::fstream myfile("balance.txt", std::ios_base::out);
//    myfile << data << std::endl;
//    myfile.close();
//}
//
//int GetBalance() {
//    WaitForSingleObject(FileMutex, INFINITE);
//    int balance = ReadFromFile();
//    ReleaseMutex(FileMutex);
//    return balance;
//}
//
//void Deposit(int money) {
//    WaitForSingleObject(FileMutex, INFINITE);
//
//    int balance = ReadFromFile();
//    balance += money;
//    WriteToFile(balance);
//    printf("Balance after deposit: %d\n", balance);
//
//    ReleaseMutex(FileMutex);
//}
//
//void Withdraw(int money) {
//    WaitForSingleObject(FileMutex, INFINITE);
//
//    int balance = ReadFromFile();
//    if (balance < money) {
//        printf("Cannot withdraw money, balance lower than %d\n", money);
//    }
//    else {
//        Sleep(20);
//        balance -= money;
//        WriteToFile(balance);
//        printf("Balance after withdraw: %d\n", balance);
//    }
//
//    ReleaseMutex(FileMutex);
//}
//
//DWORD WINAPI DoDeposit(CONST LPVOID lpParameter) {
//    Deposit((int)(INT_PTR)lpParameter);
//    return 0;
//}
//
//DWORD WINAPI DoWithdraw(CONST LPVOID lpParameter) {
//    Withdraw((int)(INT_PTR)lpParameter);
//    return 0;
//}
//
//int _tmain(int argc, _TCHAR* argv[]) {
//    std::cout << "Process started! Instance: " << (argc == 1 ? "First" : "Second") << std::endl;
//
//    HANDLE* handles = new HANDLE[50];
//
//    FileMutex = CreateMutex(NULL, FALSE, _T("Global\\BalanceFileMutex"));
//
//    if (FileMutex == NULL) {
//        std::cout << "CreateMutex error: " << GetLastError() << std::endl;
//        return 1;
//    }
//
//    WaitForSingleObject(FileMutex, INFINITE);
//    std::fstream testFile("balance.txt", std::ios_base::in);
//    if (!testFile.is_open()) {
//        WriteToFile(0);
//    }
//    testFile.close();
//    ReleaseMutex(FileMutex);
//
//    SetProcessAffinityMask(GetCurrentProcess(), 1);
//    for (int i = 0; i < 50; i++) {
//        handles[i] = (i % 2 == 0)
//            ? CreateThread(NULL, 0, &DoDeposit, (LPVOID)(INT_PTR)230, CREATE_SUSPENDED, NULL)
//            : CreateThread(NULL, 0, &DoWithdraw, (LPVOID)(INT_PTR)1000, CREATE_SUSPENDED, NULL);
//        ResumeThread(handles[i]);
//    }
//
//    WaitForMultipleObjects(50, handles, TRUE, INFINITE);
//
//    int finalBalance = GetBalance();
//    printf("Final Balance: %d\n", finalBalance);
//
//    if (finalBalance < 0) {
//        printf("ERROR: Negative balance detected! Synchronization failed!\n");
//    }
//    else {
//        printf("SUCCESS: Balance is correct!\n");
//    }
//
//    std::cout << "Press Enter to exit..." << std::endl;
//    std::cin.get();
//
//    for (int i = 0; i < 50; i++) {
//        CloseHandle(handles[i]);
//    }
//    CloseHandle(FileMutex);
//    delete[] handles;
//
//    return 0;
//}