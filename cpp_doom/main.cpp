#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <CommCtrl.h>

#pragma comment(lib, "comctl32.lib")

// Control IDs
#define ID_BTN_SAVE 1001
#define ID_BTN_NUKE 1002
#define ID_EDIT_ACCOUNT 2001
#define ID_EDIT_ACCESS_KEY 2002
#define ID_EDIT_SECRET_KEY 2003

// Region Checkbox IDs
#define ID_CHK_REGION_START 3000

struct Region {
    std::wstring name;
    bool selected;
    HWND hwnd;
};

std::vector<Region> g_regions = {
    {L"us-east-1", true}, {L"us-east-2", false}, {L"us-west-1", false}, {L"us-west-2", false},
    {L"ap-south-1", false}, {L"ap-northeast-1", false}, {L"ap-northeast-2", true}, {L"ap-northeast-3", false},
    {L"ap-southeast-1", false}, {L"ap-southeast-2", false}, {L"ca-central-1", false}, {L"eu-central-1", false},
    {L"eu-west-1", false}, {L"eu-west-2", false}, {L"eu-west-3", false}, {L"eu-north-1", false},
    {L"sa-east-1", false}, {L"global", true}
};

HWND g_hAccount, g_hAccessKey, g_hSecretKey, g_hLogs;

void SaveFiles(HWND hwnd) {
    wchar_t account[256], access[256], secret[256];
    GetWindowText(g_hAccount, account, 256);
    GetWindowText(g_hAccessKey, access, 256);
    GetWindowText(g_hSecretKey, secret, 256);

    // Save credentials.json
    std::wofstream cred_file(L"external/credentials.json");
    cred_file << L"{\n";
    cred_file << L"  \"aws_access_key_id\": \"" << access << L"\",\n";
    cred_file << L"  \"aws_secret_access_key\": \"" << secret << L"\"\n";
    cred_file << L"}\n";
    cred_file.close();

    // Save config.yaml
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

    MessageBox(hwnd, L"Settings saved to external/ folder.", L"Success", MB_OK | MB_ICONINFORMATION);
}

void RunNuke(HWND hwnd) {
    int cleanup = MessageBox(hwnd, L"Are you sure you want to PERMANENTLY DELETE all resources in the selected regions?", L"WARNING", MB_YESNO | MB_ICONWARNING);
    if (cleanup != IDYES) return;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::wstring cmd = L"external/aws-nuke.exe --config external/config.yaml --no-dry-run --force";
    if (CreateProcess(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        MessageBox(hwnd, L"aws-nuke process started.", L"Info", MB_OK | MB_ICONINFORMATION);
    } else {
        MessageBox(hwnd, L"Failed to launch external/aws-nuke.exe. Ensure it exists.", L"Error", MB_OK | MB_ICONERROR);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        HFONT hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        CreateWindow(L"STATIC", L"⚙ SETTINGS", WS_VISIBLE | WS_CHILD, 20, 20, 200, 25, hwnd, NULL, NULL, NULL);

        CreateWindow(L"STATIC", L"Account-ID :", WS_VISIBLE | WS_CHILD, 20, 60, 150, 20, hwnd, NULL, NULL, NULL);
        g_hAccount = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 58, 200, 22, hwnd, (HMENU)ID_EDIT_ACCOUNT, NULL, NULL);

        CreateWindow(L"STATIC", L"AWS-ACCESS-KEY-ID :", WS_VISIBLE | WS_CHILD, 20, 90, 150, 20, hwnd, NULL, NULL, NULL);
        g_hAccessKey = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, 88, 200, 22, hwnd, (HMENU)ID_EDIT_ACCESS_KEY, NULL, NULL);

        CreateWindow(L"STATIC", L"AWS-SECRET-ACCESS-KEY :", WS_VISIBLE | WS_CHILD, 20, 120, 180, 20, hwnd, NULL, NULL, NULL);
        g_hSecretKey = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 200, 118, 300, 22, hwnd, (HMENU)ID_EDIT_SECRET_KEY, NULL, NULL);

        // Region Group Box
        CreateWindow(L"BUTTON", L"리소스를 삭제할 리전을 선택해주세요", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 15, 150, 750, 380, hwnd, NULL, NULL, NULL);

        for (int i = 0; i < g_regions.size(); ++i) {
            int x = 30;
            int y = 180 + (i * 18);
            g_regions[i].hwnd = CreateWindow(L"BUTTON", g_regions[i].name.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, x, y, 200, 16, hwnd, (HMENU)(ID_CHK_REGION_START + i), NULL, NULL);
            if (g_regions[i].selected) SendMessage(g_regions[i].hwnd, BM_SETCHECK, BST_CHECKED, 0);
        }

        CreateWindow(L"BUTTON", L"SAVE", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 540, 80, 30, hwnd, (HMENU)ID_BTN_SAVE, NULL, NULL);

        CreateWindow(L"STATIC", L"● BOMB", WS_VISIBLE | WS_CHILD, 20, 590, 200, 25, hwnd, NULL, NULL, NULL);
        CreateWindow(L"BUTTON", L"DELETE YOUR RESOURCES", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 620, 250, 35, hwnd, (HMENU)ID_BTN_NUKE, NULL, NULL);

        CreateWindow(L"STATIC", L"📜 LOGS", WS_VISIBLE | WS_CHILD, 20, 670, 200, 25, hwnd, NULL, NULL, NULL);

        // Apply font to all children
        EnumChildWindows(hwnd, [](HWND child, LPARAM font) -> BOOL {
            SendMessage(child, WM_SETFONT, font, TRUE);
            return TRUE;
        }, (LPARAM)hFont);

        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == ID_BTN_SAVE) SaveFiles(hwnd);
        if (LOWORD(wParam) == ID_BTN_NUKE) RunNuke(hwnd);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    const wchar_t CLASS_NAME[] = L"AWSBombWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"aws-bomb", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 800, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nShowCmd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
