#include <windows.h>
#include <tchar.h>
#include <commdlg.h>

const int ID_BUTTON_INJECT = 1;
const int ID_BUTTON_BROWSE = 2;
const int ID_TEXT_PID = 3;
const int ID_TEXT_PATH = 4;

BOOL InjectDLL(DWORD pid, const TCHAR* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        return FALSE;
    }

    void* pRemoteDllPath = VirtualAllocEx(hProcess, NULL, _tcslen(dllPath) * sizeof(TCHAR) + 1, MEM_COMMIT, PAGE_READWRITE);
    WriteProcessMemory(hProcess, pRemoteDllPath, dllPath, _tcslen(dllPath) * sizeof(TCHAR) + 1, NULL);

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "LoadLibraryW"), pRemoteDllPath, 0, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        VirtualFreeEx(hProcess, pRemoteDllPath, 0, MEM_RELEASE);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return TRUE;
    }
    return FALSE;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BUTTON_INJECT) {
            TCHAR pidBuffer[10];
            TCHAR pathBuffer[MAX_PATH];
            GetWindowText(GetDlgItem(hwnd, ID_TEXT_PID), pidBuffer, 10);
            GetWindowText(GetDlgItem(hwnd, ID_TEXT_PATH), pathBuffer, MAX_PATH);

            DWORD pid = _ttoi(pidBuffer);
            if (InjectDLL(pid, pathBuffer)) {
                MessageBox(hwnd, _T("DLL Injected Successfully!"), _T("Success"), MB_OK);
            }
            else {
                MessageBox(hwnd, _T("DLL Injection Failed!"), _T("Error"), MB_OK);
            }
        }
        else if (LOWORD(wParam) == ID_BUTTON_BROWSE) {
            OPENFILENAME ofn;
            TCHAR szFile[MAX_PATH] = _T("");

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = _T("DLL Files\0*.DLL\0All\0*.*\0");
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {
                SetWindowText(GetDlgItem(hwnd, ID_TEXT_PATH), ofn.lpstrFile);
            }
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_VREDRAW | CS_HREDRAW, WindowProc, 0, 0, hInstance, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, _T("DLLInjectorClass"), NULL };
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(0, _T("DLLInjectorClass"), _T("DLL Injector"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL, hInstance, NULL);

    CreateWindowEx(0, _T("STATIC"), _T("PID:"), WS_CHILD | WS_VISIBLE, 10, 10, 40, 20, hwnd, NULL, hInstance, NULL);
    CreateWindowEx(0, _T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE, 50, 10, 220, 20, hwnd, (HMENU)ID_TEXT_PID, hInstance, NULL);

    CreateWindowEx(0, _T("STATIC"), _T("DLL Path:"), WS_CHILD | WS_VISIBLE, 10, 40, 70, 20, hwnd, NULL, hInstance, NULL);
    CreateWindowEx(0, _T("EDIT"), _T(""), WS_CHILD | WS_VISIBLE, 80, 40, 130, 20, hwnd, (HMENU)ID_TEXT_PATH, hInstance, NULL);
    CreateWindowEx(0, _T("BUTTON"), _T("Browse"), WS_CHILD | WS_VISIBLE, 220, 40, 60, 25, hwnd, (HMENU)ID_BUTTON_BROWSE, hInstance, NULL);

    CreateWindowEx(0, _T("BUTTON"), _T("Inject"), WS_CHILD | WS_VISIBLE, 100, 70, 90, 30, hwnd, (HMENU)ID_BUTTON_INJECT, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
