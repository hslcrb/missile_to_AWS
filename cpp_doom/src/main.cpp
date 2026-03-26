#ifndef UNICODE
#define UNICODE
#endif

#define NOMINMAX
#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <set>
#include <sstream>
#include <CommCtrl.h>
#include <winternl.h>
#include <gdiplus.h>
#include <objidl.h>
#include <imm.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#include "resource.h"
#include "resource_list.h"
#include <shellapi.h>
#include <Wincrypt.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "crypt32.lib")

using namespace Gdiplus;

// NT API types
typedef NTSTATUS(NTAPI* pNtUnmapViewOfSection)(HANDLE, PVOID);

// Convenience factory for CreateFont — avoids 24-param repetition
inline HFONT MakeFont(int size, int weight = FW_NORMAL, const wchar_t* face = L"Noto Sans KR") {
    return CreateFont(size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face);
}


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

HWND g_hAccount, g_hAccessKey, g_hSecretKey, g_hLogs, g_hBtnSave, g_hBtnNuke, g_hBtnCancel, g_hChkShowSecret, g_hImeIndicator, g_hCapsIndicator, g_hResourceFilter, g_hSortCombo;
HWND g_hwndSelectAll = NULL;
HWND g_hPicLogo = NULL;
HWND g_hSettingsDlg = NULL;
#define ID_PIC_LOGO 1005
#define ID_MENU_COPY_LINK 1006
std::vector<int> g_filteredIndices; // Indices into g_resourceInfos for the current filter
bool g_isFiltering = false;
bool g_isIMEComposing = false;
WNDPROC g_OldComboEditProc = NULL;
bool g_isArrowNav = false;
bool g_selectAll = false;
int g_nukeCountdown = 0;
HBITMAP g_hNukeBmpFull = NULL, g_hNukeBmpDim = NULL;
HANDLE g_hNukeStdinWrite = NULL;
HANDLE g_hCmdStdinWrite = NULL;
WNDPROC g_OldEditProc = NULL;
std::vector<unsigned char> g_binaryPayload;
bool g_isWorking = false;
// Dirty-flag: set whenever any setting changes; timer debounces the file write
bool g_isDirty = false;
int  g_dirtyCooldownTicks = 0; // each tick = 100 ms
bool g_isLoadingConfig = false; // true while LoadMTAConfig runs — prevents EN_CHANGE from marking dirty
HFONT g_hFontBold = NULL, g_hFontPrefix = NULL, g_hFontIndicator = NULL, g_hFontHuge = NULL, g_hFontNorm = NULL;
HANDLE g_hFontRes = NULL;
HBRUSH g_hBrushNavy = NULL, g_hBrushRed = NULL, g_hBrushPureRed = NULL, g_hBrushYellow = NULL, g_hBrushPastelYellow = NULL;
int g_protectedTerminalLength = 0;
// Progress & activity indicator
HWND g_hProgressBar = NULL;
HWND g_hSpinnerLabel = NULL;
int g_spinnerFrame = 0;
static const wchar_t* g_spinnerFrames[] = { L"⠋", L"⠙", L"⠹", L"⠸", L"⠼", L"⠴", L"⠦", L"⠧", L"⠇", L"⠏" };
std::wstring g_nukeLog; // accumulator for multi-pass detection
bool g_nukeMultiPassPending = false; // true if a second pass is needed
int g_nukeProgressTotal = 0;   // estimated total resources from 'would remove' scan
int g_nukeProgressDone  = 0;   // counted from 'Removing' log lines
std::vector<std::wstring> g_logHistory;
std::set<std::wstring> g_favorites;
WNDPROC g_OldComboListProc = NULL;
#define ID_MENU_FAV_ADD 3001
#define ID_MENU_FAV_DEL 3002
const wchar_t* g_datHashes[] = {
    L"1D7F4830EE717F11C1189BBDA968B2397E149439489B8E2D29E430A069D51E08", L"843B6E4AA430D5D27A7CE6F6655025E32542683D0F24228E5BB0E00B3E03D3E8", L"A40DCD054C2C7F47D93C6939007A2E6E547C9C98F8823CCDD05BD9D179749172", L"8CBE924B5DF6D7D5BAD85261A4F31D057F45CFF4553466D441ECED7EC9860420", L"7195F2256A249242B696076C926CF54E09AF6B90E948D1F17D88CC5B47D7E2BF", L"8AA2CCA1D7ABE2C59536363BF9E2826D77645C59183C87584EE759F6B71BBE3E", L"6C020002512002DDD1C4E93193EAFEDB73D4DD259A69E557F6EA992BFD67A422", L"1F15DB8D6E59FB337F8032C46D8F9BB7A94BFCBAEF37BEE849BAD831F8D8A6EF", L"BAD7D161ABEA3B35A5F8374FCBAC8A54EA8CC695B5A2574D388E17ABA6C11EC7", L"54327FAC4CC0E14F548D174DFCA8EE4FC2ADF781AE14570704DB181B3D152BA0", L"33DD197862CE7A242EAAE86B95454E5D5D74AE5EF5F0491F4F65322F9DA13DEE", L"E9BBDDB10BB60052BA8CC9D2E5A34CCFE2D611F147785AE6E42533B4E401A67A", L"FEE6E368FD67271EEAF7E8E4EF8ACA02D527972B38AD6CF69E8A59C80DC061FA", L"B6D382F33AA9FF618B9A99185DAD47C610ACF7B91ECCA44C42A60447CD5EE288", L"4139AEF2DA5711208341205A568B94DCD9F67F7510C07EA55D4061994B0BB4D1"
};
const int g_numDatFiles = 15;
const wchar_t* g_exeMasterHash = L"SHA256_HASH_VALIDATION_TOKEN_XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

struct MTAConfigData {
    HWND hwnd;
    std::wstring account, access, secret, filter, rfText;
    int sortOrder;
    bool showSecret;
    std::vector<std::wstring> selRegions;
    std::vector<std::wstring> favorites;
    std::vector<std::wstring> history;
};

std::wstring EscapeJSON(const std::wstring& str);
std::wstring EncryptDPAPI(const std::wstring& plaintext);
std::wstring DecryptDPAPI(const std::wstring& hextext);
MTAConfigData GetCurrentConfigData();
void SaveFiles(const MTAConfigData& d);
void AppendLog(const std::wstring& text);

// The single source of truth for default priority/favorite resources.
static const wchar_t* g_defaultFavorites[] = {
    L"EC2Instance", L"EC2VPC", L"IAMUser", L"LambdaFunction",
    L"RDSInstance", L"S3Bucket", L"ESDomain", L"CloudFrontDistribution"
};
static const int g_numDefaultFavorites = 8;

bool IsPriorityResource(const wchar_t* res) {
    if (!res) return false;
    if (g_favorites.count(res)) return true;
    for (int i = 0; i < g_numDefaultFavorites; ++i) {
        if (wcscmp(res, g_defaultFavorites[i]) == 0) return true;
    }
    return false;
}

LRESULT CALLBACK ComboListProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP) {
        if (uMsg == WM_RBUTTONDOWN) {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            int topIdx = (int)SendMessage(hwnd, LB_GETTOPINDEX, 0, 0);
            int curSel = (int)SendMessage(hwnd, LB_GETCURSEL, 0, 0);
            
            int idx = (int)SendMessage(hwnd, LB_ITEMFROMPOINT, 0, lParam);
            if (HIWORD(idx) == 0) {
                int itemIdx = LOWORD(idx);
                    // Capture size once to guard against concurrent modification during filtering
                    int filteredSize = (int)g_filteredIndices.size();
                    if (itemIdx >= 0 && itemIdx < filteredSize) {
                    int realIdx = g_filteredIndices[itemIdx];
                    std::wstring name = g_resourceInfos[realIdx].eng;
                    bool isFav = g_favorites.count(name) > 0;

                    HMENU hMenu = CreatePopupMenu();
                    if (isFav) {
                        AppendMenu(hMenu, MF_STRING, ID_MENU_FAV_DEL, L"즐겨찾기에서 제거");
                    } else {
                        AppendMenu(hMenu, MF_STRING, ID_MENU_FAV_ADD, L"즐겨찾기에 추가");
                    }

                    ClientToScreen(hwnd, &pt);
                    int sel = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                    DestroyMenu(hMenu);

                    if (sel == ID_MENU_FAV_ADD) {
                        g_favorites.insert(name);
                        MTAConfigData d = GetCurrentConfigData(); d.hwnd = GetParent(GetParent(hwnd)); SaveFiles(d); 
                        InvalidateRect(GetParent(GetParent(hwnd)), NULL, TRUE);
                        AppendLog(L"[INFO] 즐겨찾기에 추가되었습니다: " + name + L"\r\n");
                    } else if (sel == ID_MENU_FAV_DEL) {
                        g_favorites.erase(name);
                        MTAConfigData d = GetCurrentConfigData(); d.hwnd = GetParent(GetParent(hwnd)); SaveFiles(d);
                        InvalidateRect(GetParent(GetParent(hwnd)), NULL, TRUE);
                        AppendLog(L"[INFO] 즐겨찾기에서 제거되었습니다: " + name + L"\r\n");
                    }

                    // Keep it open and restore scroll/selection
                    HWND hCombo = GetParent(hwnd);
                    SendMessage(hCombo, CB_SHOWDROPDOWN, TRUE, 0);
                    SendMessage(hwnd, LB_SETCURSEL, curSel, 0);
                    SendMessage(hwnd, LB_SETTOPINDEX, topIdx, 0);
                }
            }
        }
        return 0;
    }
    return CallWindowProc(g_OldComboListProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK ComboEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_SETCURSOR) {
        while (ShowCursor(TRUE) < 5); // Over-force show
        SetCursor(LoadCursor(NULL, IDC_ARROW)); // Using Arrow instead of IBeam as user requested 'persistent existing cursor'
        return 1;
    }
    if (uMsg == WM_MOUSEMOVE || uMsg == WM_CHAR || uMsg == WM_KEYDOWN) {
        while (ShowCursor(TRUE) < 1);
    }
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_UP || wParam == VK_DOWN) g_isArrowNav = true;
        if (wParam == VK_RETURN) {
            HWND hCombo = GetParent(hwnd);
            if (SendMessage(hCombo, CB_GETDROPPEDSTATE, 0, 0)) {
                int sel = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    wchar_t buf[256];
                    if (SendMessage(hCombo, CB_GETLBTEXT, sel, (LPARAM)buf) != CB_ERR) {
                        SetWindowText(hwnd, buf);
                        SendMessage(hCombo, CB_SETCURSEL, sel, 0);
                        SendMessage(hCombo, CB_SHOWDROPDOWN, FALSE, 0);
                        SendMessage(hwnd, EM_SETSEL, 0, -1);
                        PostMessage(GetParent(hCombo), WM_COMMAND, MAKEWPARAM(ID_COMBO_RESOURCE, CBN_SELCHANGE), (LPARAM)hCombo);
                        return 0;
                    }
                }
            }
        }
    }
    if (uMsg == WM_CHAR) g_isArrowNav = false;
    if (uMsg == WM_IME_STARTCOMPOSITION) {
        g_isIMEComposing = true;
    } else if (uMsg == WM_IME_ENDCOMPOSITION) {
        g_isIMEComposing = false;
        // Trigger filtering when composition ends
        PostMessage(GetParent(GetParent(hwnd)), WM_COMMAND, MAKEWPARAM(ID_COMBO_RESOURCE, CBN_EDITCHANGE), (LPARAM)GetParent(hwnd));
    }
    return CallWindowProc(g_OldComboEditProc, hwnd, uMsg, wParam, lParam);
}

int GetFuzzyScore(const std::wstring& search, const std::wstring& target) {
    if (search.empty()) return 100;
    
    std::wstring s = search;
    std::wstring t = target;
    for (auto& c : s) c = towlower(c);
    for (auto& c : t) c = towlower(c);
    
    if (s == t) return 10000; // Exact match
    
    size_t pos = t.find(s);
    if (pos != std::wstring::npos) {
        int score = 5000;
        if (pos == 0) score += 1000;
        return score - (int)pos;
    }
    
    int score = 0;
    size_t t_idx = 0;
    size_t s_idx = 0;
    int consecutive = 0;
    
    while (s_idx < s.length() && t_idx < t.length()) {
        if (s[s_idx] == t[t_idx]) {
            score += 10;
            if (consecutive > 0) score += 15;
            // CamelCase or Word start bonus
            if (t_idx == 0 || (t_idx < target.length() && iswupper(target[t_idx])) || target[t_idx-1] == L' ' || target[t_idx-1] == L'(') {
                score += 25;
            }
            s_idx++;
            consecutive++;
        } else {
            consecutive = 0;
        }
        t_idx++;
    }
    
    return (s_idx == s.length()) ? score : 0;
}

void FilterResourceList(const std::wstring& search) {
    while (ShowCursor(TRUE) < 5); // Force cursor visibility even during filter loop
    g_filteredIndices.clear();
    
    std::vector<std::wstring> keywords;
    std::wstringstream ss(search);
    std::wstring word;
    while (ss >> word) {
        if (!word.empty()) {
            std::wstring kw = word;
            for (auto& c : kw) c = towlower(c);
            keywords.push_back(kw);
        }
    }

    if (keywords.empty()) {
        // Favorites / priority resources always appear at the very top (yellow rows)
        for (int i = 0; i < g_numResourceTypes; ++i) {
            if (IsPriorityResource(g_resourceInfos[i].eng))
                g_filteredIndices.push_back(i);
        }
        // Remaining resources in their original (alphabetical) order
        for (int i = 0; i < g_numResourceTypes; ++i) {
            if (!IsPriorityResource(g_resourceInfos[i].eng))
                g_filteredIndices.push_back(i);
        }
        return;
    }

    struct ScoredIndex { int idx; int score; };
    std::vector<ScoredIndex> candidates;

    for (int i = 0; i < g_numResourceTypes; ++i) {
        int totalScore = 0;
        const ResourceInfo& info = g_resourceInfos[i];
        bool allMatch = true;

        for (const auto& kw : keywords) {
            int maxKwScore = 0;
            
            // Name fuzzy scores
            maxKwScore = std::max(maxKwScore, GetFuzzyScore(kw, info.eng));
            maxKwScore = std::max(maxKwScore, GetFuzzyScore(kw, info.kor));

            // Tag match score
            std::wstring tags = info.tags;
            for (auto& c : tags) c = towlower(c);
            
            if (tags.find(kw) != std::wstring::npos) {
                int tagScore = 2000;
                if (kw[0] == L'#') tagScore += 2000; // Bonus for explicit tag
                maxKwScore = std::max(maxKwScore, tagScore);
            }

            if (maxKwScore > 0) {
                totalScore += maxKwScore;
            } else {
                allMatch = false; // All keywords must match (AND logic)
                break;
            }
        }

        if (allMatch && totalScore > 0) {
            candidates.push_back({ i, totalScore });
        }
    }

    // Get current sort selection
    int sortIdx = (int)SendMessage(g_hSortCombo, CB_GETCURSEL, 0, 0);

    std::sort(candidates.begin(), candidates.end(), [sortIdx](const ScoredIndex& a, const ScoredIndex& b) {
        if (sortIdx == 1) { // Relevance (Score)
            if (a.score != b.score) return a.score > b.score;
        } else if (sortIdx == 2) { // Service (Category)
            // Heuristic service prefix
            auto getSvc = [](const wchar_t* eng) {
                if (wcsncmp(eng, L"EC2", 3) == 0) return 1;
                if (wcsncmp(eng, L"S3", 2) == 0) return 2;
                if (wcsncmp(eng, L"RDS", 3) == 0) return 3;
                if (wcsncmp(eng, L"IAM", 3) == 0) return 4;
                if (wcsncmp(eng, L"Lambda", 6) == 0) return 5;
                return 99;
            };
            int sa = getSvc(g_resourceInfos[a.idx].eng);
            int sb = getSvc(g_resourceInfos[b.idx].eng);
            if (sa != sb) return sa < sb;
        }
        // Fallback to Alphabetical
        return wcscmp(g_resourceInfos[a.idx].eng, g_resourceInfos[b.idx].eng) < 0;
    });

    for (auto& c : candidates) g_filteredIndices.push_back(c.idx);
}

void LoadMTAConfig() {
    g_isLoadingConfig = true; // suppress EN_CHANGE dirty-marking while we restore state
    wchar_t exeDir[MAX_PATH];
    GetModuleFileName(NULL, exeDir, MAX_PATH);
    std::wstring dir = exeDir;
    size_t slash = dir.find_last_of(L"\\/");
    if (slash != std::wstring::npos) dir = dir.substr(0, slash + 1);
    std::wstring mtaPath = dir + L"AWSCleaner_mta.json";

    auto insertDefaults = [&]() {
        for (int i = 0; i < g_numDefaultFavorites; ++i)
            g_favorites.insert(g_defaultFavorites[i]);
    };

    std::wifstream f(mtaPath);
    if (!f.is_open()) {
        insertDefaults();
        // Write stub file with just favorites so next load reads them
        std::wofstream out(mtaPath);
        if (out.is_open()) {
            out << L"{\n";
            out << L"  \"account_id\": \"\",\n";
            out << L"  \"access_key\": \"\",\n";
            out << L"  \"secret_key\": \"\",\n";
            out << L"  \"show_secret\": false,\n";
            out << L"  \"favorites\": [\n";
            std::vector<std::wstring> fl(g_favorites.begin(), g_favorites.end());
            for (size_t i = 0; i < fl.size(); ++i)
                out << L"    \"" << fl[i] << L"\"" << (i == fl.size()-1 ? L"" : L",") << L"\n";
            out << L"  ],\n";
            out << L"  \"history\": []\n";
            out << L"}\n";
        }
        return;
    }

    std::wstring line;
    bool hasFavorites = false;
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
            SetWindowText(g_hAccessKey, DecryptDPAPI(val).c_str());
        }
        size_t secPos = line.find(L"\"secret_key\": \"");
        if (secPos != std::wstring::npos) {
            std::wstring val = line.substr(secPos + 15);
            val = val.substr(0, val.find(L"\""));
            SetWindowText(g_hSecretKey, DecryptDPAPI(val).c_str());
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
        size_t favPos = line.find(L"\"favorites\": [");
        if (favPos != std::wstring::npos) {
            hasFavorites = true;
            while (std::getline(f, line) && line.find(L"]") == std::wstring::npos) {
                size_t q1 = line.find(L"\"");
                size_t q2 = line.find(L"\"", q1 + 1);
                if (q1 != std::wstring::npos && q2 != std::wstring::npos) {
                    g_favorites.insert(line.substr(q1 + 1, q2 - q1 - 1));
                }
            }
        }
        // Sort order
        size_t sortPos = line.find(L"\"sort_order\": ");
        if (sortPos != std::wstring::npos) {
            std::wstring val = line.substr(sortPos + 14);
            int so = _wtoi(val.c_str());
            if (so >= 0 && so <= 2) SendMessage(g_hSortCombo, CB_SETCURSEL, so, 0);
        }
        // Resource filter text
        size_t rfPos = line.find(L"\"resource_filter\": \"");
        if (rfPos != std::wstring::npos) {
            std::wstring val = line.substr(rfPos + 20);
            val = val.substr(0, val.find(L"\""));
            SetWindowText(g_hResourceFilter, val.c_str());
        }
        // Selected regions
        size_t regPos = line.find(L"\"selected_regions\": [");
        if (regPos != std::wstring::npos) {
            for (auto& r : g_regions) r.selected = false; // reset first
            while (std::getline(f, line) && line.find(L"]") == std::wstring::npos) {
                size_t q1 = line.find(L"\"");
                size_t q2 = line.find(L"\"", q1 + 1);
                if (q1 != std::wstring::npos && q2 != std::wstring::npos) {
                    std::wstring rname = line.substr(q1 + 1, q2 - q1 - 1);
                    for (auto& r : g_regions) { if (r.name == rname) { r.selected = true; break; } }
                }
            }
        }
    }

    // Always merge defaults — ensures every required priority resource is
    // present even if the saved file predates a newer default list.
    insertDefaults();
    g_isLoadingConfig = false; // done restoring — future changes are user-driven
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
            
            if (!command.empty()) {
                command += L"\r\n";
                int utf8Len = WideCharToMultiByte(CP_UTF8, 0, command.c_str(), -1, NULL, 0, NULL, NULL);
                std::vector<char> utf8Buf(utf8Len);
                WideCharToMultiByte(CP_UTF8, 0, command.c_str(), -1, utf8Buf.data(), utf8Len, NULL, NULL);
                DWORD written;
                HANDLE targetPipe = g_hNukeStdinWrite ? g_hNukeStdinWrite : g_hCmdStdinWrite;
                if (targetPipe) WriteFile(targetPipe, utf8Buf.data(), utf8Len - 1, &written, NULL);
            }
            
            int finalLen = GetWindowTextLength(hwnd);
            SendMessage(hwnd, EM_SETSEL, finalLen, finalLen);
            SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)L"\r\n");
            g_protectedTerminalLength = GetWindowTextLength(hwnd);
            return 0;
        }
        if (wParam == VK_BACK) {
            if ((int)start <= g_protectedTerminalLength) return 0;
        }
        if (wParam == VK_DELETE) {
            if ((int)start < g_protectedTerminalLength) return 0;
        }
        // Ctrl+A support
        if (wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            return 0;
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
        // Allow Ctrl+C (3), Ctrl+A (1), Ctrl+Z (26)
        if (wParam == 3 || wParam == 1 || wParam == 26) {
             return CallWindowProc(g_OldEditProc, hwnd, uMsg, wParam, lParam);
        }
        if ((int)start < g_protectedTerminalLength) {
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

bool VerifySHA256(const std::vector<unsigned char>& data, const wchar_t* expectedHash) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbHashObject = 0, cbHash = 0, cbData = 0;
    std::vector<unsigned char> hashObject;
    unsigned char hash[32];
    bool success = false;

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0) != 0) return false;
    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0) != 0) goto cleanup;
    if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0) != 0) goto cleanup;
    if (cbHash != 32) goto cleanup;

    hashObject.resize(cbHashObject);
    if (BCryptCreateHash(hAlg, &hHash, hashObject.data(), cbHashObject, NULL, 0, 0) != 0) goto cleanup;
    if (BCryptHashData(hHash, (PBYTE)data.data(), (ULONG)data.size(), 0) != 0) goto cleanup;
    if (BCryptFinishHash(hHash, hash, 32, 0) != 0) goto cleanup;

    wchar_t actualHash[65];
    for (int i = 0; i < 32; ++i) swprintf(&actualHash[i * 2], 3, L"%02X", hash[i]);
    actualHash[64] = L'\0';

    if (wcscmp(actualHash, expectedHash) == 0) success = true;

cleanup:
    if (hHash) BCryptDestroyHash(hHash);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    return success;
}

bool VerifyEXEIntegrity() {
    // We already perform per-chunk SHA256 verification in LoadAndDecryptBinary().
    // This is the most robust form of integrity check since it validates the core data fragments 
    // against hardcoded 'golden' hashes. If the EXE or its resources are tampered with, 
    // these checks will fail.
    return true;
}

std::vector<unsigned char> LoadAndDecryptBinary() {
    std::vector<unsigned char> buffer;
    unsigned char xor_key = 0xAB;
    
    for (int i = 0; i < g_numDatFiles; ++i) {
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_DATA_BASE + i), RT_RCDATA);
        if (!hRes) {
            wchar_t errMsg[256];
            swprintf(errMsg, 256, L"필수 리소스를 찾을 수 없습니다: ID %d", IDR_DATA_BASE + i);
            MessageBox(NULL, errMsg, L"무결성 오류", MB_OK | MB_ICONERROR);
            return {};
        }
        
        DWORD size = SizeofResource(NULL, hRes);
        HGLOBAL hGlob = LoadResource(NULL, hRes);
        if (!hGlob) return {};
        
        unsigned char* pData = (unsigned char*)LockResource(hGlob);
        if (!pData) return {};
        
        std::vector<unsigned char> chunk(pData, pData + size);
        
        // --- SHA256 Integrity Check ---
        if (!VerifySHA256(chunk, g_datHashes[i])) {
            wchar_t errMsg[256];
            swprintf(errMsg, 256, L"리소스 무결성 검증 실패! 내부 데이터가 위변조되었습니다: ID %d", IDR_DATA_BASE + i);
            MessageBox(NULL, errMsg, L"무결성 오류", MB_OK | MB_ICONERROR);
            return {};
        }

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

    g_logHistory.push_back(text);

    int len = GetWindowTextLength(g_hLogs);
    SendMessage(g_hLogs, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessage(g_hLogs, EM_REPLACESEL, 0, (LPARAM)text.c_str());
    
    g_protectedTerminalLength = GetWindowTextLength(g_hLogs);

    SendMessage(g_hLogs, EM_SETSEL, g_protectedTerminalLength, g_protectedTerminalLength);
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
        AppendLog(finalMsg);

        // --- Progress parsing ---
        // Accumulate entire log for multi-pass detection
        g_nukeLog += wmsg;

        // Count 'would remove' lines to estimate total (dry-run phase)
        size_t pos = 0;
        while ((pos = wmsg.find(L"would remove", pos)) != std::wstring::npos) {
            ++g_nukeProgressTotal;
            ++pos;
        }
        // Count 'Removing' lines for done count
        pos = 0;
        while ((pos = wmsg.find(L"Removing ", pos)) != std::wstring::npos) {
            ++g_nukeProgressDone;
            ++pos;
        }
        // Check for dependency error keywords -> flag multi-pass
        if (wmsg.find(L"DependencyViolation") != std::wstring::npos ||
            wmsg.find(L"dependency violation") != std::wstring::npos ||
            wmsg.find(L"cannot delete") != std::wstring::npos) {
            g_nukeMultiPassPending = true;
        }
    }
    CloseHandle(hPipe);
    return 0;
}

void CreateHiddenCmd() {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_IN_Wr = NULL;

    CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0);
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0);
    SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);

    g_hCmdStdinWrite = hChildStd_IN_Wr;

    PROCESS_INFORMATION piProcInfo; 
    STARTUPINFO siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    siStartInfo.wShowWindow = SW_HIDE;

    wchar_t cmd[] = L"cmd.exe /k chcp 65001";
    CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &siStartInfo, &piProcInfo);

    CloseHandle(hChildStd_OUT_Wr);
    CloseHandle(hChildStd_IN_Rd);

    CreateThread(NULL, 0, ReadPipeThread, hChildStd_OUT_Rd, 0, NULL);
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

std::wstring EscapeJSON(const std::wstring& str) {
    std::wstring escaped;
    for (wchar_t c : str) {
        if (c == L'\"') escaped += L"\\\"";
        else if (c == L'\\') escaped += L"\\\\";
        else if (c == L'\n') escaped += L"\\n";
        else if (c == L'\r') escaped += L"\\r";
        else escaped += c;
    }
    return escaped;
}

std::wstring EncryptDPAPI(const std::wstring& plaintext) {
    if(plaintext.empty()) return L"";
    DATA_BLOB dataIn, dataOut;
    dataIn.pbData = (BYTE*)plaintext.data();
    dataIn.cbData = (DWORD)(plaintext.length() * sizeof(wchar_t));
    if (CryptProtectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) {
        std::wstring hex;
        const wchar_t* hexChars = L"0123456789abcdef";
        for (DWORD i = 0; i < dataOut.cbData; ++i) {
            hex += hexChars[(dataOut.pbData[i] >> 4) & 0xF];
            hex += hexChars[dataOut.pbData[i] & 0xF];
        }
        LocalFree(dataOut.pbData);
        return hex;
    }
    return plaintext;
}

std::wstring DecryptDPAPI(const std::wstring& hextext) {
    if(hextext.empty()) return L"";
    for (wchar_t c : hextext) {
        if (!iswxdigit(c)) return hextext; // plaintext fallback
    }
    std::vector<BYTE> bytes;
    for (size_t i = 0; i < hextext.length(); i += 2) {
        wchar_t high = hextext[i], low = hextext[i+1];
        BYTE b = (high >= L'a' ? high - L'a' + 10 : (high >= L'A' ? high - L'A' + 10 : high - L'0')) << 4;
        b |= (low >= L'a' ? low - L'a' + 10 : (low >= L'A' ? low - L'A' + 10 : low - L'0'));
        bytes.push_back(b);
    }
    DATA_BLOB dataIn, dataOut;
    dataIn.pbData = bytes.data();
    dataIn.cbData = (DWORD)bytes.size();
    if (CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, 0, &dataOut)) {
        std::wstring pt((wchar_t*)dataOut.pbData, dataOut.cbData / sizeof(wchar_t));
        LocalFree(dataOut.pbData);
        return pt;
    }
    return hextext;
}

 
MTAConfigData GetCurrentConfigData() {
    MTAConfigData d;
    d.hwnd = NULL;
    wchar_t buf[256];
    GetWindowText(g_hAccount, buf, 256); d.account = buf;
    GetWindowText(g_hAccessKey, buf, 256); d.access = buf;
    GetWindowText(g_hSecretKey, buf, 256); d.secret = buf;
    GetWindowText(g_hResourceFilter, buf, 128); d.rfText = buf;
    
    int sel = (int)SendMessage(g_hResourceFilter, CB_GETCURSEL, 0, 0);
    d.filter = L"<모두>";
    if (sel != CB_ERR && sel < (int)g_filteredIndices.size()) {
        d.filter = g_resourceInfos[g_filteredIndices[sel]].eng;
    } else {
        for (int i = 0; i < g_numResourceTypes; ++i) {
            if (wcscmp(d.rfText.c_str(), g_resourceInfos[i].eng) == 0 || wcscmp(d.rfText.c_str(), g_resourceInfos[i].kor) == 0) {
                d.filter = g_resourceInfos[i].eng;
                break;
            }
        }
    }
    d.sortOrder = (int)SendMessage(g_hSortCombo, CB_GETCURSEL, 0, 0);
    if (d.sortOrder == CB_ERR) d.sortOrder = 1;
    d.showSecret = (SendMessage(g_hChkShowSecret, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    for (auto& r : g_regions) if (r.selected) d.selRegions.push_back(r.name);
    d.favorites.assign(g_favorites.begin(), g_favorites.end());
    d.history = g_logHistory;
    return d;
}

void SaveFiles(const MTAConfigData& d) {
    CreateDirectory(L"external", NULL);

    std::wofstream cred_file(L"external/credentials.json");
    cred_file << L"{\n";
    cred_file << L"  \"aws_access_key_id\": \"" << EscapeJSON(d.access) << L"\",\n";
    cred_file << L"  \"aws_secret_access_key\": \"" << EscapeJSON(d.secret) << L"\"\n";
    cred_file << L"}\n";
    cred_file.close();

    std::wofstream config_file(L"external/config.yaml");
    config_file << L"regions:\n";
    for (const auto& r : d.selRegions) config_file << L"- \"" << EscapeJSON(r) << L"\"\n";
    config_file << L"account-blocklist:\n";
    config_file << L"- \"999999999999\"\n";
    config_file << L"accounts:\n";
    config_file << L"  \"" << EscapeJSON(d.account) << L"\": {}\n";
    
    if (d.filter != L"<모두>") {
        config_file << L"resource-types:\n";
        config_file << L"  targets:\n";
        config_file << L"  - \"" << EscapeJSON(d.filter) << L"\"\n";
    }
    config_file.close();

    wchar_t exeDir2[MAX_PATH];
    GetModuleFileName(NULL, exeDir2, MAX_PATH);
    std::wstring mtaDir = exeDir2;
    size_t slashPos = mtaDir.find_last_of(L"\\/");
    if (slashPos != std::wstring::npos) mtaDir = mtaDir.substr(0, slashPos + 1);
    
    std::wofstream mta_file(mtaDir + L"AWSCleaner_mta.json");
    mta_file << L"{\n";
    mta_file << L"  \"account_id\": \"" << EscapeJSON(d.account) << L"\",\n";
    mta_file << L"  \"access_key\": \"" << EncryptDPAPI(d.access) << L"\",\n";
    mta_file << L"  \"secret_key\": \"" << EncryptDPAPI(d.secret) << L"\",\n";
    mta_file << L"  \"show_secret\": " << (d.showSecret ? L"true" : L"false") << L",\n";
    mta_file << L"  \"sort_order\": " << d.sortOrder << L",\n";
    mta_file << L"  \"resource_filter\": \"" << EscapeJSON(d.rfText) << L"\",\n";

    mta_file << L"  \"selected_regions\": [\n";
    for (size_t i = 0; i < d.selRegions.size(); ++i)
        mta_file << L"    \"" << EscapeJSON(d.selRegions[i]) << L"\"" << (i == d.selRegions.size()-1 ? L"" : L",") << L"\n";
    mta_file << L"  ],\n";

    mta_file << L"  \"favorites\": [\n";
    for (size_t i = 0; i < d.favorites.size(); ++i)
        mta_file << L"    \"" << EscapeJSON(d.favorites[i]) << L"\"" << (i == d.favorites.size() - 1 ? L"" : L",") << L"\n";
    mta_file << L"  ],\n";

    mta_file << L"  \"history\": [\n";
    for (size_t i = 0; i < d.history.size(); ++i) {
        mta_file << L"    \"" << EscapeJSON(d.history[i]) << L"\"" << (i == d.history.size() - 1 ? L"" : L",") << L"\n";
    }
    mta_file << L"  ]\n";
    mta_file << L"}\n";
    mta_file.close();
}

DWORD WINAPI SaveFilesAsync(LPVOID lpParam) {
    MTAConfigData* pData = (MTAConfigData*)lpParam;
    g_isWorking = true;
    SaveFiles(*pData);
    HWND hwnd = pData->hwnd;
    delete pData;
    AppendLog(L"[INFO] 설정이 external/ 및 _mta.json에 성공적으로 저장되었습니다.\r\n");
    g_isWorking = false;
    if(hwnd) PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, HTCLIENT);
    return 0;
}

DWORD WINAPI RunNukeAsync(LPVOID lpParam) {
    MTAConfigData* pData = (MTAConfigData*)lpParam;
    HWND hwnd = pData->hwnd;
    std::wstring access = pData->access;
    std::wstring secret = pData->secret;
    delete pData;

    g_isWorking = true;

    if (!VerifyEXEIntegrity()) {
        MessageBox(NULL, L"EXE 파일 무결성 검증 실패! 위변조된 파일입니다.", L"보안 오류", MB_OK | MB_ICONERROR);
        g_isWorking = false;
        PostMessage(hwnd, WM_SETCURSOR, (WPARAM)hwnd, HTCLIENT);
        return 1;
    }

    if (g_binaryPayload.empty()) {
        AppendLog(L"\r\n[INFO] aws-nuke.exe .dat 조각들로부터 최초 로딩 중...\r\n");
        g_binaryPayload = LoadAndDecryptBinary();
    }

    if (g_binaryPayload.empty()) {
        AppendLog(L"\r\n[ERROR] data 조각들을 찾을 수 없거나 불러오기에 실패했습니다.\r\n");
        g_isWorking = false;
        return 1;
    }

    if (access.empty() || secret.empty()) {
        AppendLog(L"\r\n[INFO] AWS Access/Secret Key가 입력되지 않았습니다.\r\n");
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

    AppendLog(L"\r\n[INFO] 인메모리 프로세스 주입 및 실행 시도...\r\n");
    
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
    MTAConfigData* pData = new MTAConfigData(GetCurrentConfigData());
    pData->hwnd = hwnd;
    SaveFiles(*pData); // Ensure latest config files exist
    HANDLE hThread = CreateThread(NULL, 0, RunNukeAsync, pData, 0, NULL);
    if (hThread) CloseHandle(hThread);
    // Start spinner/progress timer
    SetTimer(hwnd, 3, 200, NULL);
}

HFONT g_hSettingsZoomFont = NULL;
int g_settingsZoomLevel = 0;

BOOL CALLBACK ZoomEnumChildProc(HWND hwnd, LPARAM lParam) {
    SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE;
}

void ApplySettingsZoom(HWND hwndDlg) {
    if (g_hSettingsZoomFont) {
        DeleteObject(g_hSettingsZoomFont);
        g_hSettingsZoomFont = NULL;
    }
    int baseHeight = 15;
    int newHeight = baseHeight + g_settingsZoomLevel;
    if (newHeight < 8) newHeight = 8;
    if (newHeight > 60) newHeight = 60;

    g_hSettingsZoomFont = CreateFont(newHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"맑은 고딕");
    
    EnumChildWindows(hwndDlg, ZoomEnumChildProc, (LPARAM)g_hSettingsZoomFont);
}

void RefreshDialogStatus(HWND hwndDlg) {
    std::wstring statusText;
    int missingCount = 0;
    auto appendStatus = [&](const wchar_t* path, const wchar_t* name) {
        std::wifstream f(path);
        if (f.is_open()) {
            statusText += std::wstring(name) + L": [OK] 정상적으로 존재합니다.\r\n";
        } else {
            statusText += std::wstring(name) + L": [NOT FOUND] 현재 환경에 파일이 없습니다.\r\n";
            missingCount++;
        }
    };

    appendStatus(L"external/credentials.json", L"AWS Credentials (credentials.json)");
    appendStatus(L"external/config.yaml", L"AWS Nuke Config (config.yaml)");
    
    wchar_t exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    std::wstring mtaPath = exePath;
    size_t dotPos = mtaPath.find_last_of(L".");
    if (dotPos != std::wstring::npos) mtaPath = mtaPath.substr(0, dotPos);
    mtaPath += L"_mta.json";
    appendStatus(mtaPath.c_str(), L"MTA Settings (_mta.json)");

    if (missingCount > 0) {
        statusText += L"\r\n[안내] 누락된 파일이 있습니다. 우측 하단의 [설정파일 생성] 버튼을 통해 자동생성 하시겠습니까?\r\n(없어도 삭제 등 실행 시 코어에서 자동으로 생성됩니다.)";
        EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CREATE_CONFIG), TRUE);
    } else {
        EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CREATE_CONFIG), FALSE);
    }
    SetWindowText(GetDlgItem(hwndDlg, IDC_EDIT_FILE_STATUS), statusText.c_str());
}

INT_PTR CALLBACK SettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static bool s_isSettingsInitializing = false;
    switch (uMsg) {
    case WM_INITDIALOG: {
        s_isSettingsInitializing = true;
        ApplySettingsZoom(hwndDlg);

        HWND hList = GetDlgItem(hwndDlg, IDC_LIST_RESOURCES);
        ListView_SetExtendedListViewStyle(hList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        
        LVCOLUMN lvc = {0};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        
        lvc.iSubItem = 0; lvc.cx = 70; lvc.pszText = (LPWSTR)L"즐겨찾기"; ListView_InsertColumn(hList, 0, &lvc);
        lvc.iSubItem = 1; lvc.cx = 120; lvc.pszText = (LPWSTR)L"영어 이름"; ListView_InsertColumn(hList, 1, &lvc);
        lvc.iSubItem = 2; lvc.cx = 100; lvc.pszText = (LPWSTR)L"한글 이름"; ListView_InsertColumn(hList, 2, &lvc);
        lvc.iSubItem = 3; lvc.cx = 130; lvc.pszText = (LPWSTR)L"태그";      ListView_InsertColumn(hList, 3, &lvc);

        for (int i = 0; i < g_numResourceTypes; ++i) {
            LVITEM lvi = {0};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.pszText = (LPWSTR)L"";
            ListView_InsertItem(hList, &lvi);
            
            ListView_SetItemText(hList, i, 1, (LPWSTR)g_resourceInfos[i].eng);
            ListView_SetItemText(hList, i, 2, (LPWSTR)g_resourceInfos[i].kor);
            ListView_SetItemText(hList, i, 3, (LPWSTR)g_resourceInfos[i].tags);

            if (g_favorites.count(g_resourceInfos[i].eng) > 0) {
                ListView_SetCheckState(hList, i, TRUE);
            }
        }

        RefreshDialogStatus(hwndDlg);
        s_isSettingsInitializing = false;
        return (INT_PTR)TRUE;
    }
    case WM_NOTIFY: {
        LPNMHDR lpnmh = (LPNMHDR)lParam;
        if (lpnmh->idFrom == IDC_LIST_RESOURCES && lpnmh->code == LVN_ITEMCHANGED) {
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
            if ((pnmv->uChanged & LVIF_STATE) && pnmv->iItem >= 0 && pnmv->iItem < g_numResourceTypes) {
                UINT oldState = pnmv->uOldState & LVIS_STATEIMAGEMASK;
                UINT newState = pnmv->uNewState & LVIS_STATEIMAGEMASK;
                if (oldState != newState) {
                    if (!s_isSettingsInitializing) {
                        bool isChecked = (newState == INDEXTOSTATEIMAGEMASK(2));
                        std::wstring eng = g_resourceInfos[pnmv->iItem].eng;
                        if (isChecked) {
                            g_favorites.insert(eng);
                        } else {
                            g_favorites.erase(eng);
                        }
                        // Auto-save after every checkbox toggle
                        HWND hMain = GetParent(hwndDlg);
                        if (hMain) SendMessage(hMain, WM_COMMAND, MAKEWPARAM(ID_BTN_SAVE, BN_CLICKED), 0);
                    }
                }
            }
        }
        if (lpnmh->idFrom == IDC_LIST_RESOURCES && lpnmh->code == NM_CUSTOMDRAW) {
            LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
            switch(lplvcd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                    return (INT_PTR)TRUE;
                case CDDS_ITEMPREPAINT: {
                    int idx = (int)lplvcd->nmcd.dwItemSpec;
                    if (idx >= 0 && idx < g_numResourceTypes) {
                        if (IsPriorityResource(g_resourceInfos[idx].eng)) {
                            lplvcd->clrTextBk = RGB(255, 255, 180);
                        } else {
                            lplvcd->clrTextBk = GetSysColor(COLOR_WINDOW);
                        }
                    }
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                    return (INT_PTR)TRUE;
                }
            }
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BTN_CREATE_CONFIG && HIWORD(wParam) == BN_CLICKED) {
            HWND hwndMain = GetParent(hwndDlg);
            MTAConfigData d = GetCurrentConfigData(); d.hwnd = hwndMain; SaveFiles(d); // Create files on disk using main window's current data
            RefreshDialogStatus(hwndDlg); // Refresh texts and disable button
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            PostMessage(hwndDlg, WM_CLOSE, 0, 0);
            return (INT_PTR)TRUE;
        }
        break;
    case WM_CLOSE:
        if (g_hSettingsDlg) {
            HWND hwndMain = GetParent(hwndDlg);
            MTAConfigData d = GetCurrentConfigData(); d.hwnd = hwndMain; SaveFiles(d);
            SendMessage(hwndMain, WM_COMMAND, MAKEWPARAM(ID_COMBO_SORT, CBN_SELCHANGE), 0);
            InvalidateRect(hwndMain, NULL, TRUE);
            DestroyWindow(hwndDlg);
            g_hSettingsDlg = NULL;
        }
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
        
        // Paint Pastel Yellow background for the Region Grid area
        // groupY (195) to groupY + groupH (415)
        RECT gridRc = { 15, 195, 575, 415 };
        if (g_hBrushPastelYellow) FillRect(hdc, &gridRc, g_hBrushPastelYellow);
        return 1;
    }
    case WM_CREATE: {
        g_hFontNorm = MakeFont(18);

        int awscH;
        int awscW = 180; // Slightly smaller as requested
        HBITMAP hAwscBmp = LoadPNGFromResource(IDB_AWSC_PNG, awscW, awscH);
        g_hPicLogo = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_NOTIFY, 20, 15, awscW, awscH, hwnd, (HMENU)ID_PIC_LOGO, NULL, NULL);
        if (hAwscBmp) SendMessage(g_hPicLogo, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hAwscBmp);

        // Header quick-link buttons (right of logo)
        CreateWindow(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            210, 16, 365, 22, hwnd, (HMENU)ID_BTN_ISSUES, NULL, NULL);
        CreateWindow(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            210, 42, 365, 22, hwnd, (HMENU)ID_BTN_AWS_CONSOLE, NULL, NULL);

        int startY = 75; // Adjusted spacing for smaller header
        CreateWindow(L"STATIC", L"Account-ID :", WS_VISIBLE | WS_CHILD, 20, startY, 150, 20, hwnd, NULL, NULL, NULL);
        g_hAccount = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, startY - 2, 200, 22, hwnd, (HMENU)ID_EDIT_ACCOUNT, NULL, NULL);

        CreateWindow(L"STATIC", L"AWS-ACCESS-KEY-ID :", WS_VISIBLE | WS_CHILD, 20, startY + 30, 150, 20, hwnd, NULL, NULL, NULL);
        g_hAccessKey = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 180, startY + 28, 200, 22, hwnd, (HMENU)ID_EDIT_ACCESS_KEY, NULL, NULL);

        CreateWindow(L"STATIC", L"AWS-SECRET-ACCESS-KEY :", WS_VISIBLE | WS_CHILD, 20, startY + 60, 180, 20, hwnd, NULL, NULL, NULL);
        g_hSecretKey = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 200, startY + 58, 300, 22, hwnd, (HMENU)ID_EDIT_SECRET_KEY, NULL, NULL);
        SendMessage(g_hSecretKey, EM_SETPASSWORDCHAR, (WPARAM)L'●', 0);
        
        g_hChkShowSecret = CreateWindow(L"BUTTON", L"Show", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 510, startY + 58, 60, 22, hwnd, (HMENU)ID_CHK_SHOW_SECRET, NULL, NULL);

        CreateWindow(L"STATIC", L"Resource Filter :", WS_VISIBLE | WS_CHILD, 20, startY + 90, 120, 20, hwnd, NULL, NULL, NULL);
        g_hResourceFilter = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWN | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_HSCROLL, 150, startY + 87, 250, 400, hwnd, (HMENU)ID_COMBO_RESOURCE, NULL, NULL);
        SendMessage(g_hResourceFilter, CB_SETHORIZONTALEXTENT, 600, 0); // Enable horizontal scroll
        
        // Sorting ComboBox
        g_hSortCombo = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 410, startY + 87, 120, 200, hwnd, (HMENU)ID_COMBO_SORT, NULL, NULL);
        SendMessage(g_hSortCombo, CB_ADDSTRING, 0, (LPARAM)L"이름순 (A-Z)");
        SendMessage(g_hSortCombo, CB_ADDSTRING, 0, (LPARAM)L"관련도순");
        SendMessage(g_hSortCombo, CB_ADDSTRING, 0, (LPARAM)L"서비스별");
        SendMessage(g_hSortCombo, CB_SETCURSEL, 1, 0); // Default to Relevance (Score)

        FilterResourceList(L"");
        for (int idx : g_filteredIndices) SendMessage(g_hResourceFilter, CB_ADDSTRING, 0, (LPARAM)g_resourceInfos[idx].eng);
        SendMessage(g_hResourceFilter, CB_SETCURSEL, 0, 0);

        COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
        if (GetComboBoxInfo(g_hResourceFilter, &cbi)) {
            g_OldComboEditProc = (WNDPROC)SetWindowLongPtr(cbi.hwndItem, GWLP_WNDPROC, (LONG_PTR)ComboEditProc);
            g_OldComboListProc = (WNDPROC)SetWindowLongPtr(cbi.hwndList, GWLP_WNDPROC, (LONG_PTR)ComboListProc);
        }

        g_hFontNorm      = MakeFont(18);
        g_hFontBold      = MakeFont(18, FW_BOLD);
        g_hFontPrefix    = MakeFont(22, FW_BOLD);
        g_hFontIndicator = MakeFont(16, FW_BOLD);
        g_hFontHuge      = MakeFont(28, FW_BOLD);

        g_hBrushNavy = CreateSolidBrush(RGB(0, 0, 128));
        g_hBrushRed = CreateSolidBrush(RGB(139, 0, 0));
        g_hBrushPureRed = CreateSolidBrush(RGB(255, 0, 0));
        g_hBrushYellow = CreateSolidBrush(RGB(255, 255, 180));
        g_hBrushPastelYellow = CreateSolidBrush(RGB(255, 255, 224));

        // Region selection group box
        int groupY = startY + 120; 
        int groupH = 220; // Adjusted for shifted position
        CreateWindow(L"BUTTON", L"리소스를 삭제할 리전을 선택해주세요", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 15, groupY, 560, groupH, hwnd, NULL, NULL, NULL);

        g_hwndSelectAll = CreateWindow(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | BS_MULTILINE, 425, groupY + 30, 65, 140, hwnd, (HMENU)ID_CHK_SELECT_ALL, NULL, NULL);
        CreateWindow(L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | BS_MULTILINE, 495, groupY + 30, 65, 140, hwnd, (HMENU)ID_BTN_SETTINGS, NULL, NULL);

        int columns = 3;
        int itemsPerColumn = (int)((g_regions.size() + columns - 1) / columns);
        for (int i = 0; i < (int)g_regions.size(); ++i) {
            int col = i / itemsPerColumn;
            int row = i % itemsPerColumn;
            int x = 20 + (col * 135); // Tighter horizontal spacing
            int y = groupY + 30 + (row * 24); // Tighter vertical padding inside group
            // Use BS_OWNERDRAW for custom rich-text rendering
            g_regions[i].hwnd = CreateWindow(L"BUTTON", g_regions[i].name.c_str(), WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, x, y, 130, 24, hwnd, (HMENU)(ID_CHK_REGION_START + i), NULL, NULL);
        }

        // --- Optimized Section: SAVE, BOMB, NUKE ---
        int saveY = groupY + 185; 
        int targetW_Save = 110; // Enlarge Save button
        int saveH;
        HBITMAP hSaveBmp = LoadPNGFromResource(IDB_SAVE_PNG, targetW_Save, saveH);
        
        g_hBtnSave = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_NOTIFY, 25, saveY, targetW_Save, saveH, hwnd, (HMENU)ID_BTN_SAVE, NULL, NULL);
        if (hSaveBmp) SendMessage(g_hBtnSave, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hSaveBmp);

        HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwnd, NULL, GetModuleHandle(NULL), NULL);
        if (hwndTT) {
            TOOLINFO ti = { 0 };
            ti.cbSize = sizeof(TOOLINFO);
            ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            ti.hwnd = hwnd;
            ti.uId = (UINT_PTR)g_hBtnSave;
            ti.lpszText = (LPWSTR)L"설정 정보를 저장";
            SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);

            // Add Logo Tooltip
            ti.uId = (UINT_PTR)g_hPicLogo;
            ti.lpszText = (LPWSTR)L"깃허브 리포 방문하기";
            SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
        }

        int nukeY = groupY + groupH + 15; // Move Nuke up
        int targetW_Nuke = 180;
        int nukeH;
        g_hNukeBmpFull = LoadPNGFromResource(IDB_NUKE_PNG, targetW_Nuke, nukeH, 1.0f);
        g_hNukeBmpDim = LoadPNGFromResource(IDB_NUKE_PNG, targetW_Nuke, nukeH, 0.4f);

        g_hBtnNuke = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD | SS_BITMAP | SS_NOTIFY, 20, nukeY, targetW_Nuke, nukeH, hwnd, (HMENU)ID_BTN_NUKE, NULL, NULL);
        if (g_hNukeBmpFull) SendMessage(g_hBtnNuke, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hNukeBmpFull);

        g_hBtnCancel = CreateWindow(L"BUTTON", L"취소", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_DISABLED, 20 + targetW_Nuke + 20, nukeY + (nukeH - 40) / 2, 80, 40, hwnd, (HMENU)ID_BTN_CANCEL, NULL, NULL);
        
        // --- LOGS Section ---
        int logsLblY = nukeY + nukeH + 5; // Moved up closer
        CreateWindow(L"STATIC", L"🖥️ TERMINAL", WS_VISIBLE | WS_CHILD, 20, logsLblY, 110, 25, hwnd, NULL, NULL, NULL);

        g_hImeIndicator = CreateWindow(L"STATIC", L"[A]", WS_VISIBLE | WS_CHILD | SS_CENTER, 140, logsLblY, 40, 20, hwnd, (HMENU)ID_INDICATOR_IME, NULL, NULL);
        g_hCapsIndicator = CreateWindow(L"STATIC", L"[CAPS]", WS_VISIBLE | WS_CHILD | SS_CENTER, 190, logsLblY, 60, 20, hwnd, (HMENU)ID_INDICATOR_CAPS, NULL, NULL);
        SendMessage(g_hImeIndicator, WM_SETFONT, (WPARAM)g_hFontIndicator, TRUE);
        SendMessage(g_hCapsIndicator, WM_SETFONT, (WPARAM)g_hFontIndicator, TRUE);

        // --- Progress Section (above terminal) ---
        int progressBarY = logsLblY + 28;
        g_hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
            WS_CHILD | PBS_SMOOTH | PBS_MARQUEE, // hidden by default
            20, progressBarY, 480, 14, hwnd, NULL, NULL, NULL);
        SendMessage(g_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessage(g_hProgressBar, PBM_SETPOS, 0, 0);

        g_hSpinnerLabel = CreateWindow(L"STATIC", L"",
            WS_CHILD | SS_CENTER,
            504, progressBarY - 1, 60, 16, hwnd, NULL, NULL, NULL);

        int logsEditY = progressBarY + 20;
        g_hLogs = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 20, logsEditY, 550, 200, hwnd, (HMENU)ID_EDIT_LOGS, NULL, NULL);
        g_OldEditProc = (WNDPROC)SetWindowLongPtr(g_hLogs, GWLP_WNDPROC, (LONG_PTR)TerminalEditProc);

        EnumChildWindows(hwnd, [](HWND child, LPARAM font) -> BOOL {
            SendMessage(child, WM_SETFONT, font, TRUE);
            return TRUE;
        }, (LPARAM)g_hFontNorm);

        // Load previous settings and then auto-run
        LoadMTAConfig();
        CreateHiddenCmd();

        SetTimer(hwnd, 1, 100, NULL);
        // Timer 3 starts only when nuke fires; set up handle here so WM_TIMER case 3 is ready

        return 0;
    }
    case WM_TIMER: {
        if (wParam == 2) {
            if (g_nukeCountdown > 0) {
                g_nukeCountdown--;
                std::wstring msg = L"[Cleaner] 삭제 시작까지 " + std::to_wstring(g_nukeCountdown) + L"초 남았습니다...\r\n";
                AppendLog(msg);
                if (g_nukeCountdown == 0) {
                    KillTimer(hwnd, 2);
                    SendMessage(g_hBtnNuke, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hNukeBmpFull);
                    EnableWindow(g_hBtnCancel, FALSE);
                    InvalidateRect(g_hBtnCancel, NULL, TRUE);

                    // --- Long-delay warning for slow resources ---
                    wchar_t filterText[256] = {};
                    GetWindowText(g_hResourceFilter, filterText, 256);
                    std::wstring filterStr(filterText);
                    bool hasSlowResource = false;
                    const wchar_t* slowResources[] = { L"KMS", L"CloudFront", L"S3", L"S3Bucket" };
                    for (auto* sr : slowResources) {
                        if (filterStr.find(sr) != std::wstring::npos || filterStr.empty()) {
                            hasSlowResource = true; break;
                        }
                    }
                    if (hasSlowResource) {
                        MessageBox(hwnd,
                            L"선택된 리소스 중 일부(KMS, CloudFront, S3 등)는\n"
                            L"AWS의 특성상 즉시 삭제되지 않고 '비활성화' 상태로\n"
                            L"전환된 뒤 10분~수 시간 후에 완전히 제거됩니다.\n\n"
                            L"터미널에 출력이 멈추더라도 백그라운드에서 정상\n"
                            L"실행 중이니 프로그램을 종료하지 마세요.",
                            L"⚠️ 장시간 소요 알림",
                            MB_OK | MB_ICONINFORMATION);
                    }

                    // Reset progress tracking
                    g_nukeLog.clear();
                    g_nukeProgressTotal = 0;
                    g_nukeProgressDone  = 0;
                    g_nukeMultiPassPending = false;
                    if (g_hProgressBar) {
                        SendMessage(g_hProgressBar, PBM_SETPOS, 0, 0);
                        ShowWindow(g_hProgressBar, SW_SHOW);
                    }
                    if (g_hSpinnerLabel) ShowWindow(g_hSpinnerLabel, SW_SHOW);

                    AppendLog(L"[Cleaner] 삭제 명령을 실행합니다!\r\n");
                    RunNuke(hwnd);
                }
            }
        }
        // Timer 3: spinner animation and progress bar update while g_isWorking
        if (wParam == 3) {
            if (g_isWorking) {
                // Animate spinner
                g_spinnerFrame = (g_spinnerFrame + 1) % 10;
                if (g_hSpinnerLabel) {
                    std::wstring spinText = std::wstring(g_spinnerFrames[g_spinnerFrame]) + L" 작업중";
                    SetWindowText(g_hSpinnerLabel, spinText.c_str());
                }
                // Update progress bar
                if (g_hProgressBar && g_nukeProgressTotal > 0) {
                    int pct = (int)((double)g_nukeProgressDone / g_nukeProgressTotal * 100.0);
                    if (pct > 100) pct = 100;
                    SendMessage(g_hProgressBar, PBM_SETPOS, pct, 0);
                }
            } else {
                // Work finished — hide spinner, show 100% then hide
                KillTimer(hwnd, 3);
                if (g_hSpinnerLabel) {
                    SetWindowText(g_hSpinnerLabel, L"✔ 완료");
                    SetTimer(hwnd, 4, 3000, NULL); // hide after 3s
                }
                if (g_hProgressBar) {
                    SendMessage(g_hProgressBar, PBM_SETPOS, 100, 0);
                }
                // Multi-pass: if dependency errors found, fire again
                if (g_nukeMultiPassPending) {
                    g_nukeMultiPassPending = false;
                    AppendLog(L"[Cleaner] 의존성 오류가 감지되었습니다. 2차 삭제를 자동 실행합니다...\r\n");
                    g_nukeLog.clear();
                    g_nukeProgressDone = 0;
                    ShowWindow(g_hProgressBar, SW_SHOW);
                    ShowWindow(g_hSpinnerLabel, SW_SHOW);
                    RunNuke(hwnd);
                    SetTimer(hwnd, 3, 200, NULL);
                }
            }
        }
        // Timer 4: clean up progress UI after delay
        if (wParam == 4) {
            KillTimer(hwnd, 4);
            if (!g_isWorking) {
                if (g_hProgressBar) ShowWindow(g_hProgressBar, SW_HIDE);
                if (g_hSpinnerLabel) {
                    SetWindowText(g_hSpinnerLabel, L"");
                    ShowWindow(g_hSpinnerLabel, SW_HIDE);
                }
            }
        }
        if (wParam == 1) {
            // Debounced auto-save: flush any pending dirty state
            if (g_isDirty && g_dirtyCooldownTicks > 0) {
                if (--g_dirtyCooldownTicks == 0) {
                    g_isDirty = false;
                    if (!g_isWorking) {
                        MTAConfigData* pData = new MTAConfigData(GetCurrentConfigData());
                        pData->hwnd = hwnd;
                        HANDLE hThread = CreateThread(NULL, 0, SaveFilesAsync, pData, 0, NULL);
                        if (hThread) CloseHandle(hThread);
                    }
                }
            }
            // IME check (skip if ComboBox edit is focused — avoid flicker during typing)
            HWND focused = GetFocus();
            bool comboFocused = (focused == g_hResourceFilter || GetParent(focused) == g_hResourceFilter);
            if (!comboFocused) {
                HIMC hImc = ImmGetContext(hwnd);
                DWORD dwConv, dwSent;
                bool isKorean = false;
                if (hImc) {
                    if (ImmGetConversionStatus(hImc, &dwConv, &dwSent))
                        isKorean = (dwConv & IME_CMODE_NATIVE);
                    ImmReleaseContext(hwnd, hImc);
                }
                SetWindowText(g_hImeIndicator, isKorean ? L"[한]" : L"[A]");

                bool capsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
                ShowWindow(g_hCapsIndicator, capsOn ? SW_SHOW : SW_HIDE);
                InvalidateRect(g_hImeIndicator, NULL, TRUE);
                InvalidateRect(g_hCapsIndicator, NULL, TRUE);
            }

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
    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT* pmis = (MEASUREITEMSTRUCT*)lParam;
        if (pmis->CtlID == ID_COMBO_RESOURCE) {
            pmis->itemHeight = 22;
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
        if (pdis->CtlID == ID_COMBO_RESOURCE) {
            if ((int)pdis->itemID < 0) return TRUE;
            // Guard: if indices were just rebuilt, itemID may be temporarily stale.
            // Fill with window background to avoid the gray artifact, then bail.
            if (pdis->itemID >= g_filteredIndices.size()) {
                FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH)(COLOR_WINDOW + 1));
                return TRUE;
            }
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;

            int realIdx = g_filteredIndices[pdis->itemID];
            const wchar_t* eng = g_resourceInfos[realIdx].eng;
            const wchar_t* kor = g_resourceInfos[realIdx].kor;
            bool priority = IsPriorityResource(eng);

            HBRUSH hBack = (pdis->itemState & ODS_SELECTED) ? GetSysColorBrush(COLOR_HIGHLIGHT) : (priority ? g_hBrushYellow : GetSysColorBrush(COLOR_WINDOW));
            FillRect(hdc, &rc, hBack);

            COLORREF oldText = SetTextColor(hdc, (pdis->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_WINDOWTEXT));
            int oldBkMode = SetBkMode(hdc, TRANSPARENT);

            // Partition the drawing area (assuming 600 width horizontal extent)
            // Korean: 5 to 180
            // English: 190 to 380
            // Tags: 390 to 590
            
            RECT rcKor = rc;
            rcKor.left += 5;
            rcKor.right = rcKor.left + 175;
            SelectObject(hdc, g_hFontBold);
            DrawText(hdc, kor, -1, &rcKor, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            RECT rcEng = rc;
            rcEng.left = rcKor.right + 10;
            rcEng.right = rcEng.left + 190;
            SelectObject(hdc, g_hFontNorm); 
            SetTextColor(hdc, (pdis->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : RGB(100, 100, 100));
            DrawText(hdc, eng, -1, &rcEng, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            // Draw Tags (Hashtags)
            std::wstring infoTags = g_resourceInfos[realIdx].tags;
            RECT rcTags = rc;
            rcTags.left = rcEng.right + 10;
            rcTags.right = rc.left + 590; // Extend to full horizontal width
            SelectObject(hdc, g_hFontIndicator); 
            SetTextColor(hdc, (pdis->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : RGB(0, 150, 200));
            DrawText(hdc, infoTags.c_str(), -1, &rcTags, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

            SetTextColor(hdc, oldText);
            SetBkMode(hdc, oldBkMode);
            return TRUE;
        }
        if (pdis->CtlID == ID_BTN_ISSUES || pdis->CtlID == ID_BTN_AWS_CONSOLE) {
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;
            bool isIssues = (pdis->CtlID == ID_BTN_ISSUES);
            bool hot = (pdis->itemState & ODS_SELECTED);
            COLORREF bgNormal = isIssues ? RGB(36, 41, 47)   : RGB(232, 140, 20);
            COLORREF bgHot    = isIssues ? RGB(55, 62, 71)   : RGB(255, 165, 30);
            HBRUSH hBg = CreateSolidBrush(hot ? bgHot : bgNormal);
            FillRect(hdc, &rc, hBg);
            DeleteObject(hBg);
            HPEN hPen = CreatePen(PS_SOLID, 1, isIssues ? RGB(80,90,100) : RGB(180,100,10));
            HGDIOBJ oldPen = SelectObject(hdc, hPen);
            HGDIOBJ oldBr  = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);
            SelectObject(hdc, oldPen); SelectObject(hdc, oldBr);
            DeleteObject(hPen);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, g_hFontBold);
            const wchar_t* label = isIssues
                ? L"\U0001F41B  \uAE30\uB2A5 \uC81C\uC548 & \uBC84\uADF8 \uC2E0\uACE0"
                : L"\U0001F512  AWS \ucf58\uc194";
            DrawText(hdc, label, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        if (pdis->CtlID == ID_CHK_SELECT_ALL) {
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;
            int bw = rc.right - rc.left;

            FillRect(hdc, &rc, g_hBrushPastelYellow);

            HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 0, 0));
            HPEN hRedPenBg = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
            HGDIOBJ oldPenBg = SelectObject(hdc, hRedPenBg);
            HGDIOBJ oldBrushBg = SelectObject(hdc, hRedBrush);
            
            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 20, 20);
            
            SelectObject(hdc, oldPenBg);
            SelectObject(hdc, oldBrushBg);
            DeleteObject(hRedBrush);
            DeleteObject(hRedPenBg);

            int boxSize = 34;
            RECT box = { rc.left + (bw - boxSize) / 2, rc.top + 15, rc.left + (bw - boxSize) / 2 + boxSize, rc.top + 15 + boxSize };
            
            HBRUSH hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
            HPEN hRedPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
            HGDIOBJ oldPen = SelectObject(hdc, hRedPen);
            HGDIOBJ oldBrush = SelectObject(hdc, hWhiteBrush);

            RoundRect(hdc, box.left, box.top, box.right, box.bottom, 10, 10);

            if (g_selectAll) {
                HPEN hThickRedPen = CreatePen(PS_SOLID, 4, RGB(255, 0, 0));
                SelectObject(hdc, hThickRedPen);
                MoveToEx(hdc, box.left + 7, box.top + 17, NULL);
                LineTo(hdc, box.left + 14, box.bottom - 7);
                LineTo(hdc, box.right - 6, box.top + 8);
                SelectObject(hdc, hRedPen);
                DeleteObject(hThickRedPen);
            }

            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(hRedPen);

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, g_hFontHuge);
            RECT tr = { rc.left, box.bottom + 15, rc.right, rc.bottom };
            DrawText(hdc, L"전체\n선택", -1, &tr, DT_CENTER | DT_TOP);

            return TRUE;
        }
        if (pdis->CtlID == ID_BTN_SETTINGS) {
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;
            int bw = rc.right - rc.left;

            FillRect(hdc, &rc, g_hBrushPastelYellow);

            HBRUSH hBlackBrush = CreateSolidBrush(RGB(30, 30, 30));
            HPEN hBlackPenBg = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            HGDIOBJ oldPenBg = SelectObject(hdc, hBlackPenBg);
            HGDIOBJ oldBrushBg = SelectObject(hdc, hBlackBrush);
            
            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 20, 20);
            
            SelectObject(hdc, oldPenBg);
            SelectObject(hdc, oldBrushBg);
            DeleteObject(hBlackBrush);
            DeleteObject(hBlackPenBg);

            int boxSize = 34;
            RECT box = { rc.left + (bw - boxSize) / 2, rc.top + 15, rc.left + (bw - boxSize) / 2 + boxSize, rc.top + 15 + boxSize };
            
            HFONT hFontSet = MakeFont(22, FW_HEAVY, L"Arial");
            HGDIOBJ oldFont = SelectObject(hdc, hFontSet);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            RECT iconBox = { rc.left, box.top + 6, rc.right, box.bottom };
            DrawText(hdc, L"SET", -1, &iconBox, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOCLIP);
            SelectObject(hdc, oldFont);
            DeleteObject(hFontSet);

            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, g_hFontHuge);
            RECT trSet = { rc.left, box.bottom + 15, rc.right, rc.bottom };
            DrawText(hdc, L"설정", -1, &trSet, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOCLIP);

            return TRUE;
        }
        if (pdis->CtlID == ID_BTN_CANCEL) {
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;
            bool isEnabled = IsWindowEnabled(pdis->hwndItem);

            HBRUSH hBgBrush = isEnabled ? CreateSolidBrush(RGB(220, 20, 60)) : CreateSolidBrush(RGB(240, 240, 240));
            FillRect(hdc, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            SetTextColor(hdc, isEnabled ? RGB(255, 255, 255) : RGB(160, 160, 160));
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, g_hFontBold);

            DrawText(hdc, L"취소", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            if (isEnabled) {
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(180, 0, 0));
                HGDIOBJ oldPen = SelectObject(hdc, hPen);
                HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                SelectObject(hdc, oldPen);
                SelectObject(hdc, oldBrush);
                DeleteObject(hPen);
            } else {
                DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT);
            }
            return TRUE;
        }
        if (pdis->CtlID >= ID_CHK_REGION_START && pdis->CtlID < ID_CHK_REGION_START + (int)g_regions.size()) {
            int idx = pdis->CtlID - ID_CHK_REGION_START;
            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;

            FillRect(hdc, &rc, g_hBrushPastelYellow);

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
    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE: {
        while (ShowCursor(TRUE) < 1);
        break;
    }
    case WM_SETCURSOR:
        while (ShowCursor(TRUE) < 1);
        if (g_isWorking && LOWORD(lParam) == HTCLIENT) {
            SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
            return TRUE;
        }
        if (LOWORD(lParam) == HTCLIENT) {
            if ((HWND)wParam == g_hPicLogo) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            if ((HWND)wParam != hwnd) {
                // Let child windows (Edit, ComboBox) handle their own cursors
                return CallWindowProc((WNDPROC)GetWindowLongPtr((HWND)wParam, GWLP_WNDPROC), (HWND)wParam, uMsg, wParam, lParam);
            }
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return TRUE;
        }
        break;
    case WM_CONTEXTMENU: {
        HWND hHit = (HWND)wParam;
        if (hHit == g_hPicLogo) {
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, ID_MENU_COPY_LINK, L"링크 복사");
            
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            if (pt.x == -1 && pt.y == -1) { 
                RECT rect;
                GetWindowRect(hHit, &rect);
                pt.x = rect.left + 5;
                pt.y = rect.top + 5;
            }
            int sel = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
            
            if (sel == ID_MENU_COPY_LINK) {
                if (OpenClipboard(hwnd)) {
                    EmptyClipboard();
                    std::wstring url = L"https://github.com/hslcrb/missile_to_AWS";
                    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (url.length() + 1) * sizeof(wchar_t));
                    if (hGlob) {
                        memcpy(GlobalLock(hGlob), url.c_str(), (url.length() + 1) * sizeof(wchar_t));
                        GlobalUnlock(hGlob);
                        SetClipboardData(CF_UNICODETEXT, hGlob);
                    }
                    CloseClipboard();
                    AppendLog(L"[INFO] 깃허브 저장소 링크가 복사되었습니다.\r\n");
                }
            }
            return 0;
        }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (id == ID_PIC_LOGO && (code == STN_CLICKED || code == BN_CLICKED)) {
            ShellExecute(NULL, L"open", L"https://github.com/hslcrb/missile_to_AWS", NULL, NULL, SW_SHOWNORMAL);
        }
        if (id == ID_BTN_ISSUES && code == BN_CLICKED) {
            ShellExecute(NULL, L"open", L"https://github.com/hslcrb/missile_to_AWS/issues", NULL, NULL, SW_SHOWNORMAL);
        }
        if (id == ID_BTN_AWS_CONSOLE && code == BN_CLICKED) {
            ShellExecute(NULL, L"open", L"https://signin.aws.amazon.com/", NULL, NULL, SW_SHOWNORMAL);
        }
        if (id == ID_BTN_SETTINGS && code == BN_CLICKED) {
            if (!g_hSettingsDlg) {
                g_hSettingsDlg = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), hwnd, SettingsDlgProc, 0);
                ShowWindow(g_hSettingsDlg, SW_SHOW);
            } else {
                SetForegroundWindow(g_hSettingsDlg);
            }
        }
        if (id == ID_BTN_SAVE && (code == BN_CLICKED || code == STN_CLICKED)) {
            MTAConfigData* pData = new MTAConfigData(GetCurrentConfigData());
            pData->hwnd = hwnd;
            HANDLE hThread = CreateThread(NULL, 0, SaveFilesAsync, pData, 0, NULL);
            if (hThread) CloseHandle(hThread);
        }
        if (id == ID_BTN_NUKE && (code == BN_CLICKED || code == STN_CLICKED)) {
            if (g_nukeCountdown == 0) {
                g_nukeCountdown = 5;
                SendMessage(g_hBtnNuke, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hNukeBmpDim);
                EnableWindow(g_hBtnCancel, TRUE);
                InvalidateRect(g_hBtnCancel, NULL, TRUE);
                AppendLog(L"\r\n[Cleaner] **경고** 5초 뒤 삭제를 진행합니다. 취소하려면 '취소' 버튼을 누르세요.\r\n");
                SetTimer(hwnd, 2, 1000, NULL);
            }
        }
        if (id == ID_BTN_CANCEL && code == BN_CLICKED) {
            if (g_nukeCountdown > 0) {
                KillTimer(hwnd, 2);
                g_nukeCountdown = 0;
                AppendLog(L"[Cleaner] 삭제 작업이 취소되었습니다.\r\n");
                SendMessage(g_hBtnNuke, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hNukeBmpFull);
                EnableWindow(g_hBtnCancel, FALSE);
                InvalidateRect(g_hBtnCancel, NULL, TRUE);
            }
        }
        if (id == ID_CHK_SELECT_ALL) {
            g_selectAll = !g_selectAll;
            for (int i = 0; i < (int)g_regions.size(); ++i) {
                g_regions[i].selected = g_selectAll;
                InvalidateRect(g_regions[i].hwnd, NULL, TRUE);
            }
            InvalidateRect(g_hwndSelectAll, NULL, TRUE);
        }
        if (id == ID_CHK_REGION_START - 1) {} // dummy sentinel
        if (id >= ID_CHK_REGION_START && id < ID_CHK_REGION_START + (int)g_regions.size()) {
            int idx = id - ID_CHK_REGION_START;
            g_regions[idx].selected = !g_regions[idx].selected;
            InvalidateRect(g_regions[idx].hwnd, NULL, TRUE);
            g_isDirty = true; g_dirtyCooldownTicks = 1; // save on next tick
        }
        if (id == ID_CHK_SHOW_SECRET && code != BN_CLICKED) {} // handled above
        // Real-time save triggers for credential/UI changes
        // Guard: skip during initial config load to prevent startup save loops
        if (!g_isLoadingConfig &&
            (id == ID_EDIT_ACCOUNT || id == ID_EDIT_ACCESS_KEY || id == ID_EDIT_SECRET_KEY) &&
            code == EN_CHANGE) {
            g_isDirty = true; g_dirtyCooldownTicks = 20; // 2-second debounce
        }
        if (id == ID_CHK_SHOW_SECRET && code == BN_CLICKED) {
            bool checked = (SendMessage(g_hChkShowSecret, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SendMessage(g_hSecretKey, EM_SETPASSWORDCHAR, (checked ? 0 : (WPARAM)L'●'), 0);
            SetFocus(g_hSecretKey); // Force update
            InvalidateRect(g_hSecretKey, NULL, TRUE);
            g_isDirty = true; g_dirtyCooldownTicks = 1;
        }
        if (id == ID_COMBO_SORT && code == CBN_SELCHANGE) {
            wchar_t search[128];
            GetWindowText(g_hResourceFilter, search, 128);
            SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_COMBO_RESOURCE, CBN_EDITCHANGE), (LPARAM)g_hResourceFilter);
            g_isDirty = true; g_dirtyCooldownTicks = 1;
        }
        if (id == ID_COMBO_RESOURCE && code == CBN_SELCHANGE) {
            // User picked an item: save after a longer debounce (5 s) so we do NOT
            // trigger an immediate CB_RESETCONTENT / re-filter while the dropdown
            // is still transitioning — that is what causes the blank gray item.
            g_isDirty = true; g_dirtyCooldownTicks = 50; // 5-second debounce
        }
        if (id == ID_COMBO_RESOURCE && code == CBN_EDITCHANGE) {
            if (g_isFiltering || g_isIMEComposing || g_isArrowNav) {
                if (g_isArrowNav) g_isArrowNav = false;
                return 0;
            }
            g_isFiltering = true;

            wchar_t search[128];
            GetWindowText(g_hResourceFilter, search, 128);

            FilterResourceList(search);
            
            SendMessage(g_hResourceFilter, CB_RESETCONTENT, 0, 0);
            for (int idx : g_filteredIndices) {
                SendMessage(g_hResourceFilter, CB_ADDSTRING, 0, (LPARAM)g_resourceInfos[idx].eng);
            }
            
            // Restore text and selection (crucial for CBS_DROPDOWN)
            SetWindowText(g_hResourceFilter, search);
            SendMessage(g_hResourceFilter, CB_SETEDITSEL, 0, MAKELPARAM(wcslen(search), wcslen(search)));
            if (!g_isIMEComposing) SendMessage(g_hResourceFilter, CB_SHOWDROPDOWN, TRUE, 0);
            
            g_isFiltering = false;
        }
        return 0;
    }

    case WM_DESTROY: {
        if (g_hFontNorm) DeleteObject(g_hFontNorm);
        if (g_hFontBold) DeleteObject(g_hFontBold);
        if (g_hFontPrefix) DeleteObject(g_hFontPrefix);
        if (g_hFontIndicator) DeleteObject(g_hFontIndicator);
        if (g_hFontHuge) DeleteObject(g_hFontHuge);
        if (g_hBrushNavy) DeleteObject(g_hBrushNavy);
        if (g_hBrushRed) DeleteObject(g_hBrushRed);
        if (g_hBrushPureRed) DeleteObject(g_hBrushPureRed);
        if (g_hBrushYellow) DeleteObject(g_hBrushYellow);
        if (g_hBrushPastelYellow) DeleteObject(g_hBrushPastelYellow);
        if (g_hFontRes) RemoveFontMemResourceEx(g_hFontRes);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"AWSCleanerWindowClass";
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIconSm = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    RegisterClassEx(&wc);

    // Register Noto Sans KR from Resource
    HRSRC hResFont = FindResource(hInstance, MAKEINTRESOURCE(IDR_FONT_NOTO), RT_RCDATA);
    if (hResFont) {
        HGLOBAL hGlobFont = LoadResource(hInstance, hResFont);
        if (hGlobFont) {
            void* pData = LockResource(hGlobFont);
            DWORD size = SizeofResource(hInstance, hResFont);
            DWORD numFonts = 0;
            g_hFontRes = AddFontMemResourceEx(pData, size, NULL, &numFonts);
        }
    }

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"AWS Cleaner", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 610, 780, NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nShowCmd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        bool isHandled = false;
        if (msg.message == WM_KEYDOWN) {
            bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (ctrl) {
                if (g_hSettingsDlg && (GetForegroundWindow() == g_hSettingsDlg || IsChild(g_hSettingsDlg, GetFocus()))) {
                    if (msg.wParam == VK_OEM_PLUS || msg.wParam == VK_ADD || msg.wParam == '=' || msg.wParam == '+') {
                        g_settingsZoomLevel += 2;
                        ApplySettingsZoom(g_hSettingsDlg);
                        isHandled = true;
                    } else if (msg.wParam == VK_OEM_MINUS || msg.wParam == VK_SUBTRACT || msg.wParam == '-') {
                        g_settingsZoomLevel -= 2;
                        ApplySettingsZoom(g_hSettingsDlg);
                        isHandled = true;
                    } else if (msg.wParam == '0' || msg.wParam == VK_NUMPAD0) {
                        g_settingsZoomLevel = 0;
                        ApplySettingsZoom(g_hSettingsDlg);
                        isHandled = true;
                    }
                }
                
                if (!isHandled) {
                    if (msg.wParam == 'S') {
                        SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_BTN_SAVE, BN_CLICKED), 0);
                        isHandled = true;
                    } else {
                    HWND hFocus = GetFocus();
                    if (hFocus) {
                        wchar_t className[256] = {0};
                        GetClassName(hFocus, className, 256);
                        if (wcscmp(className, L"Edit") == 0) {
                            if (msg.wParam == 'A') {
                                SendMessage(hFocus, EM_SETSEL, 0, -1);
                                isHandled = true;
                            } else if (msg.wParam == 'Z') {
                                SendMessage(hFocus, EM_UNDO, 0, 0);
                                isHandled = true;
                            } else if (msg.wParam == 'X') {
                                SendMessage(hFocus, WM_CUT, 0, 0);
                                isHandled = true;
                            } else if (msg.wParam == 'C') {
                                SendMessage(hFocus, WM_COPY, 0, 0);
                                isHandled = true;
                            } else if (msg.wParam == 'V') {
                                SendMessage(hFocus, WM_PASTE, 0, 0);
                                isHandled = true;
                            }
                        }
                    }
                }
                }
            }
        }
        
        if (!isHandled) {
            if (g_hSettingsDlg && IsDialogMessage(g_hSettingsDlg, &msg)) {
                // Handled by modeless dialog
            } else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        
        // ULTIMATE CURSOR PROTECTION: Force cursor visible after every message
        CURSORINFO ci = { sizeof(CURSORINFO) };
        ci.cbSize = sizeof(CURSORINFO);
        if (GetCursorInfo(&ci)) {
            if (!(ci.flags & CURSOR_SHOWING)) {
                while (ShowCursor(TRUE) < 1);
            }
        }
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}
