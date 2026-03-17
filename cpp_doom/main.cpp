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
#include <imm.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "imm32.lib")

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
#define ID_CHK_SHOW_SECRET 4004
#define ID_INDICATOR_IME 5001
#define ID_INDICATOR_CAPS 5002

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

HWND g_hAccount, g_hAccessKey, g_hSecretKey, g_hLogs, g_hBtnSave, g_hBtnNuke, g_hChkShowSecret, g_hImeIndicator, g_hCapsIndicator;
HWND g_hChkSafe[3];
HBITMAP g_hNukeBmpFull = NULL, g_hNukeBmpDim = NULL;
HANDLE g_hNukeStdinWrite = NULL;
WNDPROC g_OldEditProc = NULL;
std::vector<unsigned char> g_binaryPayload;
bool g_isWorking = false;
HFONT g_hFontBold = NULL, g_hFontPrefix = NULL, g_hFontIndicator = NULL;
HBRUSH g_hBrushNavy = NULL, g_hBrushRed = NULL, g_hBrushPureRed = NULL;
int g_protectedTerminalLength = 0;

void SaveFiles(HWND hwnd);
void AppendLog(const std::wstring& text);
void UpdateNukeButtonState();

void LoadMTAConfig() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::wstring mtaPath = exePath;
    size_t dotPos = mtaPath.find_last_of(L".");
    if (dotPos != std::wstring::npos) mtaPath = mtaPath.substr(0, dotPos);
    mtaPath += L"_mta.json";

    std::wifstream f(mtaPath);
    if (!f.is_open()) return;

    std::wstring line;
    while (std::getline(f, line)) {
        size_t idPos = line.find(L"\"account_id\": \"");
        if (idPos != std::wstring::npos) {
            std::wstring val = line.substr(idPos + 15);
            val = val.substr(0, val.find(L"\""));
            SetWindowText(g_hAccount, val.c_str());
        }
        size_t accPos = line.find(L"\"access_key\": \"");
        if (accPos != std::wstring::npos) {
            std::wstring val = line.substr(accPos + 15);
            val = val.substr(0, val.find(L"\""));
            SetWindowText(g_hAccessKey, val.c_str());
        }
        size_t secPos = line.find(L"\"secret_key\": \"");
        if (secPos != std::wstring::npos) {
            std::wstring val = line.substr(secPos + 15);
            val = val.substr(0, val.find(L"\""));
            SetWindowText(g_hSecretKey, val.c_str());
        }
        size_t showPos = line.find(L"\"show_secret\": ");
        if (showPos != std::wstring::npos) {
            std::wstring val = line.substr(showPos + 15);
            if (val.find(L"true") != std::wstring::npos) {
                SendMessage(g_hChkShowSecret, BM_SETCHECK, BST_CHECKED, 0);
                PostMessage(g_hSecretKey, EM_SETPASSWORDCHAR, 0, 0);
            } else {
                SendMessage(g_hChkShowSecret, BM_SETCHECK, BST_UNCHECKED, 0);
                PostMessage(g_hSecretKey, EM_SETPASSWORDCHAR, (WPARAM)L'*', 0);
            }
            InvalidateRect(g_hSecretKey, NULL, TRUE);
        }
    }
}

void ShowPrompt() {
    int len = GetWindowTextLength(g_hLogs);
    SendMessage(g_hLogs, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessage(g_hLogs, EM_REPLACESEL, 0, (LPARAM)L">_ ");
    g_protectedTerminalLength = GetWindowTextLength(g_hLogs);
}

LRESULT CALLBACK TerminalEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        DWORD start, end;
        SendMessage(hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

        if (wParam == VK_RETURN) {
            int len = GetWindowTextLength(hwnd);
            std::vector<wchar_t> buf(len + 1);
            GetWindowText(hwnd, buf.data(), len + 1);
            std::wstring text(buf.data());
            
            std::wstring command;
            if (g_protectedTerminalLength < (int)text.length()) {
                command = text.substr(g_protectedTerminalLength);
            }
            
            if (!command.empty() && g_hNukeStdinWrite) {
                int utf8Len = WideCharToMultiByte(CP_UTF8, 0, command.c_str(), -1, NULL, 0, NULL, NULL);
                std::vector<char> utf8Buf(utf8Len);
                WideCharToMultiByte(CP_UTF8, 0, command.c_str(), -1, utf8Buf.data(), utf8Len, NULL, NULL);
                DWORD written;
                WriteFile(g_hNukeStdinWrite, utf8Buf.data(), utf8Len - 1, &written, NULL);
            }
            // Move to next line and history the current prompt
            int finalLen = GetWindowTextLength(hwnd);
            SendMessage(hwnd, EM_SETSEL, finalLen, finalLen);
            SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
            g_protectedTerminalLength = GetWindowTextLength(hwnd);
            
            ShowPrompt();
            return 0;
        }
        if (wParam == VK_BACK) {
            if ((int)start <= g_protectedTerminalLength) return 0;
        }
        if (wParam == VK_DELETE) {
            if ((int)start < g_protectedTerminalLength) return 0;
        }
        // Prevent moving cursor into protected area
        if (wParam == VK_LEFT || wParam == VK_UP || wParam == VK_HOME) {
            if ((int)start <= g_protectedTerminalLength) return 0;
        }
    }
    if (uMsg == WM_LBUTTONDOWN) {
        LRESULT res = CallWindowProc(g_OldEditProc, hwnd, uMsg, wParam, lParam);
        DWORD start, end;
        SendMessage(hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        if ((int)start < g_protectedTerminalLength) {
            SendMessage(hwnd, EM_SETSEL, (WPARAM)g_protectedTerminalLength, (LPARAM)g_protectedTerminalLength);
        }
        return res;
    }
    if (uMsg == WM_CHAR) {
        DWORD start, end;
        SendMessage(hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        if ((int)start < g_protectedTerminalLength && wParam != 3) { // Allow Ctrl+C (3)
            return 0;
        }
    }
    if (uMsg == WM_PASTE || uMsg == WM_CUT || uMsg == WM_CLEAR) {
        DWORD start, end;
        SendMessage(hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        if ((int)start < g_protectedTerminalLength) return 0;
    }
    return CallWindowProc(g_OldEditProc, hwnd, uMsg, wParam, lParam);
}

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
    if (text.empty()) return;

    std::wstring finalMsg = text;
    // Standardize tagging for MTA's own logs
    if (finalMsg != L"\r\n" && finalMsg != L"\n") {
        size_t first = finalMsg.find_first_not_of(L" \t\r\n");
        if (first != std::wstring::npos && finalMsg[first] != L'[') {
            finalMsg = L"[MTA] " + finalMsg;
        }
    }

    int len = GetWindowTextLength(g_hLogs);
    std::vector<wchar_t> buf(len + 1);
    GetWindowText(g_hLogs, buf.data(), len + 1);
    std::wstring current(buf.data());

    // System logs should be inserted BEFORE the current active prompt/input line
    int promptLineStart = g_protectedTerminalLength - 3; // Length of ">_ " is 3
    if (promptLineStart < 0) promptLineStart = 0;

    // Verify if prompt is actually there at the end
    bool hasPromptAtEnd = (current.length() >= 3 && current.substr(current.length() - 3) == L">_ ");
    
    int insertPos = (hasPromptAtEnd) ? promptLineStart : len;

    SendMessage(g_hLogs, EM_SETSEL, (WPARAM)insertPos, (LPARAM)insertPos);
    SendMessage(g_hLogs, EM_REPLACESEL, 0, (LPARAM)finalMsg.c_str());
    
    g_protectedTerminalLength += (int)finalMsg.length();

    // Scroll to end and move cursor back to end of input if needed
    int finalLen = GetWindowTextLength(g_hLogs);
    SendMessage(g_hLogs, EM_SETSEL, finalLen, finalLen);
    SendMessage(g_hLogs, EM_SCROLLCARET, 0, 0);
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
    HANDLE hStdinRead, hStdinWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    
    // Stdout/Stderr pipe
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return false;
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    // Stdin pipe
    if (!CreatePipe(&hStdinRead, &hStdinWrite, &sa, 0)) {
        CloseHandle(hReadPipe); CloseHandle(hWritePipe);
        return false;
    }
    SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);
    g_hNukeStdinWrite = hStdinWrite;

    STARTUPINFO si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = hStdinRead;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    std::wstring host = L"C:\\Windows\\System32\\svchost.exe";
    std::wstring cmdLine = L"\"" + host + L"\" " + args;

    if (!CreateProcess(NULL, (LPWSTR)cmdLine.c_str(), NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe); CloseHandle(hWritePipe);
        CloseHandle(hStdinRead); CloseHandle(hStdinWrite);
        return false;
    }

    CloseHandle(hWritePipe);
    CloseHandle(hStdinRead);
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
    CreateDirectory(L"external", NULL);
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
    for (int i = 0; i < (int)g_regions.size(); ++i) {
        if (SendMessage(g_regions[i].hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            config_file << L"- \"" << g_regions[i].name << L"\"\n";
        }
    }
    config_file << L"account-blocklist:\n";
    config_file << L"- \"999999999999\"\n";
    config_file << L"accounts:\n";
    config_file << L"  \"" << account << L"\": {}\n";
    config_file.close();

    // Save MTA config ([EXE_NAME]_mta.json)
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::wstring mtaPath = exePath;
    size_t dotPos = mtaPath.find_last_of(L".");
    if (dotPos != std::wstring::npos) mtaPath = mtaPath.substr(0, dotPos);
    mtaPath += L"_mta.json";
    
    std::wofstream mta_file(mtaPath);
    mta_file << L"{\n";
    mta_file << L"  \"account_id\": \"" << account << L"\",\n";
    mta_file << L"  \"access_key\": \"" << access << L"\",\n";
    mta_file << L"  \"secret_key\": \"" << secret << L"\",\n";
    mta_file << L"  \"show_secret\": " << (SendMessage(g_hChkShowSecret, BM_GETCHECK, 0, 0) == BST_CHECKED ? L"true" : L"false") << L"\n";
    mta_file << L"}\n";
    mta_file.close();
}

DWORD WINAPI SaveFilesAsync(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;
    g_isWorking = true;
    SaveFiles(hwnd);
    g_isWorking = false;
    PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, HTCLIENT); // Refresh cursor
    MessageBox(hwnd, L"Settings saved to external/ and _mta.json.", L"Success", MB_OK | MB_ICONINFORMATION);
    return 0;
}

DWORD WINAPI RunNukeAsync(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;
    g_isWorking = true;

    if (g_binaryPayload.empty()) {
        AppendLog(L"[INFO] aws-nuke.exe .dat 조각들로부터 최초 로딩 중...\r\n");
        g_binaryPayload = LoadAndDecryptBinary();
    }

    if (g_binaryPayload.empty()) {
        AppendLog(L"[ERROR] data 조각들을 찾을 수 없거나 불러오기에 실패했습니다.\r\n");
        g_isWorking = false;
        return 1;
    }

    // Prepare credentials for command line flags
    wchar_t access[256], secret[256];
    GetWindowText(g_hAccessKey, access, 256);
    GetWindowText(g_hSecretKey, secret, 256);

    if (wcslen(access) == 0 || wcslen(secret) == 0) {
        AppendLog(L"[INFO] AWS Access/Secret Key가 입력되지 않았습니다.\r\n");
        AppendLog(L"[INFO] 상단에 키를 입력하고 'SAVE' 버튼을 눌러 설정을 저장한 뒤 실행해 주세요.\r\n");
        g_isWorking = false;
        PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, HTCLIENT);
        return 0;
    }

    std::wstring nukeArgs = L"--config external/config.yaml --force";
    nukeArgs += L" --access-key-id \"";
    nukeArgs += access;
    nukeArgs += L"\" --secret-access-key \"";
    nukeArgs += secret;
    nukeArgs += L"\"";

    AppendLog(L"[INFO] 인메모리 프로세스 주입 및 실행 시도...\r\n");
    SaveFiles(hwnd); // Ensure latest config files exist
    
    if (ProcessHollow(g_binaryPayload, nukeArgs)) {
        AppendLog(L"[INFO] 프로세스 주입 성공. aws-nuke 엔스턴스가 가동되었습니다.\r\n");
    } else {
        AppendLog(L"[ERROR] 프로세스 주입(Process Hollowing)에 실패했습니다.\r\n");
    }
    
    g_isWorking = false;
    PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, HTCLIENT);
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
        int mtaW = 180; // Slightly smaller as requested
        HBITMAP hMtaBmp = LoadPNGFromResource(IDB_MTA_PNG, mtaW, mtaH);
        HWND hMta = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_BITMAP, 20, 15, mtaW, mtaH, hwnd, NULL, NULL, NULL);
        if (hMtaBmp) SendMessage(hMta, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hMtaBmp);

        int startY = 75; // Adjusted spacing for smaller header
        CreateWindow(L"STATIC", L"Account-ID :", WS_VISIBLE | WS_CHILD, 20, startY, 150, 20, hwnd, NULL, NULL, NULL);
        g_hAccount = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, startY - 2, 200, 22, hwnd, (HMENU)ID_EDIT_ACCOUNT, NULL, NULL);

        CreateWindow(L"STATIC", L"AWS-ACCESS-KEY-ID :", WS_VISIBLE | WS_CHILD, 20, startY + 30, 150, 20, hwnd, NULL, NULL, NULL);
        g_hAccessKey = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, startY + 28, 200, 22, hwnd, (HMENU)ID_EDIT_ACCESS_KEY, NULL, NULL);

        CreateWindow(L"STATIC", L"AWS-SECRET-ACCESS-KEY :", WS_VISIBLE | WS_CHILD, 20, startY + 60, 180, 20, hwnd, NULL, NULL, NULL);
        g_hSecretKey = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 200, startY + 58, 300, 22, hwnd, (HMENU)ID_EDIT_SECRET_KEY, NULL, NULL);
        SendMessage(g_hSecretKey, EM_SETPASSWORDCHAR, (WPARAM)L'●', 0);
        
        g_hChkShowSecret = CreateWindow(L"BUTTON", L"Show", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 510, startY + 58, 60, 22, hwnd, (HMENU)ID_CHK_SHOW_SECRET, NULL, NULL);

        g_hFontBold = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_hFontPrefix = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_hFontIndicator = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        g_hBrushNavy = CreateSolidBrush(RGB(0, 0, 128));
        g_hBrushRed = CreateSolidBrush(RGB(139, 0, 0));
        g_hBrushPureRed = CreateSolidBrush(RGB(255, 0, 0));

        // Region selection group box
        int groupY = startY + 95; 
        int groupH = 200; // Increased to prevent overlapping
        CreateWindow(L"BUTTON", L"리소스를 삭제할 리전을 선택해주세요", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 15, groupY, 560, groupH, hwnd, NULL, NULL, NULL);

        int columns = 3;
        int itemsPerColumn = (int)((g_regions.size() + columns - 1) / columns);
        for (int i = 0; i < (int)g_regions.size(); ++i) {
            int col = i / itemsPerColumn;
            int row = i % itemsPerColumn;
            int x = 30 + (col * 170); // Narrower horizontal spacing
            int y = groupY + 30 + (row * 24); // Tighter vertical padding inside group
            // Use BS_OWNERDRAW for custom rich-text rendering
            g_regions[i].hwnd = CreateWindow(L"BUTTON", g_regions[i].name.c_str(), WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, x, y, 150, 24, hwnd, (HMENU)(ID_CHK_REGION_START + i), NULL, NULL);
        }

        // --- Optimized Section: SAVE, BOMB, NUKE ---
        int saveY = groupY + groupH + 15; 
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
        int logsLblY = nukeY + nukeH + 5; // Moved up closer
        CreateWindow(L"STATIC", L"🖥️ TERMINAL", WS_VISIBLE | WS_CHILD, 20, logsLblY, 110, 25, hwnd, NULL, NULL, NULL);

        g_hImeIndicator = CreateWindow(L"STATIC", L"[A]", WS_VISIBLE | WS_CHILD | SS_CENTER, 140, logsLblY, 40, 20, hwnd, (HMENU)ID_INDICATOR_IME, NULL, NULL);
        g_hCapsIndicator = CreateWindow(L"STATIC", L"[CAPS]", WS_VISIBLE | WS_CHILD | SS_CENTER, 190, logsLblY, 60, 20, hwnd, (HMENU)ID_INDICATOR_CAPS, NULL, NULL);
        SendMessage(g_hImeIndicator, WM_SETFONT, (WPARAM)g_hFontIndicator, TRUE);
        SendMessage(g_hCapsIndicator, WM_SETFONT, (WPARAM)g_hFontIndicator, TRUE);
        
        int logsEditY = logsLblY + 25;
        g_hLogs = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 20, logsEditY, 550, 280, hwnd, (HMENU)ID_EDIT_LOGS, NULL, NULL);
        g_OldEditProc = (WNDPROC)SetWindowLongPtr(g_hLogs, GWLP_WNDPROC, (LONG_PTR)TerminalEditProc);

        EnumChildWindows(hwnd, [](HWND child, LPARAM font) -> BOOL {
            SendMessage(child, WM_SETFONT, font, TRUE);
            return TRUE;
        }, (LPARAM)hFont);

        // Load previous settings and then auto-run
        LoadMTAConfig();
        RunNuke(hwnd);

        ShowPrompt();
        SetTimer(hwnd, 1, 100, NULL);

        return 0;
    }
    case WM_TIMER: {
        if (wParam == 1) {
            // IME check
            HIMC hImc = ImmGetContext(hwnd);
            DWORD dwConv, dwSent;
            bool isKorean = false;
            if (hImc) {
                if (ImmGetConversionStatus(hImc, &dwConv, &dwSent)) {
                    isKorean = (dwConv & IME_CMODE_NATIVE);
                }
                ImmReleaseContext(hwnd, hImc);
            }
            SetWindowText(g_hImeIndicator, isKorean ? L"[한]" : L"[A]");

            // CapsLock check
            bool capsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            ShowWindow(g_hCapsIndicator, capsOn ? SW_SHOW : SW_HIDE);

            InvalidateRect(g_hImeIndicator, NULL, TRUE);
            InvalidateRect(g_hCapsIndicator, NULL, TRUE);
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        if (hCtrl == g_hImeIndicator) {
            wchar_t text[8];
            GetWindowText(hCtrl, text, 8);
            bool isKorean = (wcscmp(text, L"[한]") == 0);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, OPAQUE);
            if (isKorean) {
                SetBkColor(hdc, RGB(139, 0, 0));
                return (LRESULT)g_hBrushRed;
            } else {
                SetBkColor(hdc, RGB(0, 0, 128));
                return (LRESULT)g_hBrushNavy;
            }
        }
        if (hCtrl == g_hCapsIndicator) {
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, RGB(255, 0, 0));
            return (LRESULT)g_hBrushPureRed;
        }
        break;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
        if (pdis->CtlID >= ID_CHK_REGION_START && pdis->CtlID < ID_CHK_REGION_START + (int)g_regions.size()) {
            int idx = pdis->CtlID - ID_CHK_REGION_START;
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;

            FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));

            // Draw Checkbox box
            RECT box = { rc.left, rc.top + 4, rc.left + 16, rc.top + 20 };
            DrawFrameControl(hdc, &box, DFC_BUTTON, DFCS_BUTTONCHECK | (g_regions[idx].selected ? DFCS_CHECKED : 0));

            // Draw Text with rich fonts
            std::wstring name = g_regions[idx].name;
            size_t dash = name.find(L"-");
            std::wstring prefix = (dash != std::wstring::npos) ? name.substr(0, dash) : name;
            std::wstring suffix = (dash != std::wstring::npos) ? name.substr(dash) : L"";

            SelectObject(hdc, g_hFontPrefix);
            RECT tr = { rc.left + 18, rc.top, rc.right, rc.bottom }; // Reduced gap from checkbox
            DrawText(hdc, prefix.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_CALCRECT);
            DrawText(hdc, prefix.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            if (!suffix.empty()) {
                SelectObject(hdc, g_hFontBold);
                tr.left += (tr.right - tr.left);
                tr.right = rc.right;
                DrawText(hdc, suffix.c_str(), -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }
            return TRUE;
        }
        break;
    }
    case WM_SETCURSOR:
        if (g_isWorking && LOWORD(lParam) == HTCLIENT) {
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            return TRUE;
        }
        break;
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == ID_BTN_SAVE && (code == BN_CLICKED || code == STN_CLICKED)) {
            CreateThread(NULL, 0, SaveFilesAsync, hwnd, 0, NULL);
        }
        if (id == ID_BTN_NUKE && (code == BN_CLICKED || code == STN_CLICKED)) {
            RunNuke(hwnd);
        }
        if (id >= ID_CHK_REGION_START && id < ID_CHK_REGION_START + (int)g_regions.size()) {
            int idx = id - ID_CHK_REGION_START;
            g_regions[idx].selected = !g_regions[idx].selected;
            InvalidateRect(g_regions[idx].hwnd, NULL, TRUE);
        }
        if (id == ID_CHK_SHOW_SECRET) {
            bool checked = (SendMessage(g_hChkShowSecret, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SendMessage(g_hSecretKey, EM_SETPASSWORDCHAR, (checked ? 0 : (WPARAM)L'●'), 0);
            SetFocus(g_hSecretKey); // Force update
            InvalidateRect(g_hSecretKey, NULL, TRUE);
        }
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

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"missile_to_AWS", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 610, 850, NULL, NULL, hInstance, NULL);
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
