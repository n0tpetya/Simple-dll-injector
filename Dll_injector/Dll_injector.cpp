#include <Windows.h>
#include <iostream>

BOOL InjectDLL(DWORD processId, LPCWSTR dllPath)
{

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL)
    {
        std::cout << "Failed to open process. Error code: " << GetLastError() << std::endl;
        return FALSE;
    }

    LPVOID dllPathAddress = VirtualAllocEx(hProcess, NULL, (wcslen(dllPath) + 1) * sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE);
    if (dllPathAddress == NULL)
    {
        std::cout << "Failed to allocate memory in the target process. Error code: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return FALSE;
    }

    if (!WriteProcessMemory(hProcess, dllPathAddress, dllPath, (wcslen(dllPath) + 1) * sizeof(WCHAR), NULL))
    {
        std::cout << "Failed to write DLL path to target process memory. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    // find adress of LoadLibraryW in kernel32.dll
    LPTHREAD_START_ROUTINE loadLibraryAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
    if (loadLibraryAddr == NULL)
    {
        std::cout << "Failed to get address of LoadLibraryW. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, loadLibraryAddr, dllPathAddress, 0, NULL);
    if (hThread == NULL)
    {
        std::cout << "Failed to create remote thread in target process. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    // Warten auf Beendigung Threads
    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return TRUE;
}

int main()
{
    DWORD processId;
    std::cout << "Enter the process ID: ";
    std::cin >> processId;

    LPCWSTR dllPath = L""; // your DLL path

    if (InjectDLL(processId, dllPath))
    {
        std::cout << "DLL injected successfully!" << std::endl;
    }
    else
    {
        std::cout << "DLL injection failed." << std::endl;
    }

    std::cout << "Press any key to exit...";
    std::cin.get();
    return 0;
}
