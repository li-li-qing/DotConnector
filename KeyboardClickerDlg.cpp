#include "KeyboardClickerDlg.h"

#include "resource.h"

#include <algorithm>
#include <afxdlgs.h>
#include <cstring>
#include <memory>
#include <vector>

#import "third_party\\dm\\dm.dll" no_namespace named_guids \
    rename("CopyFile", "DmCopyFile") \
    rename("DeleteFile", "DmDeleteFile") \
    rename("FindWindow", "DmFindWindow") \
    rename("FindWindowEx", "DmFindWindowEx") \
    rename("GetCommandLine", "DmGetCommandLine") \
    rename("Int64ToInt32", "DmInt64ToInt32") \
    rename("MoveFile", "DmMoveFile") \
    rename("SetWindowText", "DmSetWindowText") \
    rename("StrStr", "DmStrStr")

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace
{
constexpr LPCTSTR SettingsSection = _T("Settings");
constexpr LPCTSTR DmDllFileName = _T("dm.dll");
constexpr LPCTSTR DmProjectDllPath = _T("third_party\\dm\\dm.dll");
constexpr LPCTSTR DmImageFolderName = _T("assets\\images");
constexpr LPCTSTR DmDictFolderName = _T("assets\\dicts");
constexpr LPCTSTR DmDefaultDictRelativePath = _T("assets\\dicts\\default.txt");
constexpr LPCWSTR DmRegistrationCode = L"9752236420d88364e6c355f4ca176d362e4d4833f";
constexpr LPCWSTR DmAdditionalCode = L"1886668989";
constexpr LPCTSTR AdvancedFeaturePassword = _T("1886668989");
constexpr int MinWindowWidth = 820;
constexpr int MinWindowHeight = 840;
constexpr int LayoutMargin = 12;
constexpr int LayoutGap = 10;
constexpr int TargetGroupHeight = 70;
constexpr int StatusHeight = 24;
constexpr int MinClickGroupHeight = 150;
constexpr int MinShoutGroupHeight = 170;
constexpr int MinDebugPanelHeight = 150;
constexpr int MaxDebugPanelHeight = 420;
constexpr int ScrollBarWidth = 16;
constexpr int ClickHeaderOffsetY = 42;
constexpr int ClickRowsOffsetY = 62;
constexpr int ClickFooterReservedHeight = 44;
constexpr int ClickDisplayRowHeight = 24;
constexpr int ShoutHeaderOffsetY = 42;
constexpr int ShoutRowsOffsetY = 62;
constexpr int ShoutDisplayRowHeight = 32;
constexpr int ClickControlBaseLocal = 2000;
constexpr int ClickControlStride = 10;
constexpr int ClickRowY = 128;
constexpr int ClickRowHeight = 24;
constexpr int ActionControlBaseLocal = 3000;
constexpr int ActionControlStride = 10;
constexpr int ActionRowY = 336;
constexpr int ActionRowHeight = 34;
constexpr UINT_PTR HornTimerId = 4000;
constexpr int AdvancedControlBase = 5000;
constexpr int IDC_HORN_ENABLED = AdvancedControlBase + 1;
constexpr int IDC_HORN_INTERVAL = AdvancedControlBase + 2;
constexpr int IDC_HORN_SKILL_KEY = AdvancedControlBase + 3;
constexpr int IDC_SELECT_DEBUFF_REGION = AdvancedControlBase + 4;
constexpr int IDC_SELECT_HORN_REGION = AdvancedControlBase + 5;
constexpr int IDC_DEBUFF_OPTIONS = AdvancedControlBase + 6;
constexpr int IDC_HORN_SIMILARITY = AdvancedControlBase + 7;
constexpr int IDC_ADD_HORN_SAMPLE = AdvancedControlBase + 8;
constexpr int IDC_TEST_HORN = AdvancedControlBase + 9;
constexpr int IDC_SKILL_MONITOR_TOGGLE = AdvancedControlBase + 10;
constexpr int IDC_DEBUFF_CHECK_BASE = AdvancedControlBase + 20;
constexpr int IDC_DEBUFF_SAMPLE_BASE = AdvancedControlBase + 40;
constexpr int IDC_SKILL_REGION_BASE = AdvancedControlBase + 60;
constexpr int IDC_SKILL_SAMPLE_BASE = AdvancedControlBase + 70;
constexpr int IDC_SKILL_DETECT_BASE = AdvancedControlBase + 80;
constexpr int IDC_SKILL_KEY_BASE = AdvancedControlBase + 90;
constexpr int IDC_SKILL_DEBUFF_BASE = AdvancedControlBase + 110;
constexpr int IDC_SKILL_PRIORITY_BASE = AdvancedControlBase + 150;
constexpr int IDC_SKILL_ENABLED_BASE = AdvancedControlBase + 160;
constexpr int IDC_SKILL_INTERVAL_BASE = AdvancedControlBase + 170;
constexpr int IDC_SKILL_DETECT_INTERVAL_BASE = AdvancedControlBase + 180;
constexpr int AdvancedTabHeight = 28;
constexpr int AdvancedHintHeight = 24;
constexpr int MaxDebugTextLength = 24000;
constexpr int DebugTrimTargetLength = 16000;
constexpr int MaxDebugLineLength = 900;
constexpr UINT WM_APP_ASYNC_STATUS = WM_APP + 101;
constexpr UINT WM_APP_SKILL_WORKER_FINISHED = WM_APP + 102;
constexpr UINT WM_APP_SHOUT_WORKER_FINISHED = WM_APP + 103;
constexpr double DmFindPicSimilarity = 0.9;
constexpr LPCTSTR DmFindPicDeltaColor = _T("000000");
constexpr LPCTSTR HornPicturePath = _T("assets\\images\\horn.bmp");
constexpr LPCTSTR HornRootPicturePattern = _T("horn*.bmp");
constexpr LPCTSTR HornSampleDirectory = _T("assets\\images\\horn");
constexpr ULONGLONG HornPictureRefreshMs = 1000;
constexpr UINT DefaultSkillDetectIntervalMs = 50;
constexpr UINT DefaultSkillReleaseIntervalMs = 500;
constexpr UINT MinSkillDetectIntervalMs = 20;
constexpr UINT MinSkillReleaseIntervalMs = 50;
constexpr ULONGLONG SkillAvailabilityCacheMs = 3000;

class CAdvancedPasswordDialog : public CDialogEx
{
public:
    explicit CAdvancedPasswordDialog(CWnd* parent)
        : CDialogEx(IDD_ADVANCED_PASSWORD_DIALOG, parent)
    {
    }

    const CString& Password() const
    {
        return m_password;
    }

protected:
    BOOL OnInitDialog() override
    {
        CDialogEx::OnInitDialog();
        if (CWnd* edit = GetDlgItem(IDC_ADVANCED_PASSWORD_EDIT))
        {
            edit->SetFocus();
            return FALSE;
        }
        return TRUE;
    }

    void OnOK() override
    {
        GetDlgItemText(IDC_ADVANCED_PASSWORD_EDIT, m_password);
        m_password.Trim();
        if (m_password.IsEmpty())
        {
            AfxMessageBox(_T("请输入高级功能密码。"), MB_ICONINFORMATION);
            return;
        }
        CDialogEx::OnOK();
    }

private:
    CString m_password;
};

struct AdvancedSkillDefinition
{
    LPCTSTR label;
    LPCTSTR settingsPrefix;
    LPCTSTR rootPicturePattern;
    LPCTSTR sampleDirectory;
};

constexpr AdvancedSkillDefinition AdvancedSkillDefinitions[] = {
    { _T("号角"), _T("Horn"), _T("horn*.bmp"), _T("assets\\images\\horn") },
    { _T("生命值涌现"), _T("LifeSurge"), _T("life_surge*.bmp"), _T("assets\\images\\life_surge") },
    { _T("挣脱枷锁"), _T("BreakShackles"), _T("break_shackles*.bmp"), _T("assets\\images\\break_shackles") },
    { _T("霸气"), _T("Domineering"), _T("domineering*.bmp"), _T("assets\\images\\domineering") },
};

constexpr bool DefaultSkillDebuffs[][4] = {
    { true, false, false, false },
    { true, true, true, true },
    { true, false, false, false },
    { true, true, true, true },
};

constexpr UINT DefaultSkillPriorities[] = {
    2, 3, 1, 4
};

struct DebuffDefinition
{
    LPCTSTR label;
    LPCTSTR picture;
    LPCTSTR sampleDirectory;
};

constexpr DebuffDefinition DebuffDefinitions[] = {
    { _T("束缚(tie.bmp)"), _T("assets\\images\\tie.bmp"), _T("assets\\images\\tie") },
    { _T("眩晕(dizziness.bmp)"), _T("assets\\images\\dizziness.bmp"), _T("assets\\images\\dizziness") },
    { _T("枪刺(bayonet.bmp)"), _T("assets\\images\\bayonet.bmp"), _T("assets\\images\\bayonet") },
    { _T("倒地(collapse.bmp)"), _T("assets\\images\\collapse.bmp"), _T("assets\\images\\collapse") },
};

struct AsyncStatusMessage
{
    CString text;
};

struct SkillDetectionItem
{
    bool enabled = false;
    size_t index = 0;
    UINT skillVk = 0;
    UINT priority = 0;
    CRect skillRegion;
    CString skillPictures;
    CString debuffPictures;
    bool cachedSkillAvailable = false;
    ULONGLONG cachedSkillAvailableTick = 0;
};

struct SkillAvailabilityUpdate
{
    size_t index = 0;
    bool available = false;
    ULONGLONG tick = 0;
};

struct SkillDetectionTask
{
    HWND targetHwnd = nullptr;
    CString dmDllPath;
    CString resourceRoot;
    double similarity = 0.9;
    bool respectIntervals = false;
    bool saveSettings = false;
    bool actionLockedUntilDebuffClears = false;
    bool fastAutoDispatch = false;
    ULONGLONG dispatchTick = 0;
    size_t availabilityProbeIndex = 0;
    CString combinedDebuffPictures;
    std::vector<SkillDetectionItem> items;
    CRect debuffRegion;
};

struct SkillDetectionResult
{
    bool success = false;
    bool clearActionLock = false;
    bool setActionLock = false;
    bool respectIntervals = false;
    bool saveSettings = false;
    bool sawMatchingDebuff = false;
    bool inputAlreadySent = false;
    bool inputFailed = false;
    size_t index = 0;
    UINT skillVk = 0;
    UINT priority = 0;
    CString message;
    std::vector<CString> diagnostics;
    std::vector<SkillAvailabilityUpdate> availabilityUpdates;
};

struct ShoutWorkerTask
{
    HWND ownerHwnd = nullptr;
    HWND targetHwnd = nullptr;
    CString text;
    UINT reportDelayMs = 0;
    UINT startDelayMs = 0;
    CString sequence;
    CString oldClipboardText;
    bool hadClipboardText = false;
};

struct ShoutWorkerResult
{
    bool success = false;
    CString message;
};

using DllGetClassObjectProc = HRESULT(WINAPI*)(REFCLSID clsid, REFIID iid, void** object);

class CRegionSelectWnd : public CWnd
{
public:
    bool Select(CRect& region)
    {
        m_region.SetRectEmpty();
        m_dragging = false;
        m_done = false;
        m_cancelled = false;

        const int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        const int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        const int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        const int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        m_screenRect.SetRect(x, y, x + w, y + h);

        CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, LoadCursor(nullptr, IDC_CROSS), reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH)), nullptr);
        if (!CreateEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED, className, _T("Select Region"), WS_POPUP, m_screenRect, nullptr, 0))
        {
            return false;
        }

        SetLayeredWindowAttributes(0, 80, LWA_ALPHA);
        ShowWindow(SW_SHOW);
        UpdateWindow();
        SetCapture();

        MSG msg = {};
        BOOL messageResult = 0;
        while (!m_done && (messageResult = GetMessage(&msg, nullptr, 0, 0)) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (messageResult == 0)
        {
            m_cancelled = true;
            PostQuitMessage(static_cast<int>(msg.wParam));
        }
        else if (messageResult == -1)
        {
            m_cancelled = true;
        }

        if (GetCapture() == this)
        {
            ReleaseCapture();
        }
        if (GetSafeHwnd())
        {
            DestroyWindow();
        }

        if (m_cancelled || m_region.Width() < 3 || m_region.Height() < 3)
        {
            return false;
        }

        region = m_region;
        return true;
    }

protected:
    afx_msg void OnPaint()
    {
        CPaintDC dc(this);
        CRect client;
        GetClientRect(&client);
        dc.FillSolidRect(client, RGB(0, 0, 0));

        if (!m_region.IsRectEmpty())
        {
            CRect local = m_region;
            local.OffsetRect(-m_screenRect.left, -m_screenRect.top);
            CPen pen(PS_SOLID, 2, RGB(0, 180, 255));
            CBrush* oldBrush = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));
            CPen* oldPen = dc.SelectObject(&pen);
            dc.Rectangle(local);
            dc.SelectObject(oldPen);
            dc.SelectObject(oldBrush);
        }
    }

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point)
    {
        UNREFERENCED_PARAMETER(nFlags);
        m_dragging = true;
        m_start = point + m_screenRect.TopLeft();
        m_region.SetRect(m_start, m_start);
        Invalidate(FALSE);
    }

    afx_msg void OnMouseMove(UINT nFlags, CPoint point)
    {
        UNREFERENCED_PARAMETER(nFlags);
        if (!m_dragging)
        {
            return;
        }

        const CPoint current = point + m_screenRect.TopLeft();
        m_region.SetRect((std::min)(m_start.x, current.x), (std::min)(m_start.y, current.y), (std::max)(m_start.x, current.x), (std::max)(m_start.y, current.y));
        Invalidate(FALSE);
    }

    afx_msg void OnLButtonUp(UINT nFlags, CPoint point)
    {
        UNREFERENCED_PARAMETER(nFlags);
        if (m_dragging)
        {
            const CPoint current = point + m_screenRect.TopLeft();
            m_region.SetRect((std::min)(m_start.x, current.x), (std::min)(m_start.y, current.y), (std::max)(m_start.x, current.x), (std::max)(m_start.y, current.y));
        }
        m_dragging = false;
        m_done = true;
    }

    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
    {
        UNREFERENCED_PARAMETER(nRepCnt);
        UNREFERENCED_PARAMETER(nFlags);
        if (nChar == VK_ESCAPE)
        {
            m_cancelled = true;
            m_done = true;
        }
    }

    DECLARE_MESSAGE_MAP()

private:
    bool m_dragging = false;
    bool m_done = false;
    bool m_cancelled = false;
    CPoint m_start;
    CRect m_region;
    CRect m_screenRect;
};

BEGIN_MESSAGE_MAP(CRegionSelectWnd, CWnd)
    ON_WM_PAINT()
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_KEYDOWN()
END_MESSAGE_MAP()

int ClickControlId(size_t index, int offset)
{
    return ClickControlBaseLocal + static_cast<int>(index) * ClickControlStride + offset;
}

int ActionControlId(size_t index, int offset)
{
    return ActionControlBaseLocal + static_cast<int>(index) * ActionControlStride + offset;
}

bool FileExists(LPCTSTR path)
{
    const DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool DirectoryExists(LPCTSTR path)
{
    const DWORD attributes = GetFileAttributes(path);
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

CString NormalizeDirectoryPath(const CString& path)
{
    CString normalized(path);
    normalized.Trim();
    normalized.Replace(_T("/"), _T("\\"));
    while (normalized.GetLength() > 3 && normalized[normalized.GetLength() - 1] == L'\\')
    {
        normalized.Delete(normalized.GetLength() - 1);
    }
    return normalized;
}

CString GetParentDirectory(const CString& path)
{
    const CString normalized = NormalizeDirectoryPath(path);
    const int slash = normalized.ReverseFind(L'\\');
    if (slash <= 0)
    {
        return CString();
    }

    CString parent = normalized.Left(slash);
    if (parent.GetLength() == 2 && parent[1] == L':')
    {
        parent += _T("\\");
    }
    return parent;
}

bool EnsureDirectoryExists(LPCTSTR path)
{
    const CString normalized = NormalizeDirectoryPath(path);
    if (normalized.IsEmpty())
    {
        return false;
    }

    if (DirectoryExists(normalized))
    {
        return true;
    }

    const CString parent = GetParentDirectory(normalized);
    if (!parent.IsEmpty() && parent != normalized && !DirectoryExists(parent))
    {
        if (!EnsureDirectoryExists(parent))
        {
            return false;
        }
    }

    return CreateDirectory(normalized, nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool EnsureFileExists(LPCTSTR path)
{
    if (FileExists(path))
    {
        return true;
    }

    const CString parent = GetParentDirectory(path);
    if (!parent.IsEmpty() && !EnsureDirectoryExists(parent))
    {
        return false;
    }

    HANDLE file = CreateFile(path, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return GetLastError() == ERROR_FILE_EXISTS;
    }

    CloseHandle(file);
    return true;
}

bool IsBitmapFileExtension(const CString& extension)
{
    CString normalized(extension);
    normalized.Trim();
    normalized.MakeLower();
    return normalized == _T(".bmp");
}

CString CombinePath(const CString& directory, LPCTSTR child)
{
    CString path(directory);
    if (!path.IsEmpty() && path[path.GetLength() - 1] != L'\\')
    {
        path += _T("\\");
    }
    path += child;
    return path;
}

CString GetExecutableDirectory()
{
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, path, static_cast<DWORD>(_countof(path)));
    if (length == 0 || length >= _countof(path))
    {
        return CString();
    }

    CString directory(path);
    const int slash = directory.ReverseFind(L'\\');
    if (slash < 0)
    {
        return CString();
    }

    return directory.Left(slash + 1);
}

CString GetCurrentDirectoryPath()
{
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetCurrentDirectoryW(static_cast<DWORD>(_countof(path)), path);
    if (length == 0 || length >= _countof(path))
    {
        return CString();
    }

    CString directory(path);
    if (!directory.IsEmpty() && directory[directory.GetLength() - 1] != L'\\')
    {
        directory += _T("\\");
    }
    return directory;
}

CString ResolveDmDllPath()
{
    CString path = GetExecutableDirectory() + DmDllFileName;
    if (FileExists(path))
    {
        return path;
    }

    path = GetCurrentDirectoryPath() + DmProjectDllPath;
    if (FileExists(path))
    {
        return path;
    }

    if (FileExists(DmProjectDllPath))
    {
        return CString(DmProjectDllPath);
    }

    return CString();
}

CString ResolveDmResourceRoot()
{
    CString current = GetCurrentDirectoryPath();
    if (!current.IsEmpty() && DirectoryExists(CombinePath(current, DmImageFolderName)))
    {
        return current;
    }

    CString root = GetExecutableDirectory();
    if (!root.IsEmpty())
    {
        return root;
    }

    return current;
}

int CountPictureEntries(const CString& pictures)
{
    int count = 0;
    CString remaining(pictures);
    int position = 0;
    while (position != -1)
    {
        CString token = remaining.Tokenize(_T("|"), position);
        token.Trim();
        if (!token.IsEmpty())
        {
            ++count;
        }
    }
    return count;
}

void AppendPictureList(CString& destination, const CString& source)
{
    CString remaining(source);
    int position = 0;
    while (position != -1)
    {
        CString token = remaining.Tokenize(_T("|"), position);
        token.Trim();
        if (token.IsEmpty())
        {
            continue;
        }
        if (!destination.IsEmpty())
        {
            destination += _T("|");
        }
        destination += token;
    }
}

CString CompactForDebugLine(const CString& text, int maxChars = MaxDebugLineLength)
{
    CString compact(text);
    compact.Replace(_T("\r"), _T(" "));
    compact.Replace(_T("\n"), _T(" "));
    compact.Trim();
    if (compact.GetLength() > maxChars)
    {
        compact = compact.Left((std::max)(0, maxChars - 3)) + _T("...");
    }
    return compact;
}

bool WorkerIsExtendedKey(UINT vk)
{
    switch (vk)
    {
    case VK_RMENU:
    case VK_RCONTROL:
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_CANCEL:
    case VK_SNAPSHOT:
    case VK_DIVIDE:
        return true;
    default:
        return false;
    }
}

bool WorkerPrepareForegroundTarget(HWND targetHwnd)
{
    if (!IsWindow(targetHwnd))
    {
        return false;
    }

    HWND root = ::GetAncestor(targetHwnd, GA_ROOT);
    if (!IsWindow(root))
    {
        root = targetHwnd;
    }

    if (::IsIconic(root))
    {
        ::ShowWindow(root, SW_RESTORE);
    }

    HWND foregroundRoot = ::GetForegroundWindow();
    if (IsWindow(foregroundRoot))
    {
        foregroundRoot = ::GetAncestor(foregroundRoot, GA_ROOT);
    }

    if (foregroundRoot != root)
    {
        ::SetForegroundWindow(root);
        Sleep(40);
    }
    return true;
}

bool WorkerSendInputKey(UINT vk)
{
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(vk);
    inputs[0].ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
    inputs[0].ki.dwFlags = WorkerIsExtendedKey(vk) ? KEYEVENTF_EXTENDEDKEY : 0;

    inputs[1] = inputs[0];
    inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;

    const UINT inputCount = static_cast<UINT>(_countof(inputs));
    if (::SendInput(inputCount, inputs, sizeof(INPUT)) != inputCount)
    {
        return false;
    }

    Sleep(10);
    return true;
}

bool WorkerSendInputShortcut(UINT modifierVk, UINT vk)
{
    INPUT inputs[4] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(modifierVk);
    inputs[0].ki.wScan = static_cast<WORD>(MapVirtualKey(modifierVk, MAPVK_VK_TO_VSC));
    inputs[0].ki.dwFlags = WorkerIsExtendedKey(modifierVk) ? KEYEVENTF_EXTENDEDKEY : 0;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = static_cast<WORD>(vk);
    inputs[1].ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
    inputs[1].ki.dwFlags = WorkerIsExtendedKey(vk) ? KEYEVENTF_EXTENDEDKEY : 0;

    inputs[2] = inputs[1];
    inputs[2].ki.dwFlags |= KEYEVENTF_KEYUP;

    inputs[3] = inputs[0];
    inputs[3].ki.dwFlags |= KEYEVENTF_KEYUP;

    const UINT inputCount = static_cast<UINT>(_countof(inputs));
    return ::SendInput(inputCount, inputs, sizeof(INPUT)) == inputCount;
}

bool WorkerSendMouseWheel(int delta)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = static_cast<DWORD>(delta);
    return ::SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool WorkerReadClipboardText(HWND ownerHwnd, CString& text)
{
    text.Empty();
    if (!::OpenClipboard(ownerHwnd))
    {
        return false;
    }

    HANDLE data = ::GetClipboardData(CF_UNICODETEXT);
    if (data == nullptr)
    {
        ::CloseClipboard();
        return false;
    }

    const wchar_t* value = static_cast<const wchar_t*>(::GlobalLock(data));
    if (value == nullptr)
    {
        ::CloseClipboard();
        return false;
    }

    text = value;
    ::GlobalUnlock(data);
    ::CloseClipboard();
    return true;
}

bool WorkerSetClipboardText(HWND ownerHwnd, const CString& text)
{
    if (!::OpenClipboard(ownerHwnd))
    {
        return false;
    }

    ::EmptyClipboard();

    const SIZE_T byteCount = static_cast<SIZE_T>(text.GetLength() + 1) * sizeof(wchar_t);
    HGLOBAL data = ::GlobalAlloc(GMEM_MOVEABLE, byteCount);
    if (data == nullptr)
    {
        ::CloseClipboard();
        return false;
    }

    void* buffer = ::GlobalLock(data);
    if (buffer == nullptr)
    {
        ::GlobalFree(data);
        ::CloseClipboard();
        return false;
    }

    memcpy(buffer, static_cast<LPCWSTR>(text), byteCount);
    ::GlobalUnlock(data);

    if (::SetClipboardData(CF_UNICODETEXT, data) == nullptr)
    {
        ::GlobalFree(data);
        ::CloseClipboard();
        return false;
    }

    ::CloseClipboard();
    return true;
}

bool WorkerSetAndVerifyClipboardText(HWND ownerHwnd, const CString& text)
{
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        if (!WorkerSetClipboardText(ownerHwnd, text))
        {
            Sleep(20);
            continue;
        }

        Sleep(30);

        CString verifiedText;
        if (WorkerReadClipboardText(ownerHwnd, verifiedText) && verifiedText == text)
        {
            return true;
        }

        Sleep(30);
    }
    return false;
}

void WorkerClearClipboard(HWND ownerHwnd)
{
    if (::OpenClipboard(ownerHwnd))
    {
        ::EmptyClipboard();
        ::CloseClipboard();
    }
}

bool WorkerTryParseVirtualKey(const CString& token, UINT& vk)
{
    CString normalized(token);
    normalized.Trim();
    normalized.MakeUpper();

    if (normalized.GetLength() == 1)
    {
        const wchar_t ch = normalized[0];
        if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9'))
        {
            vk = static_cast<UINT>(ch);
            return true;
        }
    }

    if (normalized.Left(1) == _T("F"))
    {
        wchar_t* end = nullptr;
        const unsigned long fIndex = wcstoul(static_cast<LPCWSTR>(normalized) + 1, &end, 10);
        if (end != static_cast<LPCWSTR>(normalized) + 1 && *end == L'\0' && fIndex >= 1 && fIndex <= 24)
        {
            vk = VK_F1 + static_cast<UINT>(fIndex - 1);
            return true;
        }
    }

    struct NamedKey
    {
        LPCTSTR name;
        UINT vk;
    };

    static const NamedKey namedKeys[] = {
        { _T("ESC"), VK_ESCAPE }, { _T("ESCAPE"), VK_ESCAPE }, { _T("TAB"), VK_TAB },
        { _T("ENTER"), VK_RETURN }, { _T("RETURN"), VK_RETURN }, { _T("SPACE"), VK_SPACE },
        { _T("CTRL"), VK_CONTROL }, { _T("CONTROL"), VK_CONTROL }, { _T("SHIFT"), VK_SHIFT },
        { _T("ALT"), VK_MENU }, { _T("LEFT"), VK_LEFT }, { _T("RIGHT"), VK_RIGHT },
        { _T("UP"), VK_UP }, { _T("DOWN"), VK_DOWN }, { _T("HOME"), VK_HOME },
        { _T("END"), VK_END }, { _T("PGUP"), VK_PRIOR }, { _T("PAGEUP"), VK_PRIOR },
        { _T("PGDN"), VK_NEXT }, { _T("PAGEDOWN"), VK_NEXT }, { _T("INSERT"), VK_INSERT },
        { _T("INS"), VK_INSERT }, { _T("DELETE"), VK_DELETE }, { _T("DEL"), VK_DELETE }
    };

    for (const NamedKey& key : namedKeys)
    {
        if (normalized == key.name)
        {
            vk = key.vk;
            return true;
        }
    }

    return false;
}

bool WorkerExecuteSequenceToken(const CString& token, CString& error)
{
    CString normalized(token);
    normalized.Trim();
    normalized.MakeUpper();

    wchar_t* end = nullptr;
    const unsigned long delayMs = wcstoul(normalized, &end, 10);
    if (end != static_cast<LPCWSTR>(normalized) && *end == L'\0')
    {
        if (delayMs > 3000)
        {
            error = _T("序列中的等待时间不能超过 3000 毫秒。");
            return false;
        }
        Sleep(static_cast<DWORD>(delayMs));
        return true;
    }

    if (normalized == _T("WHEELUP") || normalized == _T("WHEEL_UP") || normalized == _T("SCROLLUP"))
    {
        return WorkerSendMouseWheel(WHEEL_DELTA);
    }
    if (normalized == _T("WHEELDOWN") || normalized == _T("WHEEL_DOWN") || normalized == _T("SCROLLDOWN"))
    {
        return WorkerSendMouseWheel(-WHEEL_DELTA);
    }

    UINT vk = 0;
    if (!WorkerTryParseVirtualKey(normalized, vk))
    {
        error.Format(_T("无法识别序列项: %s"), static_cast<LPCTSTR>(token));
        return false;
    }

    if (!WorkerSendInputKey(vk))
    {
        error = _T("发送序列按键失败。");
        return false;
    }
    return true;
}

bool WorkerDmFindPicEx(IDispatch* dmObject, const CRect& region, const CString& pictures, double similarity, CString& result, CString& error)
{
    result.Empty();
    error.Empty();
    if (dmObject == nullptr || pictures.IsEmpty())
    {
        return false;
    }

    try
    {
        _bstr_t raw = static_cast<Idmsoft*>(dmObject)->FindPicEx(region.left, region.top, region.right, region.bottom,
            _bstr_t(pictures), _bstr_t(DmFindPicDeltaColor), similarity, 0);
        const wchar_t* value = static_cast<const wchar_t*>(raw);
        if (value != nullptr)
        {
            result = value;
        }
    }
    catch (const _com_error& comError)
    {
        error.Format(_T("FindPicEx 调用失败，HRESULT=0x%08X"), static_cast<unsigned>(comError.Error()));
        return false;
    }

    result.Trim();
    return !result.IsEmpty() && result.Left(2) != _T("-1");
}

bool CreateWorkerDmObject(const CString& dmDllPath, const CString& resourceRoot, HMODULE& dmModule, IDispatch*& dmObject, CString& error)
{
    dmModule = nullptr;
    dmObject = nullptr;

    dmModule = LoadLibraryW(dmDllPath);
    if (dmModule == nullptr)
    {
        error.Format(_T("后台加载 dm.dll 失败，错误码: %lu"), GetLastError());
        return false;
    }

    auto getClassObject = reinterpret_cast<DllGetClassObjectProc>(GetProcAddress(dmModule, "DllGetClassObject"));
    if (getClassObject == nullptr)
    {
        error = _T("后台 dm.dll 中未找到 DllGetClassObject。");
        return false;
    }

    IClassFactory* factory = nullptr;
    HRESULT hr = getClassObject(__uuidof(dmsoft), IID_IClassFactory, reinterpret_cast<void**>(&factory));
    if (FAILED(hr) || factory == nullptr)
    {
        error.Format(_T("后台获取 dmsoft 类工厂失败，HRESULT=0x%08X"), static_cast<unsigned>(hr));
        return false;
    }

    hr = factory->CreateInstance(nullptr, __uuidof(Idmsoft), reinterpret_cast<void**>(&dmObject));
    factory->Release();
    if (FAILED(hr) || dmObject == nullptr)
    {
        error.Format(_T("后台创建 dmsoft 对象失败，HRESULT=0x%08X"), static_cast<unsigned>(hr));
        return false;
    }

    try
    {
        const long regResult = static_cast<Idmsoft*>(dmObject)->Reg(_bstr_t(DmRegistrationCode), _bstr_t(DmAdditionalCode));
        if (regResult != 1)
        {
            error.Format(_T("后台 Reg 返回 %ld。"), regResult);
            return false;
        }

        const long setPathResult = static_cast<Idmsoft*>(dmObject)->SetPath(_bstr_t(resourceRoot));
        if (setPathResult != 1)
        {
            error.Format(_T("后台 SetPath 返回 %ld。"), setPathResult);
            return false;
        }

        const long setDictResult = static_cast<Idmsoft*>(dmObject)->SetDict(0, _bstr_t(DmDefaultDictRelativePath));
        if (setDictResult != 1)
        {
            error.Format(_T("后台 SetDict 返回 %ld。"), setDictResult);
            return false;
        }
    }
    catch (const _com_error& comError)
    {
        error.Format(_T("后台初始化 dmsoft 失败，HRESULT=0x%08X"), static_cast<unsigned>(comError.Error()));
        return false;
    }

    return true;
}

void ReleaseWorkerDmObject(HMODULE dmModule, IDispatch* dmObject)
{
    if (dmObject != nullptr)
    {
        dmObject->Release();
    }
    if (dmModule != nullptr)
    {
        FreeLibrary(dmModule);
    }
}

int ClampInt(int value, int minValue, int maxValue)
{
    return (std::max)(minValue, (std::min)(value, maxValue));
}

struct BaseLayoutMetrics
{
    int contentTop = 0;
    int targetY = 0;
    int targetH = 0;
    int clickY = 0;
    int clickH = 0;
    int shoutY = 0;
    int shoutH = 0;
    int statusY = 0;
    int statusH = 0;
    int debugY = 0;
    int debugH = 0;
};

int CalculateDebugPanelHeight(const CRect& client)
{
    return ClampInt(client.Height() / 3, MinDebugPanelHeight, MaxDebugPanelHeight);
}

CString MakeSingleLineStatusText(const CString& text)
{
    CString displayText(text);
    displayText.Replace(_T("\r"), _T(" "));
    displayText.Replace(_T("\n"), _T(" "));
    displayText.Trim();

    constexpr int MaxStatusDisplayChars = 120;
    if (displayText.GetLength() > MaxStatusDisplayChars)
    {
        displayText = displayText.Left(MaxStatusDisplayChars - 3) + _T("...");
    }

    return displayText;
}

BaseLayoutMetrics CalculateBaseLayoutMetrics(const CRect& client)
{
    BaseLayoutMetrics metrics;
    metrics.contentTop = LayoutMargin + AdvancedTabHeight + LayoutGap;
    metrics.targetY = metrics.contentTop;
    metrics.targetH = TargetGroupHeight;
    metrics.statusH = StatusHeight;
    metrics.clickY = metrics.targetY + metrics.targetH + LayoutGap;

    const int minActionHeight = MinClickGroupHeight + LayoutGap + MinShoutGroupHeight;
    const int maxDebugHeight = (std::max)(MinDebugPanelHeight,
        client.Height() - LayoutMargin - metrics.clickY - minActionHeight - metrics.statusH - LayoutGap * 2);
    metrics.debugH = (std::min)(CalculateDebugPanelHeight(client), maxDebugHeight);
    metrics.debugY = client.Height() - LayoutMargin - metrics.debugH;
    metrics.statusY = metrics.debugY - LayoutGap - metrics.statusH;

    int actionHeight = metrics.statusY - LayoutGap - metrics.clickY;
    actionHeight = (std::max)(minActionHeight, actionHeight);

    metrics.clickH = (std::max)(MinClickGroupHeight, actionHeight * 42 / 100);
    metrics.shoutH = actionHeight - metrics.clickH - LayoutGap;
    if (metrics.shoutH < MinShoutGroupHeight)
    {
        metrics.shoutH = MinShoutGroupHeight;
        metrics.clickH = (std::max)(MinClickGroupHeight, actionHeight - metrics.shoutH - LayoutGap);
    }

    metrics.shoutY = metrics.clickY + metrics.clickH + LayoutGap;
    if (metrics.shoutY + metrics.shoutH + LayoutGap > metrics.statusY)
    {
        metrics.shoutH = (std::max)(MinShoutGroupHeight, metrics.statusY - LayoutGap - metrics.shoutY);
    }

    return metrics;
}
}

BEGIN_MESSAGE_MAP(CKeyboardClickerDlg, CDialogEx)
    ON_WM_DESTROY()
    ON_WM_HOTKEY()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_VSCROLL()
    ON_BN_CLICKED(IDC_START, &CKeyboardClickerDlg::OnStart)
    ON_BN_CLICKED(IDC_STOP, &CKeyboardClickerDlg::OnStop)
    ON_BN_CLICKED(IDC_ADD_CLICK_ACTION, &CKeyboardClickerDlg::OnAddClickAction)
    ON_BN_CLICKED(IDC_REMOVE_CLICK_ACTION, &CKeyboardClickerDlg::OnRemoveClickAction)
    ON_BN_CLICKED(IDC_ADD_SHOUT_ACTION, &CKeyboardClickerDlg::OnAddShoutAction)
    ON_BN_CLICKED(IDC_REMOVE_SHOUT_ACTION, &CKeyboardClickerDlg::OnRemoveShoutAction)
    ON_BN_CLICKED(IDC_ADVANCED_FEATURES, &CKeyboardClickerDlg::OnAdvancedFeatures)
    ON_BN_CLICKED(IDC_SKILL_MONITOR_TOGGLE, &CKeyboardClickerDlg::OnToggleSkillMonitoring)
    ON_BN_CLICKED(IDC_CLEAR_DEBUG_LOG, &CKeyboardClickerDlg::OnClearDebugLog)
    ON_BN_CLICKED(IDC_HORN_ENABLED, &CKeyboardClickerDlg::OnHornEnabled)
    ON_BN_CLICKED(IDC_DEBUFF_OPTIONS, &CKeyboardClickerDlg::OnDebuffOptions)
    ON_BN_CLICKED(IDC_SELECT_DEBUFF_REGION, &CKeyboardClickerDlg::OnSelectDebuffRegion)
    ON_BN_CLICKED(IDC_SELECT_HORN_REGION, &CKeyboardClickerDlg::OnSelectHornRegion)
    ON_BN_CLICKED(IDC_ADD_HORN_SAMPLE, &CKeyboardClickerDlg::OnAddHornSample)
    ON_BN_CLICKED(IDC_TEST_HORN, &CKeyboardClickerDlg::OnTestHornRecognition)
    ON_BN_CLICKED(IDC_DEBUFF_CHECK_BASE + 0, &CKeyboardClickerDlg::OnDebuffOptionChanged)
    ON_BN_CLICKED(IDC_DEBUFF_CHECK_BASE + 1, &CKeyboardClickerDlg::OnDebuffOptionChanged)
    ON_BN_CLICKED(IDC_DEBUFF_CHECK_BASE + 2, &CKeyboardClickerDlg::OnDebuffOptionChanged)
    ON_BN_CLICKED(IDC_DEBUFF_CHECK_BASE + 3, &CKeyboardClickerDlg::OnDebuffOptionChanged)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SKILL_REGION_BASE + 0, IDC_SKILL_REGION_BASE + 3, &CKeyboardClickerDlg::OnSelectSkillRegion)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SKILL_SAMPLE_BASE + 0, IDC_SKILL_SAMPLE_BASE + 3, &CKeyboardClickerDlg::OnAddSkillSample)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SKILL_DETECT_BASE + 0, IDC_SKILL_DETECT_BASE + 3, &CKeyboardClickerDlg::OnDetectSkill)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SKILL_ENABLED_BASE + 0, IDC_SKILL_ENABLED_BASE + 3, &CKeyboardClickerDlg::OnSkillEnabledChanged)
    ON_CONTROL_RANGE(EN_CHANGE, IDC_SKILL_INTERVAL_BASE + 0, IDC_SKILL_INTERVAL_BASE + 3, &CKeyboardClickerDlg::OnSkillIntervalChanged)
    ON_CONTROL_RANGE(EN_CHANGE, IDC_SKILL_DETECT_INTERVAL_BASE + 0, IDC_SKILL_DETECT_INTERVAL_BASE + 3, &CKeyboardClickerDlg::OnSkillIntervalChanged)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_SKILL_DEBUFF_BASE, IDC_SKILL_DEBUFF_BASE + 15, &CKeyboardClickerDlg::OnSkillDebuffChanged)
    ON_CONTROL_RANGE(BN_CLICKED, IDC_DEBUFF_SAMPLE_BASE + 0, IDC_DEBUFF_SAMPLE_BASE + 3, &CKeyboardClickerDlg::OnAddDebuffSample)
    ON_NOTIFY(TCN_SELCHANGE, IDC_PAGE_TABS, &CKeyboardClickerDlg::OnPageTabChanged)
    ON_MESSAGE(WM_KEY_CAPTURED, &CKeyboardClickerDlg::OnKeyCaptured)
    ON_MESSAGE(WM_APP_ASYNC_STATUS, &CKeyboardClickerDlg::OnAsyncStatus)
    ON_MESSAGE(WM_APP_SKILL_WORKER_FINISHED, &CKeyboardClickerDlg::OnSkillWorkerFinished)
    ON_MESSAGE(WM_APP_SHOUT_WORKER_FINISHED, &CKeyboardClickerDlg::OnShoutWorkerFinished)
END_MESSAGE_MAP()

CKeyboardClickerDlg::CKeyboardClickerDlg()
    : CDialogEx(IDD_KEYBOARDCLICKER_DIALOG)
    , m_rng(std::random_device{}())
{
}

BOOL CKeyboardClickerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    InitializeClickActionControls();
    InitializeShoutActionControls();
    InitializeAdvancedControls();
    m_clickScrollBar.Create(WS_CHILD | WS_VISIBLE | SBS_VERT, CRect(0, 0, 0, 0), this, IDC_CLICK_SCROLL);
    m_shoutScrollBar.Create(WS_CHILD | WS_VISIBLE | SBS_VERT, CRect(0, 0, 0, 0), this, IDC_SHOUT_SCROLL);

    SetDlgItemText(IDC_RANDOM_DEVIATION, _T("0"));
    ConfigureDefaultClickActions();
    ConfigureDefaultShoutActions();
    LoadSettings();
    m_pageTabs.SetCurSel(m_activePage);

    CRect initialWindow;
    GetWindowRect(&initialWindow);
    CRect requiredClient(0, 0, MinWindowWidth, MinWindowHeight);
    ::AdjustWindowRectEx(&requiredClient, GetStyle(), ::GetMenu(m_hWnd) != nullptr, GetExStyle());
    const int initialWidth = (std::max)(initialWindow.Width(), requiredClient.Width());
    const int initialHeight = (std::max)(initialWindow.Height(), requiredClient.Height());
    SetWindowPos(nullptr, 0, 0, initialWidth, initialHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    CenterWindow();

    if (CEdit* debugLog = static_cast<CEdit*>(GetDlgItem(IDC_DEBUG_LOG)))
    {
        debugLog->SetLimitText(MaxDebugTextLength + 4096);
    }

    GetDlgItem(IDC_STOP)->EnableWindow(FALSE);

    if (!RegisterHotKey(m_hWnd, HotkeyCaptureTarget, 0, VK_F9))
    {
        SetStatus(_T("F9 热键注册失败，可能已被其他程序占用。"));
    }
    else
    {
        SetStatus(_T("就绪。把鼠标移到目标窗口/控件上，然后按 F9。"));
    }
    ShowClickActionRows();
    ShowShoutActionRows();
    LayoutControls();
    RegisterAllShoutHotkeys();

    return TRUE;
}

void CKeyboardClickerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

void CKeyboardClickerDlg::OnDestroy()
{
    m_closing.store(true);
    SaveSettings();
    StopJobs();
    StopHornMonitoring();
    UnregisterHotKey(m_hWnd, HotkeyCaptureTarget);
    UnregisterAllShoutHotkeys();
    if (m_skillWorker.joinable())
    {
        m_skillWorker.join();
    }
    if (m_shoutWorker.joinable())
    {
        m_shoutWorker.join();
    }
    ReleaseDmResources();
    CDialogEx::OnDestroy();
}

void CKeyboardClickerDlg::OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2)
{
    UNREFERENCED_PARAMETER(nKey1);
    UNREFERENCED_PARAMETER(nKey2);

    if (nHotKeyId == HotkeyCaptureTarget)
    {
        CaptureTargetUnderCursor();
        return;
    }
    const int hotkeyId = static_cast<int>(nHotKeyId);
    if (hotkeyId >= HotkeyShoutBase && hotkeyId < HotkeyShoutBase + static_cast<int>(MaxShoutActions))
    {
        if (!m_shoutWorkerActive.load())
        {
            HandleShoutHotkey(static_cast<size_t>(hotkeyId - HotkeyShoutBase));
        }
        return;
    }

    CDialogEx::OnHotKey(nHotKeyId, nKey1, nKey2);
}

void CKeyboardClickerDlg::OnTimer(UINT_PTR nIDEvent)
{
    JoinFinishedAsyncWorkers();

    if (nIDEvent == HornTimerId)
    {
        if (!m_hornState.enabled || !HasEnabledAdvancedSkill())
        {
            StopHornMonitoring();
            return;
        }

        DetectBestSkillOnce(false, true);
        if (m_hornState.enabled)
        {
            SetTimer(HornTimerId, NextSkillMonitorDelay(), nullptr);
        }
        return;
    }

    if (nIDEvent >= TimerBase)
    {
        const UINT_PTR timerOffset = nIDEvent - TimerBase;
        if (timerOffset >= m_jobs.size())
        {
            CDialogEx::OnTimer(nIDEvent);
            return;
        }

        const size_t index = static_cast<size_t>(timerOffset);
        if (m_running && m_jobs[index].enabled)
        {
            SendKey(m_jobs[index].vk);
            ScheduleNext(index);
        }
        return;
    }

    CDialogEx::OnTimer(nIDEvent);
}

void CKeyboardClickerDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    LayoutControls();
}

void CKeyboardClickerDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    CDialogEx::OnGetMinMaxInfo(lpMMI);

    CRect requiredClient(0, 0, MinWindowWidth, MinWindowHeight);
    if (GetSafeHwnd())
    {
        ::AdjustWindowRectEx(&requiredClient, GetStyle(), ::GetMenu(m_hWnd) != nullptr, GetExStyle());
    }
    lpMMI->ptMinTrackSize.x = requiredClient.Width();
    lpMMI->ptMinTrackSize.y = requiredClient.Height();
}

void CKeyboardClickerDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    size_t* offset = nullptr;
    size_t count = 0;
    size_t capacity = 0;

    if (pScrollBar != nullptr && pScrollBar->GetSafeHwnd() == m_clickScrollBar.GetSafeHwnd())
    {
        offset = &m_clickScrollOffset;
        count = m_clickActionCount;
        capacity = VisibleClickRowCapacity();
    }
    else if (pScrollBar != nullptr && pScrollBar->GetSafeHwnd() == m_shoutScrollBar.GetSafeHwnd())
    {
        offset = &m_shoutScrollOffset;
        count = m_shoutActionCount;
        capacity = VisibleShoutRowCapacity();
    }
    else
    {
        CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
        return;
    }

    const size_t maxOffset = count > capacity ? count - capacity : 0;
    size_t nextOffset = *offset;
    switch (nSBCode)
    {
    case SB_LINEUP:
        if (nextOffset > 0)
        {
            --nextOffset;
        }
        break;
    case SB_LINEDOWN:
        if (nextOffset < maxOffset)
        {
            ++nextOffset;
        }
        break;
    case SB_PAGEUP:
        nextOffset = nextOffset > capacity ? nextOffset - capacity : 0;
        break;
    case SB_PAGEDOWN:
        nextOffset = (std::min)(maxOffset, nextOffset + capacity);
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        nextOffset = (std::min)(maxOffset, static_cast<size_t>(nPos));
        break;
    default:
        break;
    }

    if (nextOffset != *offset)
    {
        *offset = nextOffset;
        LayoutControls();
    }
}

void CKeyboardClickerDlg::OnStart()
{
    if (m_running)
    {
        return;
    }

    if (!IsWindow(m_targetHwnd))
    {
        SetStatus(_T("请先把鼠标移到有效目标窗口/控件上，然后按 F9。"));
        return;
    }

    if (!ReadJobs())
    {
        return;
    }
    SaveSettings();

    bool hasJob = false;
    for (size_t i = 0; i < m_jobs.size(); ++i)
    {
        if (m_jobs[i].enabled)
        {
            ScheduleNext(i);
            hasJob = true;
        }
    }

    if (!hasJob)
    {
        SetStatus(_T("请至少点击一个按键输入框并按下要连点的按键。"));
        return;
    }

    m_running = true;
    GetDlgItem(IDC_START)->EnableWindow(FALSE);
    GetDlgItem(IDC_STOP)->EnableWindow(TRUE);
    ShowClickActionRows();
    SetStatus(_T("运行中。"));
}

void CKeyboardClickerDlg::OnStop()
{
    StopJobs();
}

void CKeyboardClickerDlg::OnAdvancedFeatures()
{
    if (m_advancedInitialized)
    {
        m_activePage = 1;
        m_pageTabs.SetCurSel(m_activePage);
        LayoutControls();
        SetStatus(_T("高级功能已启用。需要自动检测时，请在高级功能页点击“开始”。"));
        return;
    }

    if (!RequestAdvancedPassword())
    {
        return;
    }

    SetStatus(_T("正在注册大漠高级功能..."));
    if (RegisterDmPlugin())
    {
        m_advancedInitialized = true;
        m_activePage = 1;
        m_pageTabs.SetCurSel(m_activePage);
        StopHornMonitoring();
        LayoutControls();
        SaveSettings();
        SetStatus(_T("高级功能初始化完成。点击“开始”后才会自动检测。"));
    }
}

void CKeyboardClickerDlg::OnToggleSkillMonitoring()
{
    if (!m_advancedInitialized)
    {
        SetStatus(_T("请先点击底部“高级功能”完成初始化。"));
        return;
    }

    if (m_hornState.enabled)
    {
        StopHornMonitoring();
        UpdateSkillMonitorButtonText();
        SetStatus(_T("自动检测已停止。"));
        return;
    }

    if (!HasEnabledAdvancedSkill())
    {
        SetStatus(_T("请至少启用一个技能后再开始自动检测。"));
        return;
    }

    m_hornState.enabled = true;
    UpdateSkillMonitoringTimer();
    UpdateSkillMonitorButtonText();
}

void CKeyboardClickerDlg::OnClearDebugLog()
{
    if (!GetSafeHwnd())
    {
        return;
    }

    SetDlgItemText(IDC_DEBUG_LOG, _T(""));
    SetDlgItemText(IDC_STATUS, _T("调试输出已清空。"));
    m_lastStatusText = _T("调试输出已清空。");
}

void CKeyboardClickerDlg::OnHornEnabled()
{
    DetectSkillOnce(0);
    SaveSettings();
}

void CKeyboardClickerDlg::OnDebuffOptions()
{
    m_hornState.debuffPanelVisible = !m_hornState.debuffPanelVisible;
    UpdateAdvancedControls();
    LayoutControls();
}

void CKeyboardClickerDlg::OnDebuffOptionChanged()
{
    for (size_t i = 0; i < m_hornState.debuffs.size(); ++i)
    {
        m_hornState.debuffs[i] = m_debuffChecks[i].GetCheck() == BST_CHECKED;
    }
    InvalidateHornPictureCache();
    UpdateDebuffSummary();
    SaveSettings();
}

void CKeyboardClickerDlg::OnAddHornSample()
{
    if (AddPictureSamples(GetSkillSampleDirectory(0), AdvancedSkillDefinitions[0].label))
    {
        InvalidateHornPictureCache();
    }
}

void CKeyboardClickerDlg::OnTestHornRecognition()
{
    DetectBestSkillOnce();
}

void CKeyboardClickerDlg::OnSelectSkillRegion(UINT nID)
{
    const int offset = static_cast<int>(nID) - IDC_SKILL_REGION_BASE;
    if (offset < 0 || offset >= static_cast<int>(m_skillStates.size()))
    {
        return;
    }

    const size_t index = static_cast<size_t>(offset);
    CRect region;
    CString prompt;
    prompt.Format(_T("请拖拽 %s 识别区域，按 Esc 可取消。"), AdvancedSkillDefinitions[index].label);
    SetStatus(prompt);
    if (!SelectScreenRegion(region))
    {
        CString message;
        message.Format(_T("%s 区域选择已取消。"), AdvancedSkillDefinitions[index].label);
        SetStatus(message);
        return;
    }

    m_skillStates[index].region = region;
    m_skillStates[index].hasRegion = true;

    CString emptyText;
    emptyText.Format(_T("%s范围: 未设置"), AdvancedSkillDefinitions[index].label);
    SetRegionStatus(m_skillControls[index].regionLabel, true, region, emptyText);

    CString message;
    message.Format(_T("%s 区域已设置: %ld,%ld - %ld,%ld"), AdvancedSkillDefinitions[index].label, region.left, region.top, region.right, region.bottom);
    SetStatus(message);
    SaveSettings();
}

void CKeyboardClickerDlg::OnAddSkillSample(UINT nID)
{
    const int offset = static_cast<int>(nID) - IDC_SKILL_SAMPLE_BASE;
    if (offset < 0 || offset >= static_cast<int>(m_skillStates.size()))
    {
        return;
    }

    const size_t index = static_cast<size_t>(offset);
    if (AddPictureSamples(GetSkillSampleDirectory(index), AdvancedSkillDefinitions[index].label))
    {
        InvalidateHornPictureCache();
    }
}

void CKeyboardClickerDlg::OnDetectSkill(UINT nID)
{
    const int offset = static_cast<int>(nID) - IDC_SKILL_DETECT_BASE;
    if (offset < 0 || offset >= static_cast<int>(m_skillStates.size()))
    {
        return;
    }

    DetectSkillOnce(static_cast<size_t>(offset));
    SaveSettings();
}

void CKeyboardClickerDlg::OnSkillEnabledChanged(UINT nID)
{
    const int offset = static_cast<int>(nID) - IDC_SKILL_ENABLED_BASE;
    if (offset < 0 || offset >= static_cast<int>(m_skillStates.size()))
    {
        return;
    }

    const size_t index = static_cast<size_t>(offset);
    m_skillStates[index].enabled = m_skillControls[index].enabledCheck.GetCheck() == BST_CHECKED;
    UpdateSkillMonitoringTimer();
    SaveSettings();
}

void CKeyboardClickerDlg::OnSkillIntervalChanged(UINT nID)
{
    int offset = static_cast<int>(nID) - IDC_SKILL_DETECT_INTERVAL_BASE;
    const bool isDetectInterval = offset >= 0 && offset < static_cast<int>(m_skillStates.size());
    if (!isDetectInterval)
    {
        offset = static_cast<int>(nID) - IDC_SKILL_INTERVAL_BASE;
    }
    if (offset < 0 || offset >= static_cast<int>(m_skillStates.size()))
    {
        return;
    }

    const size_t index = static_cast<size_t>(offset);
    if (isDetectInterval)
    {
        if (!m_skillControls[index].detectIntervalEdit.GetSafeHwnd())
        {
            return;
        }
        m_skillStates[index].detectIntervalMs = ReadSkillDetectIntervalMs(index);
    }
    else
    {
        if (!m_skillControls[index].intervalEdit.GetSafeHwnd())
        {
            return;
        }
        m_skillStates[index].releaseIntervalMs = ReadSkillReleaseIntervalMs(index);
    }

    if (m_hornState.enabled && HasEnabledAdvancedSkill())
    {
        m_hornState.intervalMs = NextSkillMonitorDelay();
        SetTimer(HornTimerId, m_hornState.intervalMs, nullptr);
    }
}

void CKeyboardClickerDlg::OnSkillDebuffChanged(UINT nID)
{
    const int offset = static_cast<int>(nID) - IDC_SKILL_DEBUFF_BASE;
    if (offset < 0)
    {
        return;
    }

    const size_t skillIndex = static_cast<size_t>(offset / 4);
    const size_t debuffIndex = static_cast<size_t>(offset % 4);
    if (skillIndex >= m_skillStates.size() || debuffIndex >= m_skillStates[skillIndex].debuffs.size())
    {
        return;
    }

    m_skillStates[skillIndex].debuffs[debuffIndex] = m_skillControls[skillIndex].debuffChecks[debuffIndex].GetCheck() == BST_CHECKED;
    InvalidateHornPictureCache();
    SaveSettings();
}

void CKeyboardClickerDlg::OnAddDebuffSample(UINT nID)
{
    const int controlOffset = static_cast<int>(nID) - IDC_DEBUFF_SAMPLE_BASE;
    if (controlOffset < 0)
    {
        return;
    }

    const size_t index = static_cast<size_t>(controlOffset);
    if (index >= m_debuffSampleButtons.size())
    {
        return;
    }

    if (AddDebuffSample(index))
    {
        InvalidateHornPictureCache();
    }
}

void CKeyboardClickerDlg::OnSelectDebuffRegion()
{
    CRect region;
    SetStatus(_T("请拖拽 Debuff 识别区域，按 Esc 可取消。"));
    if (!SelectScreenRegion(region))
    {
        SetStatus(_T("Debuff 区域选择已取消。"));
        return;
    }

    m_hornState.debuffRegion = region;
    m_hornState.hasDebuffRegion = true;
    SetRegionStatus(m_debuffRegionLabel, true, region, _T("Debuff范围: 未设置"));

    CString message;
    message.Format(_T("Debuff 区域已设置: %ld,%ld - %ld,%ld"), region.left, region.top, region.right, region.bottom);
    SetStatus(message);
    SaveSettings();
}

void CKeyboardClickerDlg::OnSelectHornRegion()
{
    OnSelectSkillRegion(IDC_SKILL_REGION_BASE);
}

void CKeyboardClickerDlg::OnPageTabChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    UNREFERENCED_PARAMETER(pNMHDR);
    const int selection = m_pageTabs.GetCurSel();
    if (selection >= 0)
    {
        m_activePage = selection;
        LayoutControls();
        SaveSettings();
    }
    if (pResult != nullptr)
    {
        *pResult = 0;
    }
}

LRESULT CKeyboardClickerDlg::OnKeyCaptured(WPARAM wParam, LPARAM lParam)
{
    const int controlId = static_cast<int>(wParam);
    for (size_t i = 0; i < MaxClickActions; ++i)
    {
        if (controlId == ClickControlId(i, 1))
        {
            SaveSettings();
            SetStatus(_T("自动按键已设置。"));
            return 0;
        }
    }

    for (size_t i = 0; i < MaxShoutActions; ++i)
    {
        if (controlId == ActionControlId(i, 1))
        {
            RegisterShoutHotkey(i);
            SaveSettings();
            return 0;
        }
        if (controlId == ActionControlId(i, 7))
        {
            SaveSettings();
            SetStatus(lParam == 0 ? _T("冷却补按已清空。") : _T("冷却补按已设置。"));
            return 0;
        }
    }

    if (controlId >= IDC_SKILL_KEY_BASE && controlId < IDC_SKILL_KEY_BASE + static_cast<int>(m_skillStates.size()))
    {
        const size_t index = static_cast<size_t>(controlId - IDC_SKILL_KEY_BASE);
        m_skillStates[index].skillVk = static_cast<UINT>(lParam);
        SaveSettings();
        CString message;
        message.Format(lParam == 0 ? _T("%s 技能键已清空。") : _T("%s 技能键已设置。"), AdvancedSkillDefinitions[index].label);
        SetStatus(message);
        return 0;
    }

    if (controlId == IDC_HORN_SKILL_KEY)
    {
        m_skillStates[0].skillVk = static_cast<UINT>(lParam);
        SaveSettings();
        SetStatus(lParam == 0 ? _T("号角技能键已清空。") : _T("号角技能键已设置。"));
        return 0;
    }

    UNREFERENCED_PARAMETER(lParam);
    SaveSettings();
    SetStatus(_T("按键已获取。设置间隔后点击开始。"));
    return 0;
}

LRESULT CKeyboardClickerDlg::OnAsyncStatus(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    std::unique_ptr<AsyncStatusMessage> message(reinterpret_cast<AsyncStatusMessage*>(lParam));
    if (message)
    {
        SetStatus(message->text);
    }
    return 0;
}

LRESULT CKeyboardClickerDlg::OnSkillWorkerFinished(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    std::unique_ptr<SkillDetectionResult> result(reinterpret_cast<SkillDetectionResult*>(lParam));
    if (!result)
    {
        return 0;
    }

    const ULONGLONG now = ::GetTickCount64();
    const bool shouldLogDiagnostics = !result->respectIntervals || now - m_hornState.lastHornDebugTick >= 2000;
    if (shouldLogDiagnostics)
    {
        for (const CString& diagnostic : result->diagnostics)
        {
            AppendDebugText(diagnostic);
        }
        if (!result->diagnostics.empty())
        {
            m_hornState.lastHornDebugTick = now;
        }
    }

    for (const SkillAvailabilityUpdate& update : result->availabilityUpdates)
    {
        if (update.index < m_skillStates.size())
        {
            m_skillStates[update.index].lastSkillAvailable = update.available;
            m_skillStates[update.index].lastSkillAvailableTick = update.tick;
            m_nextAvailabilityProbeIndex = update.index + 1;
        }
    }

    if (result->clearActionLock)
    {
        m_hornState.actionLockedUntilDebuffClears = false;
        for (auto& skill : m_skillStates)
        {
            skill.lastDetectTick = 0;
        }
    }

    if (result->success)
    {
        if (result->respectIntervals && !m_hornState.enabled)
        {
            SetStatus(_T("自动检测已停止，本次后台识图结果已忽略。"));
        }
        else if (result->inputAlreadySent || (PrepareForegroundTarget() && SendInputKey(result->skillVk)))
        {
            if (result->index < m_skillStates.size())
            {
                m_skillStates[result->index].lastReleaseTick = now;
                m_skillStates[result->index].lastSkillAvailable = false;
                m_skillStates[result->index].lastSkillAvailableTick = 0;
            }
            if (result->setActionLock)
            {
                m_hornState.actionLockedUntilDebuffClears = true;
            }
            CString message;
            if (!result->message.IsEmpty())
            {
                message = result->message;
            }
            else
            {
                message.Format(_T("按优先级检测：已使用 %s，优先级 %u。"),
                    AdvancedSkillDefinitions[result->index].label,
                    static_cast<unsigned>(result->priority));
            }
            SetStatus(message);
        }
        else
        {
            SetStatus(_T("后台识图已命中，但发送技能键失败。"));
        }
    }
    else if (!result->message.IsEmpty())
    {
        SetStatus(result->message);
    }

    m_skillInputPending.store(false);

    if (result->saveSettings)
    {
        SaveSettings();
    }
    JoinFinishedAsyncWorkers();
    return 0;
}

LRESULT CKeyboardClickerDlg::OnShoutWorkerFinished(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    std::unique_ptr<ShoutWorkerResult> result(reinterpret_cast<ShoutWorkerResult*>(lParam));
    RegisterAllShoutHotkeys();
    ShowShoutActionRows();

    if (result && !result->message.IsEmpty())
    {
        SetStatus(result->message);
    }
    else
    {
        SetStatus(_T("喊话任务已结束。"));
    }

    JoinFinishedAsyncWorkers();
    return 0;
}

void CKeyboardClickerDlg::CaptureTargetUnderCursor()
{
    POINT point = {};
    if (!GetCursorPos(&point))
    {
        SetStatus(_T("读取鼠标位置失败。"));
        return;
    }

    HWND hwnd = ::WindowFromPoint(point);
    if (!IsWindow(hwnd))
    {
        SetStatus(_T("鼠标下方没有有效窗口/控件。"));
        return;
    }

    m_targetHwnd = hwnd;
    SetDlgItemText(IDC_TARGET_HWND, FormatHwnd(hwnd));
    SetDlgItemText(IDC_TARGET_TITLE, ReadWindowText(hwnd));
    SetStatus(_T("目标已获取。"));
}

void CKeyboardClickerDlg::StopJobs()
{
    for (size_t i = 0; i < m_jobs.size(); ++i)
    {
        KillTimer(TimerBase + i);
        m_jobs[i].enabled = false;
    }

    m_running = false;

    if (GetSafeHwnd())
    {
        if (CWnd* start = GetDlgItem(IDC_START))
        {
            start->EnableWindow(TRUE);
        }
        if (CWnd* stop = GetDlgItem(IDC_STOP))
        {
            stop->EnableWindow(FALSE);
        }
        ShowClickActionRows();
        SetStatus(_T("已停止。"));
    }
}

void CKeyboardClickerDlg::InitializeClickActionControls()
{
    CFont* font = GetFont();
    const DWORD editStyle = WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL;
    const DWORD readOnlyEditStyle = editStyle | ES_READONLY;
    const DWORD numberEditStyle = editStyle | ES_NUMBER;
    const DWORD labelStyle = WS_CHILD | SS_LEFT;

    for (size_t i = 0; i < MaxClickActions; ++i)
    {
        const int y = ClickRowY + static_cast<int>(i) * ClickRowHeight;
        CString indexText;
        indexText.Format(_T("%u"), static_cast<unsigned>(i + 1));

        m_clickControls[i].indexLabel.Create(indexText, labelStyle, CRect(24, y + 3, 42, y + 17), this, ClickControlId(i, 0));
        m_clickControls[i].keyEdit.Create(readOnlyEditStyle, CRect(52, y, 100, y + 16), this, ClickControlId(i, 1));
        m_clickControls[i].intervalEdit.Create(numberEditStyle, CRect(122, y, 186, y + 16), this, ClickControlId(i, 2));

        m_clickControls[i].indexLabel.SetFont(font);
        m_clickControls[i].keyEdit.SetFont(font);
        m_clickControls[i].intervalEdit.SetFont(font);
    }
}

void CKeyboardClickerDlg::ConfigureDefaultClickActions()
{
    m_clickActionCount = 3;
    for (size_t i = 0; i < MaxClickActions; ++i)
    {
        m_clickControls[i].intervalEdit.SetWindowText(_T("1000"));
    }
}

void CKeyboardClickerDlg::ShowClickActionRows()
{
    if (CWnd* addButton = GetDlgItem(IDC_ADD_CLICK_ACTION))
    {
        addButton->EnableWindow(m_clickActionCount < MaxClickActions && !m_running);
    }
    if (CWnd* removeButton = GetDlgItem(IDC_REMOVE_CLICK_ACTION))
    {
        removeButton->EnableWindow(m_clickActionCount > 1 && !m_running);
    }
    ClampScrollOffsets();
    UpdateScrollBars();
}

void CKeyboardClickerDlg::MoveDlgItem(int controlId, int x, int y, int width, int height)
{
    if (CWnd* control = GetDlgItem(controlId))
    {
        control->MoveWindow(x, y, width, height);
    }
}

size_t CKeyboardClickerDlg::VisibleClickRowsPerColumn() const
{
    if (!GetSafeHwnd())
    {
        return 1;
    }

    CRect client;
    GetClientRect(&client);
    const BaseLayoutMetrics metrics = CalculateBaseLayoutMetrics(client);
    const int rowsTop = metrics.clickY + ClickRowsOffsetY;
    const int rowsBottom = metrics.clickY + metrics.clickH - ClickFooterReservedHeight;
    const int available = (std::max)(ClickDisplayRowHeight, rowsBottom - rowsTop);
    return static_cast<size_t>((std::max)(1, available / ClickDisplayRowHeight));
}

size_t CKeyboardClickerDlg::VisibleClickRowCapacity() const
{
    return VisibleClickRowsPerColumn() * 2;
}

size_t CKeyboardClickerDlg::VisibleShoutRowCapacity() const
{
    if (!GetSafeHwnd())
    {
        return 1;
    }

    CRect client;
    GetClientRect(&client);
    const BaseLayoutMetrics metrics = CalculateBaseLayoutMetrics(client);
    const int rowsTop = metrics.shoutY + ShoutRowsOffsetY;
    const int rowsBottom = metrics.shoutY + (std::max)(MinShoutGroupHeight, metrics.shoutH) - 12;
    const int available = (std::max)(ShoutDisplayRowHeight, rowsBottom - rowsTop);
    return static_cast<size_t>((std::max)(1, available / ShoutDisplayRowHeight));
}

void CKeyboardClickerDlg::ClampScrollOffsets()
{
    const size_t clickCapacity = VisibleClickRowCapacity();
    const size_t maxClickOffset = m_clickActionCount > clickCapacity ? m_clickActionCount - clickCapacity : 0;
    m_clickScrollOffset = (std::min)(m_clickScrollOffset, maxClickOffset);

    const size_t shoutCapacity = VisibleShoutRowCapacity();
    const size_t maxShoutOffset = m_shoutActionCount > shoutCapacity ? m_shoutActionCount - shoutCapacity : 0;
    m_shoutScrollOffset = (std::min)(m_shoutScrollOffset, maxShoutOffset);
}

void CKeyboardClickerDlg::UpdateScrollBars()
{
    if (m_clickScrollBar.GetSafeHwnd())
    {
        const size_t capacity = VisibleClickRowCapacity();
        const bool needsScroll = m_clickActionCount > capacity;
        m_clickScrollBar.ShowWindow(needsScroll ? SW_SHOW : SW_HIDE);

        SCROLLINFO info = {};
        info.cbSize = sizeof(info);
        info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        info.nMin = 0;
        info.nMax = static_cast<int>(m_clickActionCount > 0 ? m_clickActionCount - 1 : 0);
        info.nPage = static_cast<UINT>(capacity);
        info.nPos = static_cast<int>(m_clickScrollOffset);
        m_clickScrollBar.SetScrollInfo(&info, TRUE);
    }

    if (m_shoutScrollBar.GetSafeHwnd())
    {
        const size_t capacity = VisibleShoutRowCapacity();
        const bool needsScroll = m_shoutActionCount > capacity;
        m_shoutScrollBar.ShowWindow(needsScroll ? SW_SHOW : SW_HIDE);

        SCROLLINFO info = {};
        info.cbSize = sizeof(info);
        info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        info.nMin = 0;
        info.nMax = static_cast<int>(m_shoutActionCount > 0 ? m_shoutActionCount - 1 : 0);
        info.nPage = static_cast<UINT>(capacity);
        info.nPos = static_cast<int>(m_shoutScrollOffset);
        m_shoutScrollBar.SetScrollInfo(&info, TRUE);
    }
}

void CKeyboardClickerDlg::LayoutControls()
{
    if (!GetSafeHwnd() || !GetDlgItem(IDC_STATUS))
    {
        return;
    }
    if (!m_clickControls[0].indexLabel.GetSafeHwnd() || !m_shoutControls[0].indexLabel.GetSafeHwnd())
    {
        return;
    }

    SetRedraw(FALSE);

    CRect client;
    GetClientRect(&client);
    const int margin = LayoutMargin;
    const int groupWidth = client.Width() - margin * 2;

    m_pageTabs.MoveWindow(margin, margin, groupWidth, AdvancedTabHeight);
    m_pageTabs.ShowWindow(SW_SHOW);
    if (m_pageTabs.GetCurSel() != m_activePage)
    {
        m_pageTabs.SetCurSel(m_activePage);
    }

    LayoutDebugPanel();
    if (m_activePage == 1)
    {
        LayoutAdvancedPage();
    }
    else
    {
        LayoutBasePage();
    }

    SetRedraw(TRUE);
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void CKeyboardClickerDlg::LayoutDebugPanel()
{
    CRect client;
    GetClientRect(&client);
    const int margin = LayoutMargin;
    const int groupWidth = client.Width() - margin * 2;
    const int innerLeft = margin + 14;
    const int innerRight = client.Width() - margin - 14;
    const BaseLayoutMetrics layoutMetrics = CalculateBaseLayoutMetrics(client);

    MoveDlgItem(IDC_STATUS, margin, layoutMetrics.statusY + 4, (std::max)(120, groupWidth - 180), 18);
    MoveDlgItem(IDC_ADVANCED_FEATURES, innerRight - 170, layoutMetrics.statusY, 76, 24);
    MoveDlgItem(IDC_CLEAR_DEBUG_LOG, innerRight - 82, layoutMetrics.statusY, 76, 24);
    MoveDlgItem(IDC_DEBUG_GROUP, margin, layoutMetrics.debugY, groupWidth, layoutMetrics.debugH);
    MoveDlgItem(IDC_DEBUG_LOG, innerLeft, layoutMetrics.debugY + 24, innerRight - innerLeft, (std::max)(40, layoutMetrics.debugH - 36));

    if (CWnd* advancedButton = GetDlgItem(IDC_ADVANCED_FEATURES))
    {
        advancedButton->SetWindowText(m_advancedInitialized ? _T("高级已启用") : _T("高级功能"));
    }
}

void CKeyboardClickerDlg::LayoutAdvancedPage()
{
    CRect client;
    GetClientRect(&client);
    const int margin = LayoutMargin;
    const int gap = LayoutGap;
    const int width = client.Width();
    const int groupWidth = width - margin * 2;
    const int innerLeft = margin + 14;
    const int innerRight = width - margin - 14;
    const int contentTop = margin + AdvancedTabHeight + gap;
    const BaseLayoutMetrics layoutMetrics = CalculateBaseLayoutMetrics(client);

    ShowBasePageControls(SW_HIDE);
    ShowAdvancedPageControls(SW_SHOW);
    UpdateSkillMonitorButtonText();

    if (m_advancedInitialized)
    {
        m_advancedHintLabel.SetWindowText(_T("高级功能已启用。检测(ms)=Debuff 扫描频率；冷却(ms)=释放后防重复触发。"));
    }
    else
    {
        m_advancedHintLabel.SetWindowText(_T("请先点击底部“高级功能”完成初始化；初始化前不会显示技能设置项。"));
    }

    m_advancedHintLabel.MoveWindow(innerLeft, contentTop + 6, innerRight - innerLeft, AdvancedHintHeight);

    const int sharedY = contentTop + 32;
    m_hornSimilarityLabel.MoveWindow(innerLeft, sharedY + 4, 54, 18);
    m_hornSimilarityEdit.MoveWindow(innerLeft + 56, sharedY, 60, 22);
    m_debuffRegionButton.MoveWindow(innerLeft + 128, sharedY - 1, 92, 24);
    m_debuffOptionsButton.MoveWindow(innerLeft + 230, sharedY - 1, 92, 24);
    m_hornTestButton.MoveWindow(innerLeft + 332, sharedY - 1, 108, 24);
    m_skillMonitorButton.MoveWindow(innerLeft + 450, sharedY - 1, 70, 24);
    m_debuffRegionLabel.MoveWindow(innerLeft, sharedY + 34, innerRight - innerLeft, 18);
    m_debuffSummaryLabel.MoveWindow(innerLeft, sharedY + 56, innerRight - innerLeft, 18);

    const int sampleButtonY = sharedY + 82;
    const int sampleButtonW = (std::max)(118, (innerRight - innerLeft - 30) / 4);
    for (size_t i = 0; i < m_debuffChecks.size(); ++i)
    {
        const int x = innerLeft + static_cast<int>(i) * (sampleButtonW + 10);
        m_debuffChecks[i].MoveWindow(x, sampleButtonY, sampleButtonW, 22);
        m_debuffSampleButtons[i].MoveWindow(x, sampleButtonY, sampleButtonW, 24);
    }

    const int skillTop = sharedY + (m_hornState.debuffPanelVisible ? 116 : 64);
    const int cardGap = 10;
    const int cardW = (std::max)(240, (innerRight - innerLeft - cardGap) / 2);
    const int availableCardArea = layoutMetrics.statusY - gap - skillTop;
    const int cardH = ClampInt((availableCardArea - cardGap) / 2, 166, 178);

    for (size_t i = 0; i < m_skillControls.size(); ++i)
    {
        AdvancedSkillControls& controls = m_skillControls[i];
        const int column = static_cast<int>(i % 2);
        const int row = static_cast<int>(i / 2);
        const int cardX = innerLeft + column * (cardW + cardGap);
        const int cardY = skillTop + row * (cardH + cardGap);
        controls.group.MoveWindow(cardX, cardY, cardW, cardH);
        controls.enabledCheck.MoveWindow(cardX + 14, cardY + 23, 58, 22);
        controls.keyLabel.MoveWindow(cardX + 78, cardY + 25, 54, 18);
        controls.keyEdit.MoveWindow(cardX + 132, cardY + 22, 58, 22);
        controls.priorityLabel.MoveWindow(cardX + 200, cardY + 25, 58, 18);
        controls.priorityEdit.MoveWindow(cardX + 258, cardY + 22, 38, 22);
        controls.detectIntervalLabel.MoveWindow(cardX + 14, cardY + 53, 66, 18);
        controls.detectIntervalEdit.MoveWindow(cardX + 82, cardY + 50, 48, 22);
        controls.intervalLabel.MoveWindow(cardX + 142, cardY + 53, 66, 18);
        controls.intervalEdit.MoveWindow(cardX + 210, cardY + 50, 58, 22);
        const int smallButtonGap = 8;
        const int smallButtonW = (std::max)(62, (cardW - 28 - smallButtonGap * 2) / 3);
        controls.regionButton.MoveWindow(cardX + 14, cardY + 80, smallButtonW, 24);
        controls.sampleButton.MoveWindow(cardX + 14 + smallButtonW + smallButtonGap, cardY + 80, smallButtonW, 24);
        controls.detectButton.MoveWindow(cardX + 14 + (smallButtonW + smallButtonGap) * 2, cardY + 80, smallButtonW, 24);
        controls.regionLabel.MoveWindow(cardX + 14, cardY + 110, cardW - 28, 18);
        controls.timingHelpLabel.MoveWindow(cardX + 14, cardY + 130, cardW - 28, 18);
        const int debuffY = cardY + cardH - 28;
        const int debuffW = (std::max)(52, (cardW - 28) / 4);
        for (size_t debuff = 0; debuff < controls.debuffChecks.size(); ++debuff)
        {
            controls.debuffChecks[debuff].MoveWindow(cardX + 14 + static_cast<int>(debuff) * debuffW, debuffY, debuffW, 22);
        }
    }

    UNREFERENCED_PARAMETER(groupWidth);
}

void CKeyboardClickerDlg::LayoutBasePage()
{
    CRect client;
    GetClientRect(&client);
    const int margin = LayoutMargin;
    const int width = client.Width();
    const int groupWidth = width - margin * 2;
    const int innerLeft = margin + 14;
    const int innerRight = width - margin - 14;

    ShowAdvancedPageControls(SW_HIDE);
    ShowBasePageControls(SW_SHOW);

    const BaseLayoutMetrics metrics = CalculateBaseLayoutMetrics(client);
    const int targetY = metrics.targetY;
    const int targetH = metrics.targetH;
    MoveDlgItem(IDC_TARGET_GROUP, margin, targetY, groupWidth, targetH);
    MoveDlgItem(IDC_TARGET_HWND_LABEL, innerLeft, targetY + 24, 62, 18);
    MoveDlgItem(IDC_TARGET_HWND, innerLeft + 68, targetY + 21, 170, 22);
    MoveDlgItem(IDC_TARGET_HINT, innerLeft + 250, targetY + 24, innerRight - (innerLeft + 250), 18);
    MoveDlgItem(IDC_TARGET_TITLE_LABEL, innerLeft, targetY + 48, 62, 18);
    MoveDlgItem(IDC_TARGET_TITLE, innerLeft + 68, targetY + 45, innerRight - (innerLeft + 68), 22);

    const int clickY = metrics.clickY;
    const int clickH = metrics.clickH;
    MoveDlgItem(IDC_CLICK_GROUP, margin, clickY, groupWidth, clickH);
    MoveDlgItem(IDC_ADD_CLICK_ACTION, innerRight - 118, clickY + 16, 52, 24);
    MoveDlgItem(IDC_REMOVE_CLICK_ACTION, innerRight - 58, clickY + 16, 52, 24);

    const int clickHeaderY = clickY + ClickHeaderOffsetY;
    const int clickRowY = clickY + ClickRowsOffsetY;
    const int clickListRight = innerRight - ScrollBarWidth - 10;
    const int clickColumnGap = 30;
    const int clickColumnW = (std::max)(210, (clickListRight - innerLeft - clickColumnGap) / 2);
    const int clickColumn2X = innerLeft + clickColumnW + clickColumnGap;
    const size_t clickRowsPerColumn = VisibleClickRowsPerColumn();
    const size_t clickVisibleCapacity = VisibleClickRowCapacity();
    ClampScrollOffsets();

    MoveDlgItem(IDC_CLICK_INDEX_HEADER, innerLeft, clickHeaderY, 20, 18);
    MoveDlgItem(IDC_CLICK_KEY_HEADER, innerLeft + 30, clickHeaderY, 44, 18);
    MoveDlgItem(IDC_CLICK_INTERVAL_HEADER, innerLeft + 112, clickHeaderY, 96, 18);
    MoveDlgItem(IDC_CLICK_INDEX_HEADER_2, clickColumn2X, clickHeaderY, 20, 18);
    MoveDlgItem(IDC_CLICK_KEY_HEADER_2, clickColumn2X + 30, clickHeaderY, 44, 18);
    MoveDlgItem(IDC_CLICK_INTERVAL_HEADER_2, clickColumn2X + 112, clickHeaderY, 96, 18);

    for (size_t i = 0; i < MaxClickActions; ++i)
    {
        const bool visible = i >= m_clickScrollOffset && i < m_clickActionCount && i - m_clickScrollOffset < clickVisibleCapacity;
        const int show = visible ? SW_SHOW : SW_HIDE;
        m_clickControls[i].indexLabel.ShowWindow(show);
        m_clickControls[i].keyEdit.ShowWindow(show);
        m_clickControls[i].intervalEdit.ShowWindow(show);

        if (!visible)
        {
            continue;
        }

        const size_t displayIndex = i - m_clickScrollOffset;
        const int columnX = displayIndex >= clickRowsPerColumn ? clickColumn2X : innerLeft;
        const int rowY = clickRowY + static_cast<int>(displayIndex % clickRowsPerColumn) * ClickDisplayRowHeight;
        m_clickControls[i].indexLabel.MoveWindow(columnX, rowY + 3, 24, 18);
        m_clickControls[i].keyEdit.MoveWindow(columnX + 30, rowY, 64, 22);
        m_clickControls[i].intervalEdit.MoveWindow(columnX + 112, rowY, 96, 22);
    }

    if (m_clickScrollBar.GetSafeHwnd())
    {
        m_clickScrollBar.MoveWindow(innerRight - ScrollBarWidth, clickRowY, ScrollBarWidth, (std::max)(ClickDisplayRowHeight, clickH - ClickRowsOffsetY - ClickFooterReservedHeight));
    }
    const int clickFooterY = clickY + clickH - 34;
    MoveDlgItem(IDC_RANDOM_DEVIATION_LABEL, innerRight - 276, clickFooterY + 5, 96, 18);
    MoveDlgItem(IDC_RANDOM_DEVIATION, innerRight - 176, clickFooterY + 2, 54, 22);
    MoveDlgItem(IDC_START, innerRight - 112, clickFooterY - 1, 50, 26);
    MoveDlgItem(IDC_STOP, innerRight - 54, clickFooterY - 1, 50, 26);

    const int shoutY = metrics.shoutY;
    const int shoutH = metrics.shoutH;
    MoveDlgItem(IDC_SHOUT_GROUP, margin, shoutY, groupWidth, shoutH);
    MoveDlgItem(IDC_REMOVE_SHOUT_ACTION, innerRight - 142, shoutY + 16, 66, 24);
    MoveDlgItem(IDC_ADD_SHOUT_ACTION, innerRight - 70, shoutY + 16, 66, 24);

    const int shoutHeaderY = shoutY + ShoutHeaderOffsetY;
    const int shoutRowY = shoutY + ShoutRowsOffsetY;
    const int cdW = 58;
    const int shoutListRight = innerRight - ScrollBarWidth - 8;
    const int cdX = shoutListRight - cdW;
    const int seqX = cdX - 154;
    const int waitX = seqX - 64;
    const int msgX = innerLeft + 168;
    const int msgW = (std::max)(80, waitX - msgX - 10);
    const size_t shoutVisibleCapacity = VisibleShoutRowCapacity();

    MoveDlgItem(IDC_SHOUT_INDEX_HEADER, innerLeft, shoutHeaderY, 20, 18);
    MoveDlgItem(IDC_SHOUT_KEY_HEADER, innerLeft + 28, shoutHeaderY, 48, 18);
    MoveDlgItem(IDC_SHOUT_COUNT_HEADER, innerLeft + 84, shoutHeaderY, 36, 18);
    MoveDlgItem(IDC_SHOUT_DELAY_HEADER, innerLeft + 124, shoutHeaderY, 36, 18);
    MoveDlgItem(IDC_SHOUT_MESSAGE_HEADER, msgX, shoutHeaderY, msgW, 18);
    MoveDlgItem(IDC_SHOUT_WAIT_HEADER, waitX, shoutHeaderY, 58, 18);
    MoveDlgItem(IDC_SHOUT_SEQUENCE_HEADER, seqX, shoutHeaderY, 92, 18);
    MoveDlgItem(IDC_SHOUT_COOLDOWN_HEADER, cdX, shoutHeaderY, cdW, 18);

    for (size_t i = 0; i < MaxShoutActions; ++i)
    {
        const bool visible = i >= m_shoutScrollOffset && i < m_shoutActionCount && i - m_shoutScrollOffset < shoutVisibleCapacity;
        const int show = visible ? SW_SHOW : SW_HIDE;
        m_shoutControls[i].indexLabel.ShowWindow(show);
        m_shoutControls[i].triggerKeyEdit.ShowWindow(show);
        m_shoutControls[i].triggerCountEdit.ShowWindow(show);
        m_shoutControls[i].chatDelayEdit.ShowWindow(show);
        m_shoutControls[i].messageEdit.ShowWindow(show);
        m_shoutControls[i].skillWaitEdit.ShowWindow(show);
        m_shoutControls[i].sequenceEdit.ShowWindow(show);
        m_shoutControls[i].cooldownSkillKeyEdit.ShowWindow(show);

        if (!visible)
        {
            continue;
        }

        const int rowY = shoutRowY + static_cast<int>(i - m_shoutScrollOffset) * ShoutDisplayRowHeight;
        m_shoutControls[i].indexLabel.MoveWindow(innerLeft, rowY + 3, 20, 18);
        m_shoutControls[i].triggerKeyEdit.MoveWindow(innerLeft + 28, rowY, 50, 22);
        m_shoutControls[i].triggerCountEdit.MoveWindow(innerLeft + 84, rowY, 34, 22);
        m_shoutControls[i].chatDelayEdit.MoveWindow(innerLeft + 124, rowY, 38, 22);
        m_shoutControls[i].messageEdit.MoveWindow(msgX, rowY, msgW, 22);
        m_shoutControls[i].skillWaitEdit.MoveWindow(waitX, rowY, 48, 22);
        m_shoutControls[i].sequenceEdit.MoveWindow(seqX, rowY, 144, 22);
        m_shoutControls[i].cooldownSkillKeyEdit.MoveWindow(cdX, rowY, cdW, 22);
    }

    if (m_shoutScrollBar.GetSafeHwnd())
    {
        m_shoutScrollBar.MoveWindow(innerRight - ScrollBarWidth, shoutRowY, ScrollBarWidth, (std::max)(ShoutDisplayRowHeight, shoutH - ShoutRowsOffsetY - 12));
    }
    UpdateScrollBars();
}

void CKeyboardClickerDlg::OnAddClickAction()
{
    if (m_running)
    {
        SetStatus(_T("运行中不能添加自动按键。"));
        return;
    }
    if (m_clickActionCount >= MaxClickActions)
    {
        CString message;
        message.Format(_T("最多支持 %u 条自动按键。"), static_cast<unsigned>(MaxClickActions));
        SetStatus(message);
        return;
    }

    ++m_clickActionCount;
    const size_t capacity = VisibleClickRowCapacity();
    if (m_clickActionCount > capacity)
    {
        m_clickScrollOffset = m_clickActionCount - capacity;
    }
    ShowClickActionRows();
    LayoutControls();
    SaveSettings();
    SetStatus(_T("已添加一条自动按键。"));
}

void CKeyboardClickerDlg::OnRemoveClickAction()
{
    if (m_running)
    {
        SetStatus(_T("运行中不能减少自动按键。"));
        return;
    }
    if (m_clickActionCount <= 1)
    {
        SetStatus(_T("至少保留 1 条自动按键。"));
        return;
    }

    --m_clickActionCount;
    m_clickControls[m_clickActionCount].keyEdit.SetCapturedKey(0);
    m_clickControls[m_clickActionCount].intervalEdit.SetWindowText(_T("1000"));
    ClampScrollOffsets();
    ShowClickActionRows();
    LayoutControls();
    SaveSettings();
    SetStatus(_T("已减少一条自动按键。"));
}

void CKeyboardClickerDlg::InitializeShoutActionControls()
{
    CFont* font = GetFont();
    const DWORD editStyle = WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL;
    const DWORD readOnlyEditStyle = editStyle | ES_READONLY;
    const DWORD numberEditStyle = editStyle | ES_NUMBER;
    const DWORD labelStyle = WS_CHILD | SS_LEFT;

    for (size_t i = 0; i < MaxShoutActions; ++i)
    {
        const int y = ActionRowY + static_cast<int>(i) * ActionRowHeight;
        CString indexText;
        indexText.Format(_T("%u"), static_cast<unsigned>(i + 1));

        m_shoutControls[i].indexLabel.Create(indexText, labelStyle, CRect(24, y + 3, 42, y + 17), this, ActionControlId(i, 0));
        m_shoutControls[i].triggerKeyEdit.Create(readOnlyEditStyle, CRect(48, y, 94, y + 16), this, ActionControlId(i, 1));
        m_shoutControls[i].triggerCountEdit.Create(numberEditStyle, CRect(104, y, 132, y + 16), this, ActionControlId(i, 2));
        m_shoutControls[i].chatDelayEdit.Create(numberEditStyle, CRect(144, y, 178, y + 16), this, ActionControlId(i, 3));
        m_shoutControls[i].messageEdit.Create(editStyle, CRect(192, y, 372, y + 16), this, ActionControlId(i, 4));
        m_shoutControls[i].skillWaitEdit.Create(numberEditStyle, CRect(382, y, 426, y + 16), this, ActionControlId(i, 5));
        m_shoutControls[i].sequenceEdit.Create(editStyle, CRect(442, y, 582, y + 16), this, ActionControlId(i, 6));
        m_shoutControls[i].cooldownSkillKeyEdit.Create(readOnlyEditStyle, CRect(592, y, 642, y + 16), this, ActionControlId(i, 7));

        m_shoutControls[i].indexLabel.SetFont(font);
        m_shoutControls[i].triggerKeyEdit.SetFont(font);
        m_shoutControls[i].triggerCountEdit.SetFont(font);
        m_shoutControls[i].chatDelayEdit.SetFont(font);
        m_shoutControls[i].messageEdit.SetFont(font);
        m_shoutControls[i].skillWaitEdit.SetFont(font);
        m_shoutControls[i].sequenceEdit.SetFont(font);
        m_shoutControls[i].cooldownSkillKeyEdit.SetFont(font);
    }
}

void CKeyboardClickerDlg::ConfigureDefaultShoutActions()
{
    m_shoutActionCount = 3;

    m_shoutControls[0].triggerKeyEdit.SetCapturedKey(VK_F2);
    m_shoutControls[0].triggerCountEdit.SetWindowText(_T("2"));
    m_shoutControls[0].chatDelayEdit.SetWindowText(_T("30"));
    m_shoutControls[0].messageEdit.SetWindowText(_T("我要拉拽了"));
    m_shoutControls[0].skillWaitEdit.SetWindowText(_T("100"));
    m_shoutControls[0].sequenceEdit.SetWindowText(_T("F1,50,F3"));
    m_shoutControls[0].cooldownSkillKeyEdit.SetCapturedKey(VK_F3);

    m_shoutControls[1].triggerKeyEdit.SetCapturedKey(VK_F3);
    m_shoutControls[1].triggerCountEdit.SetWindowText(_T("1"));
    m_shoutControls[1].chatDelayEdit.SetWindowText(_T("30"));
    m_shoutControls[1].messageEdit.SetWindowText(_T("敌人来了"));
    m_shoutControls[1].skillWaitEdit.SetWindowText(_T("0"));
    m_shoutControls[1].sequenceEdit.SetWindowText(_T(""));
    m_shoutControls[1].cooldownSkillKeyEdit.SetCapturedKey(VK_F3);

    m_shoutControls[2].triggerKeyEdit.SetCapturedKey(VK_F4);
    m_shoutControls[2].triggerCountEdit.SetWindowText(_T("1"));
    m_shoutControls[2].chatDelayEdit.SetWindowText(_T("30"));
    m_shoutControls[2].messageEdit.SetWindowText(_T("注意敌人"));
    m_shoutControls[2].skillWaitEdit.SetWindowText(_T("0"));
    m_shoutControls[2].sequenceEdit.SetWindowText(_T(""));
    m_shoutControls[2].cooldownSkillKeyEdit.SetCapturedKey(VK_F4);

    for (size_t i = 3; i < MaxShoutActions; ++i)
    {
        m_shoutControls[i].triggerKeyEdit.SetCapturedKey(0);
        m_shoutControls[i].triggerCountEdit.SetWindowText(_T("1"));
        m_shoutControls[i].chatDelayEdit.SetWindowText(_T("30"));
        m_shoutControls[i].messageEdit.SetWindowText(_T(""));
        m_shoutControls[i].skillWaitEdit.SetWindowText(_T("0"));
        m_shoutControls[i].sequenceEdit.SetWindowText(_T(""));
        m_shoutControls[i].cooldownSkillKeyEdit.SetCapturedKey(0);
    }
}

void CKeyboardClickerDlg::InitializeAdvancedControls()
{
    CFont* font = GetFont();
    const DWORD labelStyle = WS_CHILD | SS_LEFT;
    const DWORD editStyle = WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL;
    const DWORD numberEditStyle = editStyle | ES_NUMBER;
    const DWORD readOnlyEditStyle = editStyle | ES_READONLY;
    const DWORD buttonStyle = WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON;
    const DWORD hiddenButtonStyle = buttonStyle;
    const DWORD checkStyle = WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX;

    m_pageTabs.Create(WS_CHILD | WS_TABSTOP | TCS_TABS, CRect(0, 0, 0, 0), this, IDC_PAGE_TABS);
    m_pageTabs.InsertItem(0, _T("基础功能"));
    m_pageTabs.InsertItem(1, _T("高级功能"));
    m_pageTabs.SetCurSel(0);
    m_pageTabs.SetFont(font);
    m_pageTabs.ShowWindow(SW_SHOW);

    m_advancedHintLabel.Create(_T("请先点击底部“高级功能”完成高级功能初始化。"), labelStyle, CRect(0, 0, 0, 0), this);
    m_hornGroup.Create(_T("号角"), WS_CHILD | BS_GROUPBOX, CRect(0, 0, 0, 0), this);
    m_hornEnabledCheck.Create(_T("手动检测"), hiddenButtonStyle, CRect(0, 0, 0, 0), this, IDC_HORN_ENABLED);
    m_hornIntervalLabel.Create(_T("检查间隔(ms):"), labelStyle, CRect(0, 0, 0, 0), this);
    m_hornIntervalEdit.Create(numberEditStyle, CRect(0, 0, 0, 0), this, IDC_HORN_INTERVAL);
    m_hornIntervalEdit.SetWindowText(_T("500"));
    m_hornSimilarityLabel.Create(_T("精准度:"), labelStyle, CRect(0, 0, 0, 0), this);
    m_hornSimilarityEdit.Create(editStyle, CRect(0, 0, 0, 0), this, IDC_HORN_SIMILARITY);
    m_hornSimilarityEdit.SetWindowText(_T("0.90"));
    m_hornSkillLabel.Create(_T("技能键:"), labelStyle, CRect(0, 0, 0, 0), this);
    m_hornSkillEdit.Create(readOnlyEditStyle, CRect(0, 0, 0, 0), this, IDC_HORN_SKILL_KEY);
    m_debuffRegionButton.Create(_T("Debuff范围"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_SELECT_DEBUFF_REGION);
    m_hornRegionButton.Create(_T("号角范围"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_SELECT_HORN_REGION);
    m_hornSampleButton.Create(_T("号角样本"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_ADD_HORN_SAMPLE);
    m_hornTestButton.Create(_T("按优先级检测"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_TEST_HORN);
    m_skillMonitorButton.Create(_T("开始"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_MONITOR_TOGGLE);
    m_debuffOptionsButton.Create(_T("Debuff样本"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_DEBUFF_OPTIONS);
    m_debuffRegionLabel.Create(_T("Debuff范围: 未设置"), labelStyle, CRect(0, 0, 0, 0), this);
    m_hornRegionLabel.Create(_T("号角范围: 未设置"), labelStyle, CRect(0, 0, 0, 0), this);
    m_debuffSummaryLabel.Create(_T("Debuff: 未选择"), labelStyle, CRect(0, 0, 0, 0), this);

    for (size_t i = 0; i < m_skillControls.size(); ++i)
    {
        AdvancedSkillControls& controls = m_skillControls[i];
        controls.group.Create(AdvancedSkillDefinitions[i].label, WS_CHILD | BS_GROUPBOX, CRect(0, 0, 0, 0), this);
        controls.enabledCheck.Create(_T("启用"), checkStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_ENABLED_BASE + static_cast<int>(i));
        controls.keyLabel.Create(_T("技能键:"), labelStyle, CRect(0, 0, 0, 0), this);
        controls.keyEdit.Create(readOnlyEditStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_KEY_BASE + static_cast<int>(i));
        controls.priorityLabel.Create(_T("优先级:"), labelStyle, CRect(0, 0, 0, 0), this);
        controls.priorityEdit.Create(numberEditStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_PRIORITY_BASE + static_cast<int>(i));
        controls.detectIntervalLabel.Create(_T("检测(ms):"), labelStyle, CRect(0, 0, 0, 0), this);
        controls.detectIntervalEdit.Create(numberEditStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_DETECT_INTERVAL_BASE + static_cast<int>(i));
        controls.intervalLabel.Create(_T("冷却(ms):"), labelStyle, CRect(0, 0, 0, 0), this);
        controls.intervalEdit.Create(numberEditStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_INTERVAL_BASE + static_cast<int>(i));
        controls.timingHelpLabel.Create(_T("检测=扫描频率；冷却=释放后防连按"), labelStyle, CRect(0, 0, 0, 0), this);
        controls.regionButton.Create(_T("识别范围"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_REGION_BASE + static_cast<int>(i));
        controls.sampleButton.Create(_T("添加样本"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_SAMPLE_BASE + static_cast<int>(i));
        controls.detectButton.Create(_T("手动检测"), buttonStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_DETECT_BASE + static_cast<int>(i));
        controls.regionLabel.Create(_T("识别范围: 未设置"), labelStyle, CRect(0, 0, 0, 0), this);
        for (size_t debuff = 0; debuff < controls.debuffChecks.size(); ++debuff)
        {
            CString debuffText(DebuffDefinitions[debuff].label);
            const int paren = debuffText.Find(L'(');
            if (paren > 0)
            {
                debuffText = debuffText.Left(paren);
            }
            controls.debuffChecks[debuff].Create(debuffText, checkStyle, CRect(0, 0, 0, 0), this, IDC_SKILL_DEBUFF_BASE + static_cast<int>(i * 4 + debuff));
        }
    }

    m_advancedHintLabel.SetFont(font);
    m_hornGroup.SetFont(font);
    m_hornEnabledCheck.SetFont(font);
    m_hornIntervalLabel.SetFont(font);
    m_hornIntervalEdit.SetFont(font);
    m_hornSimilarityLabel.SetFont(font);
    m_hornSimilarityEdit.SetFont(font);
    m_hornSkillLabel.SetFont(font);
    m_hornSkillEdit.SetFont(font);
    m_debuffRegionButton.SetFont(font);
    m_hornRegionButton.SetFont(font);
    m_hornSampleButton.SetFont(font);
    m_hornTestButton.SetFont(font);
    m_skillMonitorButton.SetFont(font);
    m_debuffOptionsButton.SetFont(font);
    m_debuffRegionLabel.SetFont(font);
    m_hornRegionLabel.SetFont(font);
    m_debuffSummaryLabel.SetFont(font);

    for (auto& controls : m_skillControls)
    {
        controls.group.SetFont(font);
        controls.enabledCheck.SetFont(font);
        controls.keyLabel.SetFont(font);
        controls.keyEdit.SetFont(font);
        controls.priorityLabel.SetFont(font);
        controls.priorityEdit.SetFont(font);
        controls.detectIntervalLabel.SetFont(font);
        controls.detectIntervalEdit.SetFont(font);
        controls.intervalLabel.SetFont(font);
        controls.intervalEdit.SetFont(font);
        controls.timingHelpLabel.SetFont(font);
        controls.regionButton.SetFont(font);
        controls.sampleButton.SetFont(font);
        controls.detectButton.SetFont(font);
        controls.regionLabel.SetFont(font);
        for (auto& check : controls.debuffChecks)
        {
            check.SetFont(font);
        }
    }

    for (size_t i = 0; i < m_debuffChecks.size(); ++i)
    {
        m_debuffChecks[i].Create(DebuffDefinitions[i].label, checkStyle, CRect(0, 0, 0, 0), this, IDC_DEBUFF_CHECK_BASE + static_cast<int>(i));
        m_debuffChecks[i].SetFont(font);
        CString sampleText(DebuffDefinitions[i].label);
        sampleText += _T("样本");
        m_debuffSampleButtons[i].Create(sampleText, buttonStyle, CRect(0, 0, 0, 0), this, IDC_DEBUFF_SAMPLE_BASE + static_cast<int>(i));
        m_debuffSampleButtons[i].SetFont(font);
    }

    UpdateDebuffSummary();
    ShowAdvancedPageControls(SW_HIDE);
}

void CKeyboardClickerDlg::ShowShoutActionRows()
{
    const BOOL canEditRows = (!m_running && !m_shoutWorkerActive.load()) ? TRUE : FALSE;
    if (CWnd* addButton = GetDlgItem(IDC_ADD_SHOUT_ACTION))
    {
        addButton->EnableWindow(canEditRows && m_shoutActionCount < MaxShoutActions);
    }
    if (CWnd* removeButton = GetDlgItem(IDC_REMOVE_SHOUT_ACTION))
    {
        removeButton->EnableWindow(canEditRows && m_shoutActionCount > 1);
    }
    ClampScrollOffsets();
    UpdateScrollBars();
}

void CKeyboardClickerDlg::ShowBasePageControls(int show)
{
    const int baseIds[] = {
        IDC_TARGET_GROUP, IDC_TARGET_HWND_LABEL, IDC_TARGET_HWND, IDC_TARGET_HINT, IDC_TARGET_TITLE_LABEL, IDC_TARGET_TITLE,
        IDC_CLICK_GROUP, IDC_ADD_CLICK_ACTION, IDC_REMOVE_CLICK_ACTION, IDC_CLICK_INDEX_HEADER, IDC_CLICK_KEY_HEADER,
        IDC_CLICK_INTERVAL_HEADER, IDC_CLICK_INDEX_HEADER_2, IDC_CLICK_KEY_HEADER_2, IDC_CLICK_INTERVAL_HEADER_2,
        IDC_RANDOM_DEVIATION_LABEL, IDC_RANDOM_DEVIATION, IDC_START, IDC_STOP, IDC_SHOUT_GROUP, IDC_ADD_SHOUT_ACTION, IDC_REMOVE_SHOUT_ACTION,
        IDC_SHOUT_INDEX_HEADER, IDC_SHOUT_KEY_HEADER, IDC_SHOUT_COUNT_HEADER, IDC_SHOUT_DELAY_HEADER, IDC_SHOUT_MESSAGE_HEADER,
        IDC_SHOUT_WAIT_HEADER, IDC_SHOUT_SEQUENCE_HEADER, IDC_SHOUT_COOLDOWN_HEADER
    };

    for (int id : baseIds)
    {
        if (CWnd* control = GetDlgItem(id))
        {
            control->ShowWindow(show);
        }
    }

    if (show == SW_HIDE)
    {
        for (auto& control : m_clickControls)
        {
            control.indexLabel.ShowWindow(SW_HIDE);
            control.keyEdit.ShowWindow(SW_HIDE);
            control.intervalEdit.ShowWindow(SW_HIDE);
        }
        for (auto& control : m_shoutControls)
        {
            control.indexLabel.ShowWindow(SW_HIDE);
            control.triggerKeyEdit.ShowWindow(SW_HIDE);
            control.triggerCountEdit.ShowWindow(SW_HIDE);
            control.chatDelayEdit.ShowWindow(SW_HIDE);
            control.messageEdit.ShowWindow(SW_HIDE);
            control.skillWaitEdit.ShowWindow(SW_HIDE);
            control.sequenceEdit.ShowWindow(SW_HIDE);
            control.cooldownSkillKeyEdit.ShowWindow(SW_HIDE);
        }

        if (m_clickScrollBar.GetSafeHwnd())
        {
            m_clickScrollBar.ShowWindow(SW_HIDE);
        }
        if (m_shoutScrollBar.GetSafeHwnd())
        {
            m_shoutScrollBar.ShowWindow(SW_HIDE);
        }
    }
}

void CKeyboardClickerDlg::ShowAdvancedPageControls(int show)
{
    m_advancedHintLabel.ShowWindow(show);

    const int advancedShow = show == SW_SHOW && m_advancedInitialized ? SW_SHOW : SW_HIDE;
    m_hornGroup.ShowWindow(SW_HIDE);
    m_hornEnabledCheck.ShowWindow(SW_HIDE);
    m_hornIntervalLabel.ShowWindow(SW_HIDE);
    m_hornIntervalEdit.ShowWindow(SW_HIDE);
    m_hornSimilarityLabel.ShowWindow(advancedShow);
    m_hornSimilarityEdit.ShowWindow(advancedShow);
    m_hornSkillLabel.ShowWindow(SW_HIDE);
    m_hornSkillEdit.ShowWindow(SW_HIDE);
    m_debuffRegionButton.ShowWindow(advancedShow);
    m_hornRegionButton.ShowWindow(SW_HIDE);
    m_hornSampleButton.ShowWindow(SW_HIDE);
    m_hornTestButton.ShowWindow(advancedShow);
    m_skillMonitorButton.ShowWindow(advancedShow);
    m_debuffOptionsButton.ShowWindow(advancedShow);
    m_debuffRegionLabel.ShowWindow(advancedShow);
    m_hornRegionLabel.ShowWindow(SW_HIDE);
    m_debuffSummaryLabel.ShowWindow(advancedShow);

    for (auto& controls : m_skillControls)
    {
        controls.group.ShowWindow(advancedShow);
        controls.enabledCheck.ShowWindow(advancedShow);
        controls.keyLabel.ShowWindow(advancedShow);
        controls.keyEdit.ShowWindow(advancedShow);
        controls.priorityLabel.ShowWindow(advancedShow);
        controls.priorityEdit.ShowWindow(advancedShow);
        controls.detectIntervalLabel.ShowWindow(advancedShow);
        controls.detectIntervalEdit.ShowWindow(advancedShow);
        controls.intervalLabel.ShowWindow(advancedShow);
        controls.intervalEdit.ShowWindow(advancedShow);
        controls.timingHelpLabel.ShowWindow(advancedShow);
        controls.regionButton.ShowWindow(advancedShow);
        controls.sampleButton.ShowWindow(advancedShow);
        controls.detectButton.ShowWindow(advancedShow);
        controls.regionLabel.ShowWindow(advancedShow);
        for (auto& check : controls.debuffChecks)
        {
            check.ShowWindow(advancedShow);
        }
    }

    const int debuffShow = advancedShow == SW_SHOW && m_hornState.debuffPanelVisible ? SW_SHOW : SW_HIDE;
    for (auto& check : m_debuffChecks)
    {
        check.ShowWindow(SW_HIDE);
    }
    for (auto& button : m_debuffSampleButtons)
    {
        button.ShowWindow(debuffShow);
    }
}

void CKeyboardClickerDlg::UpdateAdvancedControls()
{
    m_hornEnabledCheck.SetCheck(m_hornState.enabled ? BST_CHECKED : BST_UNCHECKED);
    CString hornIntervalText;
    hornIntervalText.Format(_T("%u"), static_cast<unsigned>(m_hornState.intervalMs));
    if (GetWindowTextString(m_hornIntervalEdit).IsEmpty())
    {
        m_hornIntervalEdit.SetWindowText(hornIntervalText);
    }
    if (GetWindowTextString(m_hornSimilarityEdit).IsEmpty())
    {
        CString similarity;
        similarity.Format(_T("%.2f"), m_hornState.similarity);
        m_hornSimilarityEdit.SetWindowText(similarity);
    }
    for (size_t i = 0; i < m_skillControls.size(); ++i)
    {
        m_skillControls[i].keyEdit.SetCapturedKey(m_skillStates[i].skillVk);
        m_skillControls[i].enabledCheck.SetCheck(m_skillStates[i].enabled ? BST_CHECKED : BST_UNCHECKED);
        CString priority;
        priority.Format(_T("%u"), static_cast<unsigned>(m_skillStates[i].priority == 0 ? DefaultSkillPriorities[i] : m_skillStates[i].priority));
        m_skillControls[i].priorityEdit.SetWindowText(priority);
        CString skillDetectIntervalText;
        skillDetectIntervalText.Format(_T("%u"), static_cast<unsigned>(m_skillStates[i].detectIntervalMs < MinSkillDetectIntervalMs ? DefaultSkillDetectIntervalMs : m_skillStates[i].detectIntervalMs));
        m_skillControls[i].detectIntervalEdit.SetWindowText(skillDetectIntervalText);
        CString skillReleaseIntervalText;
        skillReleaseIntervalText.Format(_T("%u"), static_cast<unsigned>(m_skillStates[i].releaseIntervalMs < MinSkillReleaseIntervalMs ? DefaultSkillReleaseIntervalMs : m_skillStates[i].releaseIntervalMs));
        m_skillControls[i].intervalEdit.SetWindowText(skillReleaseIntervalText);

        CString emptyText;
        emptyText.Format(_T("%s范围: 未设置"), AdvancedSkillDefinitions[i].label);
        SetRegionStatus(m_skillControls[i].regionLabel, m_skillStates[i].hasRegion, m_skillStates[i].region, emptyText);
        for (size_t debuff = 0; debuff < m_skillStates[i].debuffs.size(); ++debuff)
        {
            m_skillControls[i].debuffChecks[debuff].SetCheck(m_skillStates[i].debuffs[debuff] ? BST_CHECKED : BST_UNCHECKED);
        }
    }
    for (size_t i = 0; i < m_debuffChecks.size(); ++i)
    {
        m_debuffChecks[i].SetCheck(m_hornState.debuffs[i] ? BST_CHECKED : BST_UNCHECKED);
    }
    SetRegionStatus(m_debuffRegionLabel, m_hornState.hasDebuffRegion, m_hornState.debuffRegion, _T("Debuff范围: 未设置"));
    UpdateDebuffSummary();
}

void CKeyboardClickerDlg::UpdateSkillMonitorButtonText()
{
    if (m_skillMonitorButton.GetSafeHwnd())
    {
        m_skillMonitorButton.SetWindowText(m_hornState.enabled ? _T("结束") : _T("开始"));
        m_skillMonitorButton.EnableWindow(m_advancedInitialized ? TRUE : FALSE);
    }
}

void CKeyboardClickerDlg::UpdateDebuffSummary()
{
    if (m_debuffSummaryLabel.GetSafeHwnd())
    {
        m_debuffSummaryLabel.SetWindowText(_T("适用 Debuff: 每个技能卡片内单独勾选；按优先级检测只会释放一个可用技能。"));
    }
}

void CKeyboardClickerDlg::OnAddShoutAction()
{
    if (m_running || m_shoutWorkerActive.load())
    {
        SetStatus(_T("运行中不能添加喊话动作。"));
        return;
    }

    if (m_shoutActionCount >= MaxShoutActions)
    {
        CString message;
        message.Format(_T("最多支持 %u 条喊话动作。"), static_cast<unsigned>(MaxShoutActions));
        SetStatus(message);
        return;
    }

    ++m_shoutActionCount;
    const size_t capacity = VisibleShoutRowCapacity();
    if (m_shoutActionCount > capacity)
    {
        m_shoutScrollOffset = m_shoutActionCount - capacity;
    }
    ShowShoutActionRows();
    LayoutControls();
    RegisterShoutHotkey(m_shoutActionCount - 1);
    SaveSettings();
    SetStatus(_T("已添加一条喊话动作。"));
}

void CKeyboardClickerDlg::OnRemoveShoutAction()
{
    if (m_running || m_shoutWorkerActive.load())
    {
        SetStatus(_T("运行中不能减少喊话动作。"));
        return;
    }

    if (m_shoutActionCount <= 1)
    {
        SetStatus(_T("至少保留 1 条喊话动作。"));
        return;
    }

    const size_t removedIndex = m_shoutActionCount - 1;
    UnregisterShoutHotkey(removedIndex);
    --m_shoutActionCount;

    m_shoutControls[removedIndex].triggerKeyEdit.SetCapturedKey(0);
    m_shoutControls[removedIndex].triggerCountEdit.SetWindowText(_T("1"));
    m_shoutControls[removedIndex].chatDelayEdit.SetWindowText(_T("30"));
    m_shoutControls[removedIndex].messageEdit.SetWindowText(_T(""));
    m_shoutControls[removedIndex].skillWaitEdit.SetWindowText(_T("0"));
    m_shoutControls[removedIndex].sequenceEdit.SetWindowText(_T(""));
    m_shoutControls[removedIndex].cooldownSkillKeyEdit.SetCapturedKey(0);
    m_shoutStates[removedIndex] = {};

    ClampScrollOffsets();
    ShowShoutActionRows();
    LayoutControls();
    SaveSettings();
    SetStatus(_T("已减少一条喊话动作。"));
}

void CKeyboardClickerDlg::LoadSettings()
{
    CWinApp* app = AfxGetApp();
    if (app == nullptr)
    {
        return;
    }

    SetDlgItemText(IDC_RANDOM_DEVIATION, app->GetProfileString(SettingsSection, _T("RandomDeviation"), _T("0")));

    const int clickCount = app->GetProfileInt(SettingsSection, _T("ClickActionCount"), static_cast<int>(m_clickActionCount));
    if (clickCount >= 1 && clickCount <= static_cast<int>(MaxClickActions))
    {
        m_clickActionCount = static_cast<size_t>(clickCount);
    }

    for (size_t i = 0; i < MaxClickActions; ++i)
    {
        CString prefix;
        prefix.Format(_T("Click%u"), static_cast<unsigned>(i + 1));

        int keyValue = app->GetProfileInt(SettingsSection, prefix + _T("Key"), -1);
        if (keyValue < 0 && i < 3)
        {
            CString oldKeyName;
            oldKeyName.Format(_T("Key%u"), static_cast<unsigned>(i + 1));
            keyValue = app->GetProfileInt(SettingsSection, oldKeyName, static_cast<int>(m_clickControls[i].keyEdit.CapturedKey()));
        }

        const UINT key = static_cast<UINT>(keyValue);
        if (keyValue >= 0 && key != 0)
        {
            m_clickControls[i].keyEdit.SetCapturedKey(key);
        }

        CString intervalDefault = GetWindowTextString(m_clickControls[i].intervalEdit);
        if (i < 3)
        {
            CString oldIntervalName;
            oldIntervalName.Format(_T("Interval%u"), static_cast<unsigned>(i + 1));
            intervalDefault = app->GetProfileString(SettingsSection, oldIntervalName, intervalDefault);
        }
        m_clickControls[i].intervalEdit.SetWindowText(app->GetProfileString(SettingsSection, prefix + _T("Interval"), intervalDefault));
    }

    const int actionCount = app->GetProfileInt(SettingsSection, _T("ShoutActionCount"), static_cast<int>(m_shoutActionCount));
    if (actionCount >= 1 && actionCount <= static_cast<int>(MaxShoutActions))
    {
        m_shoutActionCount = static_cast<size_t>(actionCount);
    }

    for (size_t i = 0; i < MaxShoutActions; ++i)
    {
        CString prefix;
        prefix.Format(_T("Shout%u"), static_cast<unsigned>(i + 1));

        CString keyName = prefix + _T("Key");
        CString countName = prefix + _T("Count");
        CString delayName = prefix + _T("Delay");
        CString textName = prefix + _T("Text");
        CString waitName = prefix + _T("SkillWait");
        CString sequenceName = prefix + _T("Sequence");
        CString cooldownName = prefix + _T("CooldownKey");

        const UINT key = static_cast<UINT>(app->GetProfileInt(SettingsSection, keyName, m_shoutControls[i].triggerKeyEdit.CapturedKey()));
        m_shoutControls[i].triggerKeyEdit.SetCapturedKey(key);
        m_shoutControls[i].triggerCountEdit.SetWindowText(app->GetProfileString(SettingsSection, countName, GetWindowTextString(m_shoutControls[i].triggerCountEdit)));
        m_shoutControls[i].chatDelayEdit.SetWindowText(app->GetProfileString(SettingsSection, delayName, GetWindowTextString(m_shoutControls[i].chatDelayEdit)));
        m_shoutControls[i].messageEdit.SetWindowText(app->GetProfileString(SettingsSection, textName, GetWindowTextString(m_shoutControls[i].messageEdit)));
        m_shoutControls[i].skillWaitEdit.SetWindowText(app->GetProfileString(SettingsSection, waitName, GetWindowTextString(m_shoutControls[i].skillWaitEdit)));
        m_shoutControls[i].sequenceEdit.SetWindowText(app->GetProfileString(SettingsSection, sequenceName, GetWindowTextString(m_shoutControls[i].sequenceEdit)));

        const UINT cooldownKey = static_cast<UINT>(app->GetProfileInt(SettingsSection, cooldownName, m_shoutControls[i].cooldownSkillKeyEdit.CapturedKey()));
        m_shoutControls[i].cooldownSkillKeyEdit.SetCapturedKey(cooldownKey);
    }

    m_activePage = app->GetProfileInt(SettingsSection, _T("ActivePage"), 0) == 1 ? 1 : 0;
    m_advancedAuthorized = app->GetProfileInt(SettingsSection, _T("AdvancedAuthorized"), 0) != 0;
    m_hornState.enabled = false;
    m_hornState.intervalMs = static_cast<UINT>(app->GetProfileInt(SettingsSection, _T("HornInterval"), 500));
    if (m_hornState.intervalMs < 50)
    {
        m_hornState.intervalMs = 50;
    }
    CString similarityText = app->GetProfileString(SettingsSection, _T("HornSimilarity"), _T("0.90"));
    wchar_t* similarityEnd = nullptr;
    const double savedSimilarity = wcstod(similarityText, &similarityEnd);
    if (similarityEnd != static_cast<LPCWSTR>(similarityText) && savedSimilarity >= 0.1 && savedSimilarity <= 1.0)
    {
        m_hornState.similarity = savedSimilarity;
    }
    m_hornState.hasDebuffRegion = app->GetProfileInt(SettingsSection, _T("HornHasDebuffRegion"), 0) != 0;
    m_hornState.debuffRegion.left = app->GetProfileInt(SettingsSection, _T("HornDebuffLeft"), 0);
    m_hornState.debuffRegion.top = app->GetProfileInt(SettingsSection, _T("HornDebuffTop"), 0);
    m_hornState.debuffRegion.right = app->GetProfileInt(SettingsSection, _T("HornDebuffRight"), 0);
    m_hornState.debuffRegion.bottom = app->GetProfileInt(SettingsSection, _T("HornDebuffBottom"), 0);

    for (size_t i = 0; i < m_skillStates.size(); ++i)
    {
        const CString prefix(AdvancedSkillDefinitions[i].settingsPrefix);
        CString keyName = prefix + _T("SkillKey");
        CString hasRegionName = prefix + _T("HasRegion");
        CString leftName = prefix + _T("RegionLeft");
        CString topName = prefix + _T("RegionTop");
        CString rightName = prefix + _T("RegionRight");
        CString bottomName = prefix + _T("RegionBottom");
        CString priorityName = prefix + _T("Priority");
        CString enabledName = prefix + _T("Enabled");
        CString intervalName = prefix + _T("Interval");
        CString detectIntervalName = prefix + _T("DetectInterval");

        if (i == 0)
        {
            m_skillStates[i].skillVk = static_cast<UINT>(app->GetProfileInt(SettingsSection, keyName, app->GetProfileInt(SettingsSection, _T("HornSkillKey"), 0)));
            m_skillStates[i].hasRegion = app->GetProfileInt(SettingsSection, hasRegionName, app->GetProfileInt(SettingsSection, _T("HornHasHornRegion"), 0)) != 0;
            m_skillStates[i].region.left = app->GetProfileInt(SettingsSection, leftName, app->GetProfileInt(SettingsSection, _T("HornHornLeft"), 0));
            m_skillStates[i].region.top = app->GetProfileInt(SettingsSection, topName, app->GetProfileInt(SettingsSection, _T("HornHornTop"), 0));
            m_skillStates[i].region.right = app->GetProfileInt(SettingsSection, rightName, app->GetProfileInt(SettingsSection, _T("HornHornRight"), 0));
            m_skillStates[i].region.bottom = app->GetProfileInt(SettingsSection, bottomName, app->GetProfileInt(SettingsSection, _T("HornHornBottom"), 0));
        }
        else
        {
            m_skillStates[i].skillVk = static_cast<UINT>(app->GetProfileInt(SettingsSection, keyName, 0));
            m_skillStates[i].hasRegion = app->GetProfileInt(SettingsSection, hasRegionName, 0) != 0;
            m_skillStates[i].region.left = app->GetProfileInt(SettingsSection, leftName, 0);
            m_skillStates[i].region.top = app->GetProfileInt(SettingsSection, topName, 0);
            m_skillStates[i].region.right = app->GetProfileInt(SettingsSection, rightName, 0);
            m_skillStates[i].region.bottom = app->GetProfileInt(SettingsSection, bottomName, 0);
        }
        m_skillStates[i].priority = static_cast<UINT>(app->GetProfileInt(SettingsSection, priorityName, static_cast<int>(DefaultSkillPriorities[i])));
        if (m_skillStates[i].priority == 0)
        {
            m_skillStates[i].priority = DefaultSkillPriorities[i];
        }
        m_skillStates[i].detectIntervalMs = static_cast<UINT>(app->GetProfileInt(SettingsSection, detectIntervalName, DefaultSkillDetectIntervalMs));
        if (m_skillStates[i].detectIntervalMs < MinSkillDetectIntervalMs)
        {
            m_skillStates[i].detectIntervalMs = DefaultSkillDetectIntervalMs;
        }
        m_skillStates[i].releaseIntervalMs = static_cast<UINT>(app->GetProfileInt(SettingsSection, intervalName, DefaultSkillReleaseIntervalMs));
        if (m_skillStates[i].releaseIntervalMs < MinSkillReleaseIntervalMs)
        {
            m_skillStates[i].releaseIntervalMs = DefaultSkillReleaseIntervalMs;
        }
        m_skillStates[i].enabled = app->GetProfileInt(SettingsSection, enabledName, 1) != 0;
        for (size_t debuff = 0; debuff < m_skillStates[i].debuffs.size(); ++debuff)
        {
            CString debuffName;
            debuffName.Format(_T("%sDebuff%u"), AdvancedSkillDefinitions[i].settingsPrefix, static_cast<unsigned>(debuff));
            m_skillStates[i].debuffs[debuff] = app->GetProfileInt(SettingsSection, debuffName, DefaultSkillDebuffs[i][debuff] ? 1 : 0) != 0;
        }
    }
    for (size_t i = 0; i < m_hornState.debuffs.size(); ++i)
    {
        CString name;
        name.Format(_T("HornDebuff%u"), static_cast<unsigned>(i));
        m_hornState.debuffs[i] = app->GetProfileInt(SettingsSection, name, 0) != 0;
    }
    CString intervalText;
    intervalText.Format(_T("%u"), static_cast<unsigned>(m_hornState.intervalMs));
    m_hornIntervalEdit.SetWindowText(intervalText);
    similarityText.Format(_T("%.2f"), m_hornState.similarity);
    m_hornSimilarityEdit.SetWindowText(similarityText);
    UpdateAdvancedControls();
}

void CKeyboardClickerDlg::SaveSettings() const
{
    CWinApp* app = AfxGetApp();
    if (app == nullptr || !GetSafeHwnd())
    {
        return;
    }

    CString randomDeviation;
    GetDlgItemText(IDC_RANDOM_DEVIATION, randomDeviation);
    app->WriteProfileString(SettingsSection, _T("RandomDeviation"), randomDeviation);

    app->WriteProfileInt(SettingsSection, _T("ClickActionCount"), static_cast<int>(m_clickActionCount));
    for (size_t i = 0; i < MaxClickActions; ++i)
    {
        CString prefix;
        prefix.Format(_T("Click%u"), static_cast<unsigned>(i + 1));

        app->WriteProfileInt(SettingsSection, prefix + _T("Key"), static_cast<int>(m_clickControls[i].keyEdit.CapturedKey()));
        app->WriteProfileString(SettingsSection, prefix + _T("Interval"), GetWindowTextString(m_clickControls[i].intervalEdit));
    }

    app->WriteProfileInt(SettingsSection, _T("ShoutActionCount"), static_cast<int>(m_shoutActionCount));

    for (size_t i = 0; i < MaxShoutActions; ++i)
    {
        CString prefix;
        prefix.Format(_T("Shout%u"), static_cast<unsigned>(i + 1));

        app->WriteProfileInt(SettingsSection, prefix + _T("Key"), static_cast<int>(m_shoutControls[i].triggerKeyEdit.CapturedKey()));
        app->WriteProfileString(SettingsSection, prefix + _T("Count"), GetWindowTextString(m_shoutControls[i].triggerCountEdit));
        app->WriteProfileString(SettingsSection, prefix + _T("Delay"), GetWindowTextString(m_shoutControls[i].chatDelayEdit));
        app->WriteProfileString(SettingsSection, prefix + _T("Text"), GetWindowTextString(m_shoutControls[i].messageEdit));
        app->WriteProfileString(SettingsSection, prefix + _T("SkillWait"), GetWindowTextString(m_shoutControls[i].skillWaitEdit));
        app->WriteProfileString(SettingsSection, prefix + _T("Sequence"), GetWindowTextString(m_shoutControls[i].sequenceEdit));
        app->WriteProfileInt(SettingsSection, prefix + _T("CooldownKey"), static_cast<int>(m_shoutControls[i].cooldownSkillKeyEdit.CapturedKey()));
    }

    app->WriteProfileInt(SettingsSection, _T("ActivePage"), m_activePage);
    app->WriteProfileInt(SettingsSection, _T("AdvancedAuthorized"), m_advancedAuthorized ? 1 : 0);
    app->WriteProfileInt(SettingsSection, _T("HornEnabled"), 0);
    UINT hornInterval = 0;
    if (!ReadUIntFromWindow(m_hornIntervalEdit, hornInterval) || hornInterval < 50)
    {
        hornInterval = m_hornState.intervalMs < 50 ? 500 : m_hornState.intervalMs;
    }
    app->WriteProfileInt(SettingsSection, _T("HornInterval"), static_cast<int>(hornInterval));
    app->WriteProfileString(SettingsSection, _T("HornSimilarity"), GetWindowTextString(m_hornSimilarityEdit));
    app->WriteProfileInt(SettingsSection, _T("HornSkillKey"), static_cast<int>(m_skillStates[0].skillVk));
    app->WriteProfileInt(SettingsSection, _T("HornHasDebuffRegion"), m_hornState.hasDebuffRegion ? 1 : 0);
    app->WriteProfileInt(SettingsSection, _T("HornDebuffLeft"), m_hornState.debuffRegion.left);
    app->WriteProfileInt(SettingsSection, _T("HornDebuffTop"), m_hornState.debuffRegion.top);
    app->WriteProfileInt(SettingsSection, _T("HornDebuffRight"), m_hornState.debuffRegion.right);
    app->WriteProfileInt(SettingsSection, _T("HornDebuffBottom"), m_hornState.debuffRegion.bottom);
    app->WriteProfileInt(SettingsSection, _T("HornHasHornRegion"), m_skillStates[0].hasRegion ? 1 : 0);
    app->WriteProfileInt(SettingsSection, _T("HornHornLeft"), m_skillStates[0].region.left);
    app->WriteProfileInt(SettingsSection, _T("HornHornTop"), m_skillStates[0].region.top);
    app->WriteProfileInt(SettingsSection, _T("HornHornRight"), m_skillStates[0].region.right);
    app->WriteProfileInt(SettingsSection, _T("HornHornBottom"), m_skillStates[0].region.bottom);
    for (size_t i = 0; i < m_skillStates.size(); ++i)
    {
        const CString prefix(AdvancedSkillDefinitions[i].settingsPrefix);
        app->WriteProfileInt(SettingsSection, prefix + _T("SkillKey"), static_cast<int>(m_skillStates[i].skillVk));
        app->WriteProfileInt(SettingsSection, prefix + _T("HasRegion"), m_skillStates[i].hasRegion ? 1 : 0);
        app->WriteProfileInt(SettingsSection, prefix + _T("RegionLeft"), m_skillStates[i].region.left);
        app->WriteProfileInt(SettingsSection, prefix + _T("RegionTop"), m_skillStates[i].region.top);
        app->WriteProfileInt(SettingsSection, prefix + _T("RegionRight"), m_skillStates[i].region.right);
        app->WriteProfileInt(SettingsSection, prefix + _T("RegionBottom"), m_skillStates[i].region.bottom);
        UINT priority = 0;
        if (!ReadUIntFromWindow(m_skillControls[i].priorityEdit, priority) || priority == 0)
        {
            priority = m_skillStates[i].priority == 0 ? DefaultSkillPriorities[i] : m_skillStates[i].priority;
        }
        app->WriteProfileInt(SettingsSection, prefix + _T("Priority"), static_cast<int>(priority));
        UINT detectInterval = 0;
        if (!ReadUIntFromWindow(m_skillControls[i].detectIntervalEdit, detectInterval) || detectInterval < MinSkillDetectIntervalMs)
        {
            detectInterval = m_skillStates[i].detectIntervalMs < MinSkillDetectIntervalMs ? DefaultSkillDetectIntervalMs : m_skillStates[i].detectIntervalMs;
        }
        app->WriteProfileInt(SettingsSection, prefix + _T("DetectInterval"), static_cast<int>(detectInterval));
        UINT releaseInterval = 0;
        if (!ReadUIntFromWindow(m_skillControls[i].intervalEdit, releaseInterval) || releaseInterval < MinSkillReleaseIntervalMs)
        {
            releaseInterval = m_skillStates[i].releaseIntervalMs < MinSkillReleaseIntervalMs ? DefaultSkillReleaseIntervalMs : m_skillStates[i].releaseIntervalMs;
        }
        app->WriteProfileInt(SettingsSection, prefix + _T("Interval"), static_cast<int>(releaseInterval));
        app->WriteProfileInt(SettingsSection, prefix + _T("Enabled"), m_skillControls[i].enabledCheck.GetCheck() == BST_CHECKED ? 1 : 0);
        for (size_t debuff = 0; debuff < m_skillStates[i].debuffs.size(); ++debuff)
        {
            CString debuffName;
            debuffName.Format(_T("%sDebuff%u"), AdvancedSkillDefinitions[i].settingsPrefix, static_cast<unsigned>(debuff));
            app->WriteProfileInt(SettingsSection, debuffName, m_skillControls[i].debuffChecks[debuff].GetCheck() == BST_CHECKED ? 1 : 0);
        }
    }
    for (size_t i = 0; i < m_hornState.debuffs.size(); ++i)
    {
        CString name;
        name.Format(_T("HornDebuff%u"), static_cast<unsigned>(i));
        app->WriteProfileInt(SettingsSection, name, m_hornState.debuffs[i] ? 1 : 0);
    }
}

bool CKeyboardClickerDlg::ReadJobs()
{
    if (!ReadUInt(IDC_RANDOM_DEVIATION, m_randomDeviationMs))
    {
        SetStatus(_T("随机偏差必须是 0 或更大的毫秒数。"));
        return false;
    }

    for (size_t i = 0; i < m_jobs.size(); ++i)
    {
        m_jobs[i] = {};
        if (i >= m_clickActionCount)
        {
            continue;
        }

        const UINT vk = m_clickControls[i].keyEdit.CapturedKey();
        if (vk == 0)
        {
            continue;
        }

        UINT intervalMs = 0;
        if (!ReadUIntFromWindow(m_clickControls[i].intervalEdit, intervalMs) || intervalMs == 0)
        {
            CString message;
            message.Format(_T("自动按键 %u 的间隔必须大于 0 毫秒。"), static_cast<unsigned>(i + 1));
            SetStatus(message);
            return false;
        }

        m_jobs[i].vk = vk;
        m_jobs[i].intervalMs = intervalMs;
        m_jobs[i].enabled = true;
    }

    return true;
}

bool CKeyboardClickerDlg::SendKey(UINT vk)
{
    if (!PrepareForegroundTarget())
    {
        StopJobs();
        return false;
    }

    return SendInputKey(vk);
}

bool CKeyboardClickerDlg::SendShoutMessage(size_t index)
{
    return QueueShoutMessage(index, 0);
}

bool CKeyboardClickerDlg::QueueShoutMessage(size_t index, UINT startDelayMs)
{
    if (index >= m_shoutActionCount)
    {
        return false;
    }
    if (m_shoutWorkerActive.load())
    {
        SetStatus(_T("上一条喊话动作仍在执行，请稍后再试。"));
        return false;
    }

    const ULONGLONG now = ::GetTickCount64();
    if (m_shoutStates[index].lastMessageTick != 0 && now - m_shoutStates[index].lastMessageTick < ShoutCooldownMs)
    {
        const ULONGLONG remainingMs = ShoutCooldownMs - (now - m_shoutStates[index].lastMessageTick);
        CString message;
        message.Format(_T("报点冷却中，还剩 %u 秒。"), static_cast<unsigned>((remainingMs + 999) / 1000));
        SetStatus(message);
        return false;
    }

    if (!IsWindow(m_targetHwnd))
    {
        SetStatus(_T("请先把鼠标移到有效目标窗口/控件上，然后按 F9。"));
        return false;
    }

    ShoutWorkerTask task;
    task.ownerHwnd = m_hWnd;
    task.targetHwnd = m_targetHwnd;
    task.startDelayMs = startDelayMs;
    m_shoutControls[index].messageEdit.GetWindowText(task.text);
    task.text.Trim();
    if (task.text.IsEmpty())
    {
        SetStatus(_T("团队消息不能为空。"));
        return false;
    }

    if (!ReadUIntFromWindow(m_shoutControls[index].chatDelayEdit, task.reportDelayMs) || task.reportDelayMs > 1000)
    {
        SetStatus(_T("聊天间隔必须是 0 到 1000 毫秒。"));
        return false;
    }

    m_shoutControls[index].sequenceEdit.GetWindowText(task.sequence);
    task.sequence.Trim();
    SaveSettings();

    m_shoutStates[index].lastMessageTick = now;
    UnregisterAllShoutHotkeys();
    m_shoutWorkerActive.store(true);
    ShowShoutActionRows();
    JoinFinishedAsyncWorkers();
    if (m_shoutWorker.joinable())
    {
        m_shoutWorker.join();
    }

    HWND hwnd = m_hWnd;
    m_shoutWorker = std::thread([this, hwnd, task]() {
        std::unique_ptr<ShoutWorkerResult> result(new ShoutWorkerResult());

        if (!WorkerPrepareForegroundTarget(task.targetHwnd))
        {
            result->message = _T("目标窗口无效，喊话已取消。");
        }
        else
        {
            CString oldClipboardText;
            const bool hadClipboardText = WorkerReadClipboardText(task.ownerHwnd, oldClipboardText);
            if (!WorkerSetAndVerifyClipboardText(task.ownerHwnd, task.text))
            {
                if (hadClipboardText)
                {
                    WorkerSetClipboardText(task.ownerHwnd, oldClipboardText);
                }
                else
                {
                    WorkerClearClipboard(task.ownerHwnd);
                }
                result->message = _T("剪贴板未更新为报点文字，已取消发送。");
            }
            else
            {
                Sleep(30);
                const bool sent = WorkerSendInputKey(VK_RETURN)
                    && (Sleep(task.reportDelayMs), WorkerSendInputShortcut(VK_CONTROL, 'V'))
                    && (Sleep(task.reportDelayMs), WorkerSendInputKey(VK_RETURN));
                Sleep(task.reportDelayMs);

                if (hadClipboardText)
                {
                    WorkerSetClipboardText(task.ownerHwnd, oldClipboardText);
                }
                else
                {
                    WorkerClearClipboard(task.ownerHwnd);
                }

                if (!sent)
                {
                    result->message = _T("发送报点输入失败。");
                }
                else
                {
                    if (task.startDelayMs > 0)
                    {
                        Sleep(task.startDelayMs);
                    }

                    bool sequenceOk = true;
                    CString sequenceError;
                    CString sequence(task.sequence);
                    int current = 0;
                    CString token = sequence.Tokenize(_T(",; \t\r\n"), current);
                    while (!token.IsEmpty())
                    {
                        token.Trim();
                        if (!token.IsEmpty() && !WorkerExecuteSequenceToken(token, sequenceError))
                        {
                            sequenceOk = false;
                            break;
                        }
                        token = sequence.Tokenize(_T(",; \t\r\n"), current);
                    }

                    result->success = sequenceOk;
                    result->message = sequenceOk ? CString(_T("报点和技能序列已执行。")) : sequenceError;
                }
            }
        }

        m_shoutWorkerActive.store(false);
        if (!m_closing.load() && ::IsWindow(hwnd))
        {
            ::PostMessage(hwnd, WM_APP_SHOUT_WORKER_FINISHED, 0, reinterpret_cast<LPARAM>(result.release()));
        }
    });

    SetStatus(startDelayMs > 0 ? _T("喊话任务已提交到后台，稍后执行技能序列。") : _T("喊话任务已提交到后台。"));
    return true;
}

void CKeyboardClickerDlg::HandleShoutHotkey(size_t index)
{
    if (index >= m_shoutActionCount)
    {
        return;
    }

    const ULONGLONG now = ::GetTickCount64();

    if (m_shoutStates[index].lastMessageTick != 0 && now - m_shoutStates[index].lastMessageTick < ShoutCooldownMs)
    {
        TriggerCooldownSkill(index);
        return;
    }

    UINT requiredPresses = 0;
    if (!ReadUIntFromWindow(m_shoutControls[index].triggerCountEdit, requiredPresses) || requiredPresses == 0 || requiredPresses > 10)
    {
        SetStatus(_T("触发次数必须是 1 到 10。"));
        return;
    }
    SaveSettings();

    if (m_shoutStates[index].lastPressTick == 0 || now - m_shoutStates[index].lastPressTick > ShoutSequenceMs)
    {
        m_shoutStates[index].pressCount = 0;
    }

    m_shoutStates[index].lastPressTick = now;
    ++m_shoutStates[index].pressCount;

    if (m_shoutStates[index].pressCount >= requiredPresses)
    {
        UINT startDelayMs = 0;
        if (!ReadUIntFromWindow(m_shoutControls[index].skillWaitEdit, startDelayMs) || startDelayMs > 3000)
        {
            SetStatus(_T("技能前等待必须是 0 到 3000 毫秒。"));
            return;
        }

        m_shoutStates[index].pressCount = 0;
        m_shoutStates[index].lastPressTick = 0;
        SaveSettings();

        QueueShoutMessage(index, startDelayMs);
        return;
    }

    CString message;
    message.Format(_T("喊话动作 %u 已按 %u/%u 次。"), static_cast<unsigned>(index + 1), static_cast<unsigned>(m_shoutStates[index].pressCount), static_cast<unsigned>(requiredPresses));
    SetStatus(message);
}

void CKeyboardClickerDlg::TriggerCooldownSkill(size_t index)
{
    if (index >= m_shoutActionCount)
    {
        return;
    }

    if (!PrepareForegroundTarget())
    {
        return;
    }

    const UINT vk = m_shoutControls[index].cooldownSkillKeyEdit.CapturedKey();
    if (vk == 0)
    {
        SetStatus(_T("报点冷却中，未设置补按技能。"));
        return;
    }

    SendInputKey(vk);
    SetStatus(_T("报点冷却中，已补按技能键。"));
}

bool CKeyboardClickerDlg::PrepareForegroundTarget()
{
    if (!IsWindow(m_targetHwnd))
    {
        SetStatus(_T("请先把鼠标移到有效目标窗口/控件上，然后按 F9。"));
        return false;
    }

    HWND root = ::GetAncestor(m_targetHwnd, GA_ROOT);
    if (!IsWindow(root))
    {
        root = m_targetHwnd;
    }

    if (::IsIconic(root))
    {
        ::ShowWindow(root, SW_RESTORE);
    }

    HWND foregroundRoot = ::GetForegroundWindow();
    if (IsWindow(foregroundRoot))
    {
        foregroundRoot = ::GetAncestor(foregroundRoot, GA_ROOT);
    }

    if (foregroundRoot != root)
    {
        ::SetForegroundWindow(root);
        Sleep(40);
    }
    return true;
}

bool CKeyboardClickerDlg::SendInputKey(UINT vk)
{
    std::array<bool, MaxShoutActions> suspended = {};
    SuspendShoutHotkeysForKey(vk, suspended);

    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(vk);
    inputs[0].ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
    inputs[0].ki.dwFlags = IsExtendedKey(vk) ? KEYEVENTF_EXTENDEDKEY : 0;

    inputs[1] = inputs[0];
    inputs[1].ki.dwFlags |= KEYEVENTF_KEYUP;

    const UINT inputCount = static_cast<UINT>(_countof(inputs));
    if (::SendInput(inputCount, inputs, sizeof(INPUT)) != inputCount)
    {
        RestoreSuspendedShoutHotkeys(suspended);
        SetStatus(_T("发送按键失败。"));
        return false;
    }

    Sleep(10);
    RestoreSuspendedShoutHotkeys(suspended);
    return true;
}

bool CKeyboardClickerDlg::SendInputShortcut(UINT modifierVk, UINT vk)
{
    INPUT inputs[4] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(modifierVk);
    inputs[0].ki.wScan = static_cast<WORD>(MapVirtualKey(modifierVk, MAPVK_VK_TO_VSC));
    inputs[0].ki.dwFlags = IsExtendedKey(modifierVk) ? KEYEVENTF_EXTENDEDKEY : 0;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = static_cast<WORD>(vk);
    inputs[1].ki.wScan = static_cast<WORD>(MapVirtualKey(vk, MAPVK_VK_TO_VSC));
    inputs[1].ki.dwFlags = IsExtendedKey(vk) ? KEYEVENTF_EXTENDEDKEY : 0;

    inputs[2] = inputs[1];
    inputs[2].ki.dwFlags |= KEYEVENTF_KEYUP;

    inputs[3] = inputs[0];
    inputs[3].ki.dwFlags |= KEYEVENTF_KEYUP;

    const UINT inputCount = static_cast<UINT>(_countof(inputs));
    if (::SendInput(inputCount, inputs, sizeof(INPUT)) != inputCount)
    {
        SetStatus(_T("发送快捷键失败。"));
        return false;
    }

    return true;
}

bool CKeyboardClickerDlg::SendMouseWheel(int delta)
{
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = static_cast<DWORD>(delta);

    if (::SendInput(1, &input, sizeof(INPUT)) != 1)
    {
        SetStatus(_T("发送鼠标滚轮失败。"));
        return false;
    }

    return true;
}

bool CKeyboardClickerDlg::RequestAdvancedPassword()
{
    if (m_advancedAuthorized)
    {
        return true;
    }

    CAdvancedPasswordDialog dialog(this);
    if (dialog.DoModal() != IDOK)
    {
        SetStatus(_T("高级功能需要通过密码验证后才能启用。"));
        return false;
    }

    if (dialog.Password() != AdvancedFeaturePassword)
    {
        AfxMessageBox(_T("高级功能密码错误。"), MB_ICONWARNING);
        SetStatus(_T("高级功能密码错误，已取消初始化。"));
        return false;
    }

    MarkAdvancedAuthorized();
    SetStatus(_T("高级功能密码验证成功。"));
    return true;
}

void CKeyboardClickerDlg::MarkAdvancedAuthorized()
{
    m_advancedAuthorized = true;

    CWinApp* app = AfxGetApp();
    if (app != nullptr)
    {
        app->WriteProfileInt(SettingsSection, _T("AdvancedAuthorized"), 1);
    }
}

bool CKeyboardClickerDlg::RegisterDmPlugin()
{
    if (m_dmObject != nullptr)
    {
        m_advancedInitialized = true;
        if (!InitializeDmResources(m_dmObject))
        {
            ReleaseDmResources();
            return false;
        }
        SetStatus(_T("高级功能已初始化。"));
        return true;
    }

    const CString dmDllPath = ResolveDmDllPath();
    if (dmDllPath.IsEmpty())
    {
        CString message;
        message.Format(_T("未找到插件，请确认 %s 存在。"), DmProjectDllPath);
        SetStatus(message);
        return false;
    }

    if (m_dmModule == nullptr)
    {
        m_dmModule = LoadLibraryW(dmDllPath);
        if (m_dmModule == nullptr)
        {
            CString message;
            message.Format(_T("加载 dm.dll 失败，错误码: %lu"), GetLastError());
            SetStatus(message);
            return false;
        }
    }

    auto getClassObject = reinterpret_cast<DllGetClassObjectProc>(GetProcAddress(m_dmModule, "DllGetClassObject"));
    if (getClassObject == nullptr)
    {
        SetStatus(_T("dm.dll 中未找到 DllGetClassObject。"));
        ReleaseDmResources();
        return false;
    }

    IClassFactory* factory = nullptr;
    HRESULT hr = getClassObject(__uuidof(dmsoft), IID_IClassFactory, reinterpret_cast<void**>(&factory));
    if (FAILED(hr) || factory == nullptr)
    {
        CString message;
        message.Format(_T("获取 dmsoft 类工厂失败，HRESULT=0x%08X"), static_cast<unsigned>(hr));
        SetStatus(message);
        ReleaseDmResources();
        return false;
    }

    hr = factory->CreateInstance(nullptr, __uuidof(Idmsoft), reinterpret_cast<void**>(&m_dmObject));
    factory->Release();
    if (FAILED(hr) || m_dmObject == nullptr)
    {
        CString message;
        message.Format(_T("创建对象失败，HRESULT=0x%08X"), static_cast<unsigned>(hr));
        SetStatus(message);
        ReleaseDmResources();
        return false;
    }

    long regResult = 0;
    try
    {
        regResult = static_cast<Idmsoft*>(m_dmObject)->Reg(_bstr_t(DmRegistrationCode), _bstr_t(DmAdditionalCode));
    }
    catch (const _com_error& error)
    {
        ReleaseDmResources();

        CString message;
        message.Format(_T("调用 Reg 失败，HRESULT=0x%08X"), static_cast<unsigned>(error.Error()));
        SetStatus(message);
        return false;
    }

    CString message;
    message.Format(_T(" Reg 返回 %ld：%s"), regResult, static_cast<LPCTSTR>(FormatDmRegResult(regResult)));
    SetStatus(message);

    bool initialized = false;
    if (regResult == 1)
    {
        initialized = InitializeDmResources(m_dmObject);
    }

    if (regResult != 1 || !initialized)
    {
        ReleaseDmResources();
        return false;
    }

    m_advancedInitialized = true;
    return true;
}

void CKeyboardClickerDlg::ReleaseDmResources()
{
    if (m_dmObject != nullptr)
    {
        m_dmObject->Release();
        m_dmObject = nullptr;
    }
    if (m_dmModule != nullptr)
    {
        FreeLibrary(m_dmModule);
        m_dmModule = nullptr;
    }
    m_advancedInitialized = false;
}

bool CKeyboardClickerDlg::InitializeDmResources(IDispatch* dm)
{
    if (dm == nullptr)
    {
        SetStatus(_T("对象为空，无法初始化资源路径。"));
        return false;
    }

    const CString resourceRoot = ResolveDmResourceRoot();
    if (resourceRoot.IsEmpty())
    {
        SetStatus(_T("无法定位资源目录。"));
        return false;
    }

    const CString imageFolder = CombinePath(resourceRoot, DmImageFolderName);
    const CString dictFolder = CombinePath(resourceRoot, DmDictFolderName);
    const CString dictFile = CombinePath(resourceRoot, DmDefaultDictRelativePath);

    if (!EnsureDirectoryExists(imageFolder))
    {
        CString message;
        message.Format(_T("创建图库目录失败: %s"), static_cast<LPCTSTR>(imageFolder));
        SetStatus(message);
        return false;
    }

    if (!EnsureDirectoryExists(dictFolder))
    {
        CString message;
        message.Format(_T("创建字库目录失败: %s"), static_cast<LPCTSTR>(dictFolder));
        SetStatus(message);
        return false;
    }

    if (!EnsureFileExists(dictFile))
    {
        CString message;
        message.Format(_T("创建默认字库文件失败: %s"), static_cast<LPCTSTR>(dictFile));
        SetStatus(message);
        return false;
    }

    Idmsoft* dmsoft = static_cast<Idmsoft*>(dm);
    try
    {
        const long setPathResult = dmsoft->SetPath(_bstr_t(resourceRoot));
        CString pathMessage;
        pathMessage.Format(_T("SetPath(%s) 返回 %ld。"), static_cast<LPCTSTR>(resourceRoot), setPathResult);
        SetStatus(pathMessage);
        if (setPathResult != 1)
        {
            return false;
        }

        const long setDictResult = dmsoft->SetDict(0, _bstr_t(DmDefaultDictRelativePath));
        CString dictMessage;
        dictMessage.Format(_T("SetDict(0, %s) 返回 %ld。"), DmDefaultDictRelativePath, setDictResult);
        SetStatus(dictMessage);
        if (setDictResult != 1)
        {
            return false;
        }
    }
    catch (const _com_error& error)
    {
        CString message;
        message.Format(_T("初始化资源失败，HRESULT=0x%08X"), static_cast<unsigned>(error.Error()));
        SetStatus(message);
        return false;
    }

    SetStatus(_T("图库和字库初始化完成。"));
    return true;
}

bool CKeyboardClickerDlg::SelectScreenRegion(CRect& region)
{
    CRegionSelectWnd selector;
    return selector.Select(region);
}

void CKeyboardClickerDlg::SetRegionStatus(CStatic& label, bool hasRegion, const CRect& region, LPCTSTR emptyText)
{
    if (!label.GetSafeHwnd())
    {
        return;
    }
    if (!hasRegion)
    {
        label.SetWindowText(emptyText);
        return;
    }

    CString labelText(emptyText);
    const int colon = labelText.Find(L':');
    if (colon >= 0)
    {
        labelText = labelText.Left(colon);
    }

    CString text;
    text.Format(_T("%s: %ld,%ld - %ld,%ld"), static_cast<LPCTSTR>(labelText), region.left, region.top, region.right, region.bottom);
    label.SetWindowText(text);
}

void CKeyboardClickerDlg::StopHornMonitoring()
{
    KillTimer(HornTimerId);
    m_hornState.enabled = false;
    m_hornState.lastDebuffFound = false;
    m_hornState.lastHornAvailable = false;
    m_hornState.lastHornPressStatusTick = 0;
    m_hornState.actionLockedUntilDebuffClears = false;
    m_skillInputPending.store(false);
    for (auto& skill : m_skillStates)
    {
        skill.lastDetectTick = 0;
        skill.lastReleaseTick = 0;
    }
    UpdateSkillMonitorButtonText();
}

void CKeyboardClickerDlg::JoinFinishedAsyncWorkers()
{
    if (!m_skillWorkerActive.load() && m_skillWorker.joinable())
    {
        m_skillWorker.join();
    }
    if (!m_shoutWorkerActive.load() && m_shoutWorker.joinable())
    {
        m_shoutWorker.join();
    }
}

void CKeyboardClickerDlg::PostStatusFromWorker(const CString& text)
{
    if (m_closing.load() || !GetSafeHwnd())
    {
        return;
    }

    std::unique_ptr<AsyncStatusMessage> message(new AsyncStatusMessage());
    message->text = text;
    if (!::PostMessage(m_hWnd, WM_APP_ASYNC_STATUS, 0, reinterpret_cast<LPARAM>(message.get())))
    {
        return;
    }
    message.release();
}

bool CKeyboardClickerDlg::HasEnabledAdvancedSkill() const
{
    for (const auto& controls : m_skillControls)
    {
        if (controls.enabledCheck.GetSafeHwnd() && controls.enabledCheck.GetCheck() == BST_CHECKED)
        {
            return true;
        }
    }
    return false;
}

UINT CKeyboardClickerDlg::ReadSkillDetectIntervalMs(size_t index) const
{
    if (index >= m_skillControls.size())
    {
        return DefaultSkillDetectIntervalMs;
    }

    UINT interval = 0;
    if (!ReadUIntFromWindow(m_skillControls[index].detectIntervalEdit, interval) || interval < MinSkillDetectIntervalMs)
    {
        interval = DefaultSkillDetectIntervalMs;
    }
    return interval;
}

UINT CKeyboardClickerDlg::ReadSkillReleaseIntervalMs(size_t index) const
{
    if (index >= m_skillControls.size())
    {
        return DefaultSkillReleaseIntervalMs;
    }

    UINT interval = 0;
    if (!ReadUIntFromWindow(m_skillControls[index].intervalEdit, interval) || interval < MinSkillReleaseIntervalMs)
    {
        interval = DefaultSkillReleaseIntervalMs;
    }
    return interval;
}

UINT CKeyboardClickerDlg::NextSkillMonitorDelay() const
{
    if (m_skillWorkerActive.load() || m_skillInputPending.load())
    {
        return DefaultSkillDetectIntervalMs;
    }

    if (!HasEnabledAdvancedSkill())
    {
        return DefaultSkillDetectIntervalMs;
    }

    const ULONGLONG now = ::GetTickCount64();
    UINT bestDelay = 500;
    bool hasCandidate = false;
    for (size_t i = 0; i < m_skillControls.size(); ++i)
    {
        if (!m_skillControls[i].enabledCheck.GetSafeHwnd() || m_skillControls[i].enabledCheck.GetCheck() != BST_CHECKED)
        {
            continue;
        }

        const UINT detectInterval = m_skillStates[i].detectIntervalMs < MinSkillDetectIntervalMs ? DefaultSkillDetectIntervalMs : m_skillStates[i].detectIntervalMs;
        const UINT releaseInterval = m_skillStates[i].releaseIntervalMs < MinSkillReleaseIntervalMs ? DefaultSkillReleaseIntervalMs : m_skillStates[i].releaseIntervalMs;

        UINT detectRemaining = 1;
        if (m_skillStates[i].lastDetectTick != 0 && now >= m_skillStates[i].lastDetectTick)
        {
            const ULONGLONG elapsed = now - m_skillStates[i].lastDetectTick;
            detectRemaining = elapsed >= detectInterval ? 1 : static_cast<UINT>(detectInterval - elapsed);
        }

        UINT releaseRemaining = 1;
        if (m_skillStates[i].lastReleaseTick != 0 && now >= m_skillStates[i].lastReleaseTick)
        {
            const ULONGLONG elapsed = now - m_skillStates[i].lastReleaseTick;
            releaseRemaining = elapsed >= releaseInterval ? 1 : static_cast<UINT>(releaseInterval - elapsed);
        }

        const UINT nextDelay = (std::max)(detectRemaining, releaseRemaining);
        bestDelay = hasCandidate ? (std::min)(bestDelay, nextDelay) : nextDelay;
        hasCandidate = true;
    }

    if (!hasCandidate)
    {
        return DefaultSkillDetectIntervalMs;
    }
    return (std::max)(1U, bestDelay);
}

void CKeyboardClickerDlg::UpdateSkillMonitoringTimer()
{
    if (!m_hornState.enabled)
    {
        KillTimer(HornTimerId);
        UpdateSkillMonitorButtonText();
        return;
    }

    if (!m_advancedInitialized || !HasEnabledAdvancedSkill())
    {
        StopHornMonitoring();
        SetStatus(_T("自动检测已停止：高级功能未初始化或没有启用技能。"));
        return;
    }

    m_hornState.intervalMs = NextSkillMonitorDelay();
    m_hornState.actionLockedUntilDebuffClears = false;
    for (auto& skill : m_skillStates)
    {
        skill.lastDetectTick = 0;
    }
    SetTimer(HornTimerId, m_hornState.intervalMs, nullptr);
    UpdateSkillMonitorButtonText();

    CString message;
    message.Format(_T("自动技能检测已开始，下一轮约 %u ms；每个技能的检测(ms)与冷却(ms)独立生效。"), static_cast<unsigned>(m_hornState.intervalMs));
    SetStatus(message);
}

bool CKeyboardClickerDlg::DetectSkillOnce(size_t index)
{
    if (index >= m_skillStates.size())
    {
        return false;
    }

    InvalidateHornPictureCache();
    std::vector<size_t> order;
    order.push_back(index);
    return LaunchSkillDetection(order, true, false);
}

bool CKeyboardClickerDlg::DetectBestSkillOnce(bool saveSettings, bool respectIntervals)
{
    if (m_skillWorkerActive.load() || m_skillInputPending.load())
    {
        if (!respectIntervals)
        {
            SetStatus(_T("上一次识图任务仍在执行，请稍后再试。"));
        }
        return false;
    }

    std::vector<size_t> order;
    order.reserve(m_skillStates.size());
    size_t enabledSkillCount = 0;
    bool hasDueDetection = !respectIntervals;
    const ULONGLONG now = GetTickCount64();
    for (size_t i = 0; i < m_skillStates.size(); ++i)
    {
        m_skillStates[i].enabled = m_skillControls[i].enabledCheck.GetCheck() == BST_CHECKED;
        if (!m_skillStates[i].enabled)
        {
            continue;
        }
        ++enabledSkillCount;

        UINT priority = 0;
        if (!ReadUIntFromWindow(m_skillControls[i].priorityEdit, priority) || priority == 0)
        {
            priority = DefaultSkillPriorities[i];
            CString priorityText;
            priorityText.Format(_T("%u"), static_cast<unsigned>(priority));
            m_skillControls[i].priorityEdit.SetWindowText(priorityText);
        }
        m_skillStates[i].priority = priority;
        m_skillStates[i].detectIntervalMs = ReadSkillDetectIntervalMs(i);
        m_skillStates[i].releaseIntervalMs = ReadSkillReleaseIntervalMs(i);

        bool releaseReady = true;
        if (respectIntervals && m_skillStates[i].lastReleaseTick != 0 && now >= m_skillStates[i].lastReleaseTick && now - m_skillStates[i].lastReleaseTick < m_skillStates[i].releaseIntervalMs)
        {
            releaseReady = false;
        }
        if (!releaseReady)
        {
            continue;
        }

        if (respectIntervals)
        {
            const bool detectDue = m_skillStates[i].lastDetectTick == 0 || now < m_skillStates[i].lastDetectTick || now - m_skillStates[i].lastDetectTick >= m_skillStates[i].detectIntervalMs;
            hasDueDetection = hasDueDetection || detectDue;
        }
        order.push_back(i);
    }

    std::sort(order.begin(), order.end(), [this](size_t left, size_t right) {
        if (m_skillStates[left].priority != m_skillStates[right].priority)
        {
            return m_skillStates[left].priority < m_skillStates[right].priority;
        }
        return left < right;
    });

    if (order.empty())
    {
        if (respectIntervals && enabledSkillCount > 0)
        {
            return false;
        }
        SetStatus(_T("按优先级检测：没有启用任何技能。"));
        if (saveSettings)
        {
            SaveSettings();
        }
        return false;
    }

    if (respectIntervals && !hasDueDetection)
    {
        return false;
    }

    return LaunchSkillDetection(order, saveSettings, respectIntervals);
}

bool CKeyboardClickerDlg::LaunchSkillDetection(const std::vector<size_t>& order, bool saveSettings, bool respectIntervals)
{
    if (m_skillWorkerActive.load())
    {
        SetStatus(_T("识图任务仍在执行，请稍后再试。"));
        return false;
    }

    JoinFinishedAsyncWorkers();

    for (size_t skillIndex = 0; skillIndex < m_skillStates.size(); ++skillIndex)
    {
        for (size_t debuffIndex = 0; debuffIndex < m_skillStates[skillIndex].debuffs.size(); ++debuffIndex)
        {
            m_skillStates[skillIndex].debuffs[debuffIndex] = m_skillControls[skillIndex].debuffChecks[debuffIndex].GetCheck() == BST_CHECKED;
        }
    }

    SkillDetectionTask task;
    task.targetHwnd = m_targetHwnd;
    task.dmDllPath = ResolveDmDllPath();
    task.resourceRoot = GetDmResourceRootForUse();
    task.similarity = ReadHornSimilarity();
    task.respectIntervals = respectIntervals;
    task.saveSettings = saveSettings;
    task.actionLockedUntilDebuffClears = m_hornState.actionLockedUntilDebuffClears;
    task.fastAutoDispatch = respectIntervals;
    task.dispatchTick = GetTickCount64();
    task.availabilityProbeIndex = m_nextAvailabilityProbeIndex;
    task.debuffRegion = m_hornState.debuffRegion;

    if (task.dmDllPath.IsEmpty())
    {
        SetStatus(_T("未找到 dm.dll，无法启动后台识图。"));
        return false;
    }
    if (task.resourceRoot.IsEmpty())
    {
        SetStatus(_T("无法定位大漠资源根目录。"));
        return false;
    }

    for (size_t index : order)
    {
        CString skillPictures;
        CString selectedPictures;
        if (!ValidateAdvancedSkillSettings(index, skillPictures, selectedPictures))
        {
            continue;
        }

        SkillDetectionItem item;
        item.enabled = true;
        item.index = index;
        item.skillVk = m_skillStates[index].skillVk;
        item.priority = m_skillStates[index].priority;
        item.skillRegion = m_skillStates[index].region;
        item.skillPictures = skillPictures;
        item.debuffPictures = selectedPictures;
        item.cachedSkillAvailable = m_skillStates[index].lastSkillAvailable;
        item.cachedSkillAvailableTick = m_skillStates[index].lastSkillAvailableTick;
        AppendPictureList(task.combinedDebuffPictures, selectedPictures);
        if (respectIntervals)
        {
            m_skillStates[index].lastDetectTick = task.dispatchTick;
        }
        task.items.push_back(item);
    }

    if (task.items.empty())
    {
        SetStatus(_T("没有可用于识图的技能配置。"));
        return false;
    }

    m_skillWorkerActive.store(true);
    if (m_skillWorker.joinable())
    {
        m_skillWorker.join();
    }

    HWND hwnd = m_hWnd;
    m_skillWorker = std::thread([this, hwnd, task]() {
        std::unique_ptr<SkillDetectionResult> result(new SkillDetectionResult());
        result->respectIntervals = task.respectIntervals;
        result->saveSettings = task.saveSettings;

        const HRESULT coResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool comInitialized = SUCCEEDED(coResult);

        HMODULE workerDmModule = nullptr;
        IDispatch* workerDmObject = nullptr;
        CString error;
        if (!CreateWorkerDmObject(task.dmDllPath, task.resourceRoot, workerDmModule, workerDmObject, error))
        {
            result->message = error.IsEmpty() ? CString(_T("后台识图初始化失败。")) : error;
            ReleaseWorkerDmObject(workerDmModule, workerDmObject);
            if (comInitialized)
            {
                CoUninitialize();
            }
            m_skillWorkerActive.store(false);
            if (!m_closing.load() && ::IsWindow(hwnd))
            {
                ::PostMessage(hwnd, WM_APP_SKILL_WORKER_FINISHED, 0, reinterpret_cast<LPARAM>(result.release()));
            }
            return;
        }

        CString aggregateDebuffResult;
        CString aggregateFindError;

        if (task.actionLockedUntilDebuffClears)
        {
            const bool debuffStillFound = WorkerDmFindPicEx(workerDmObject, task.debuffRegion, task.combinedDebuffPictures, task.similarity, aggregateDebuffResult, aggregateFindError);
            if (!aggregateFindError.IsEmpty())
            {
                result->diagnostics.push_back(aggregateFindError);
            }
            if (debuffStillFound)
            {
                result->message = _T("自动技能检测：已释放过技能，等待当前 Debuff 消失。");
                ReleaseWorkerDmObject(workerDmModule, workerDmObject);
                if (comInitialized)
                {
                    CoUninitialize();
                }
                m_skillWorkerActive.store(false);
                if (!m_closing.load() && ::IsWindow(hwnd))
                {
                    ::PostMessage(hwnd, WM_APP_SKILL_WORKER_FINISHED, 0, reinterpret_cast<LPARAM>(result.release()));
                }
                return;
            }

            result->clearActionLock = true;
        }

        if (task.fastAutoDispatch)
        {
            CString debuffResult;
            CString findError;
            const bool debuffFound = WorkerDmFindPicEx(workerDmObject, task.debuffRegion, task.combinedDebuffPictures, task.similarity, debuffResult, findError);
            if (!findError.IsEmpty())
            {
                result->diagnostics.push_back(findError);
            }

            if (debuffFound)
            {
                result->sawMatchingDebuff = true;
                const SkillDetectionItem* selectedItem = nullptr;

                for (const SkillDetectionItem& item : task.items)
                {
                    CString itemDebuffResult;
                    CString itemDebuffFindError;
                    const bool itemDebuffFound = WorkerDmFindPicEx(workerDmObject, task.debuffRegion, item.debuffPictures, task.similarity, itemDebuffResult, itemDebuffFindError);
                    if (!itemDebuffFindError.IsEmpty())
                    {
                        result->diagnostics.push_back(itemDebuffFindError);
                        continue;
                    }
                    if (!itemDebuffFound)
                    {
                        continue;
                    }

                    if (item.cachedSkillAvailable && item.cachedSkillAvailableTick != 0 && task.dispatchTick >= item.cachedSkillAvailableTick && task.dispatchTick - item.cachedSkillAvailableTick <= SkillAvailabilityCacheMs)
                    {
                        selectedItem = &item;
                        break;
                    }

                    CString skillResult;
                    CString skillFindError;
                    const bool skillAvailable = WorkerDmFindPicEx(workerDmObject, item.skillRegion, item.skillPictures, task.similarity, skillResult, skillFindError);
                    if (!skillFindError.IsEmpty())
                    {
                        result->diagnostics.push_back(skillFindError);
                        continue;
                    }

                    SkillAvailabilityUpdate update;
                    update.index = item.index;
                    update.available = skillAvailable;
                    update.tick = GetTickCount64();
                    result->availabilityUpdates.push_back(update);

                    if (skillAvailable)
                    {
                        selectedItem = &item;
                        break;
                    }

                    CString unavailableDiagnostic;
                    unavailableDiagnostic.Format(_T("严格优先级：%s 匹配当前 Debuff，但技能图标未命中，继续检查下一个优先级。"), AdvancedSkillDefinitions[item.index].label);
                    result->diagnostics.push_back(unavailableDiagnostic);
                }

                if (selectedItem != nullptr)
                {
                    result->success = true;
                    result->setActionLock = true;
                    result->index = selectedItem->index;
                    result->skillVk = selectedItem->skillVk;
                    result->priority = selectedItem->priority;

                    m_skillInputPending.store(true);
                    if (WorkerPrepareForegroundTarget(task.targetHwnd) && WorkerSendInputKey(selectedItem->skillVk))
                    {
                        result->inputAlreadySent = true;
                        result->message.Format(_T("严格优先级解除：检测到匹配 Debuff，已按下 %s。"), AdvancedSkillDefinitions[selectedItem->index].label);
                    }
                    else
                    {
                        result->success = false;
                        result->inputFailed = true;
                        result->message.Format(_T("严格优先级解除：检测到匹配 Debuff，但发送 %s 按键失败。"), AdvancedSkillDefinitions[selectedItem->index].label);
                    }
                }
            }
            else
            {
                const size_t itemCount = task.items.size();
                if (itemCount > 0)
                {
                    const SkillDetectionItem& probeItem = task.items[task.availabilityProbeIndex % itemCount];
                    CString skillResult;
                    CString skillFindError;
                    const bool skillAvailable = WorkerDmFindPicEx(workerDmObject, probeItem.skillRegion, probeItem.skillPictures, task.similarity, skillResult, skillFindError);
                    if (skillFindError.IsEmpty())
                    {
                        SkillAvailabilityUpdate update;
                        update.index = probeItem.index;
                        update.available = skillAvailable;
                        update.tick = GetTickCount64();
                        result->availabilityUpdates.push_back(update);
                    }
                    else
                    {
                        result->diagnostics.push_back(skillFindError);
                    }
                }
            }
        }
        else
        {
            for (const SkillDetectionItem& item : task.items)
            {
                CString debuffResult;
                CString findError;
                const bool debuffFound = WorkerDmFindPicEx(workerDmObject, task.debuffRegion, item.debuffPictures, task.similarity, debuffResult, findError);
                if (!findError.IsEmpty())
                {
                    result->diagnostics.push_back(findError);
                    continue;
                }
                if (!debuffFound)
                {
                    continue;
                }
                result->sawMatchingDebuff = true;

                CString skillResult;
                const bool skillAvailable = WorkerDmFindPicEx(workerDmObject, item.skillRegion, item.skillPictures, task.similarity, skillResult, findError);
                if (!findError.IsEmpty())
                {
                    result->diagnostics.push_back(findError);
                    continue;
                }
                SkillAvailabilityUpdate update;
                update.index = item.index;
                update.available = skillAvailable;
                update.tick = GetTickCount64();
                result->availabilityUpdates.push_back(update);

                CString compactSkillResult = skillResult.IsEmpty() ? CString(_T("<空>")) : CompactForDebugLine(skillResult, 160);
                CString diagnostic;
                diagnostic.Format(_T("%s后台识图 %s | 范围=%ld,%ld,%ld,%ld | 精准度=%.2f | 返回=%s | 图片数=%d"),
                    AdvancedSkillDefinitions[item.index].label,
                    skillAvailable ? _T("命中") : _T("未命中"),
                    item.skillRegion.left,
                    item.skillRegion.top,
                    item.skillRegion.right,
                    item.skillRegion.bottom,
                    task.similarity,
                    static_cast<LPCTSTR>(compactSkillResult),
                    CountPictureEntries(item.skillPictures));
                result->diagnostics.push_back(diagnostic);

                if (!skillAvailable)
                {
                    continue;
                }

                result->success = true;
                result->setActionLock = task.respectIntervals;
                result->index = item.index;
                result->skillVk = item.skillVk;
                result->priority = item.priority;
                result->message.Format(_T("按优先级检测：已命中 %s，准备按技能键。"), AdvancedSkillDefinitions[item.index].label);
                break;
            }
        }

        if (!result->success && result->message.IsEmpty())
        {
            result->message = result->sawMatchingDebuff ? _T("按优先级检测：当前控制状态下没有可用技能。") : _T("按优先级检测：未命中任何已配置的 Debuff。");
        }

        ReleaseWorkerDmObject(workerDmModule, workerDmObject);
        if (comInitialized)
        {
            CoUninitialize();
        }

        m_skillWorkerActive.store(false);
        if (!m_closing.load() && ::IsWindow(hwnd))
        {
            ::PostMessage(hwnd, WM_APP_SKILL_WORKER_FINISHED, 0, reinterpret_cast<LPARAM>(result.release()));
        }
    });

    if (!respectIntervals)
    {
        SetStatus(_T("手动识图任务已提交到后台。"));
    }
    return true;
}

bool CKeyboardClickerDlg::AnyConfiguredDebuffFound()
{
    if (!m_hornState.hasDebuffRegion)
    {
        return false;
    }

    for (size_t i = 0; i < m_skillStates.size(); ++i)
    {
        if (!m_skillControls[i].enabledCheck.GetSafeHwnd() || m_skillControls[i].enabledCheck.GetCheck() != BST_CHECKED)
        {
            continue;
        }

        CString pictures = BuildSkillDebuffPictureList(i);
        if (pictures.IsEmpty())
        {
            continue;
        }

        CString result;
        if (DmFindPicEx(m_hornState.debuffRegion, pictures, result))
        {
            return true;
        }
    }

    return false;
}

double CKeyboardClickerDlg::ReadHornSimilarity() const
{
    CString text = GetWindowTextString(m_hornSimilarityEdit);
    text.Trim();
    wchar_t* end = nullptr;
    const double similarity = wcstod(text, &end);
    if (end == static_cast<LPCWSTR>(text) || *end != L'\0' || similarity < 0.1 || similarity > 1.0)
    {
        return 0.9;
    }
    return similarity;
}

void CKeyboardClickerDlg::InvalidateHornPictureCache()
{
    for (auto& skill : m_skillStates)
    {
        skill.cachedPictures.Empty();
        skill.cachedDebuffPictures.Empty();
    }
    m_hornState.cachedDebuffPictures.Empty();
    m_hornState.lastPictureRefreshTick = 0;
    m_hornState.pictureCacheValid = false;
}

void CKeyboardClickerDlg::RefreshHornPictureCache(bool force)
{
    const ULONGLONG now = GetTickCount64();
    if (!force && m_hornState.pictureCacheValid && now - m_hornState.lastPictureRefreshTick < HornPictureRefreshMs)
    {
        return;
    }

    for (size_t i = 0; i < m_skillStates.size(); ++i)
    {
        m_skillStates[i].cachedPictures = BuildSkillPictureList(i);
        m_skillStates[i].cachedDebuffPictures = BuildSkillDebuffPictureList(i);
    }
    m_hornState.cachedDebuffPictures = BuildSelectedDebuffPictureList();
    m_hornState.lastPictureRefreshTick = now;
    m_hornState.pictureCacheValid = true;
}

bool CKeyboardClickerDlg::ValidateAdvancedSkillSettings(size_t index, CString& skillPictures, CString& selectedPictures)
{
    if (index >= m_skillStates.size())
    {
        return false;
    }

    if (m_dmObject == nullptr || !m_advancedInitialized)
    {
        SetStatus(_T("请先点击“高级功能”完成初始化。"));
        return false;
    }

    if (!m_hornState.hasDebuffRegion)
    {
        SetStatus(_T("请先设置 Debuff 识别范围。"));
        return false;
    }
    if (!m_skillStates[index].hasRegion)
    {
        CString message;
        message.Format(_T("请先设置 %s 识别范围。"), AdvancedSkillDefinitions[index].label);
        SetStatus(message);
        return false;
    }

    m_skillStates[index].skillVk = m_skillControls[index].keyEdit.CapturedKey();
    if (m_skillStates[index].skillVk == 0)
    {
        CString message;
        message.Format(_T("请先设置 %s 技能键。"), AdvancedSkillDefinitions[index].label);
        SetStatus(message);
        return false;
    }

    for (size_t debuff = 0; debuff < m_skillStates[index].debuffs.size(); ++debuff)
    {
        m_skillStates[index].debuffs[debuff] = m_skillControls[index].debuffChecks[debuff].GetCheck() == BST_CHECKED;
    }

    CString similarityText = GetWindowTextString(m_hornSimilarityEdit);
    similarityText.Trim();
    wchar_t* similarityEnd = nullptr;
    const double similarity = wcstod(similarityText, &similarityEnd);
    if (similarityEnd == static_cast<LPCWSTR>(similarityText) || *similarityEnd != L'\0' || similarity < 0.1 || similarity > 1.0)
    {
        SetStatus(_T("精准度必须是 0.10 到 1.00 之间的小数。"));
        return false;
    }
    m_hornState.similarity = similarity;

    const CString root = GetDmResourceRootForUse();
    if (root.IsEmpty())
    {
        SetStatus(_T("无法定位大漠资源根目录。"));
        return false;
    }

    RefreshHornPictureCache(false);
    skillPictures = m_skillStates[index].cachedPictures;
    selectedPictures = m_skillStates[index].cachedDebuffPictures;

    if (selectedPictures.IsEmpty())
    {
        CString message;
        message.Format(_T("请至少给 %s 选择一个可用的 Debuff 条件，并确认对应图片存在。"), AdvancedSkillDefinitions[index].label);
        SetStatus(message);
        return false;
    }

    if (skillPictures.IsEmpty())
    {
        CString message;
        message.Format(_T("缺少 %s 图片，请放到 %s 或点击“添加样本”。"),
            AdvancedSkillDefinitions[index].label,
            static_cast<LPCTSTR>(CombinePath(root, AdvancedSkillDefinitions[index].sampleDirectory)));
        SetStatus(message);
        return false;
    }

    return true;
}

bool CKeyboardClickerDlg::ValidateSkillIconSettings(size_t index, CString& skillPictures)
{
    if (index >= m_skillStates.size())
    {
        return false;
    }

    if (m_dmObject == nullptr || !m_advancedInitialized)
    {
        SetStatus(_T("请先点击“高级功能”完成初始化。"));
        return false;
    }

    if (!m_skillStates[index].hasRegion)
    {
        CString message;
        message.Format(_T("请先设置 %s 识别范围。"), AdvancedSkillDefinitions[index].label);
        SetStatus(message);
        return false;
    }

    m_skillStates[index].skillVk = m_skillControls[index].keyEdit.CapturedKey();
    if (m_skillStates[index].skillVk == 0)
    {
        CString message;
        message.Format(_T("请先设置 %s 技能键。"), AdvancedSkillDefinitions[index].label);
        SetStatus(message);
        return false;
    }

    CString similarityText = GetWindowTextString(m_hornSimilarityEdit);
    similarityText.Trim();
    wchar_t* similarityEnd = nullptr;
    const double similarity = wcstod(similarityText, &similarityEnd);
    if (similarityEnd == static_cast<LPCWSTR>(similarityText) || *similarityEnd != L'\0' || similarity < 0.1 || similarity > 1.0)
    {
        SetStatus(_T("精准度必须是 0.10 到 1.00 之间的小数。"));
        return false;
    }
    m_hornState.similarity = similarity;

    const CString root = GetDmResourceRootForUse();
    if (root.IsEmpty())
    {
        SetStatus(_T("无法定位大漠资源根目录。"));
        return false;
    }

    RefreshHornPictureCache(false);
    skillPictures = m_skillStates[index].cachedPictures;
    if (skillPictures.IsEmpty())
    {
        CString message;
        message.Format(_T("缺少 %s 图片，请放到 %s 或点击“添加样本”。"),
            AdvancedSkillDefinitions[index].label,
            static_cast<LPCTSTR>(CombinePath(root, AdvancedSkillDefinitions[index].sampleDirectory)));
        SetStatus(message);
        return false;
    }

    return true;
}

CString CKeyboardClickerDlg::BuildSkillDebuffPictureList(size_t index) const
{
    if (index >= m_skillStates.size())
    {
        return CString();
    }

    CString pictures;
    for (size_t i = 0; i < m_skillStates[index].debuffs.size(); ++i)
    {
        if (!m_skillStates[index].debuffs[i])
        {
            continue;
        }
        AppendPictureIfExists(pictures, DebuffDefinitions[i].picture);
        AppendPicturesFromDirectory(pictures, DebuffDefinitions[i].sampleDirectory, _T("*.bmp"));
    }
    return pictures;
}

CString CKeyboardClickerDlg::BuildSkillPictureList(size_t index) const
{
    if (index >= _countof(AdvancedSkillDefinitions))
    {
        return CString();
    }

    CString pictures;
    AppendPicturesFromDirectory(pictures, DmImageFolderName, AdvancedSkillDefinitions[index].rootPicturePattern);
    AppendPicturesFromDirectory(pictures, AdvancedSkillDefinitions[index].sampleDirectory, _T("*.bmp"));
    return pictures;
}

CString CKeyboardClickerDlg::BuildSelectedDebuffPictureList() const
{
    CString pictures;
    for (size_t i = 0; i < m_hornState.debuffs.size(); ++i)
    {
        if (!m_hornState.debuffs[i])
        {
            continue;
        }
        AppendPictureIfExists(pictures, DebuffDefinitions[i].picture);
        AppendPicturesFromDirectory(pictures, DebuffDefinitions[i].sampleDirectory, _T("*.bmp"));
    }
    return pictures;
}

bool CKeyboardClickerDlg::AppendPictureIfExists(CString& pictures, const CString& relativePath) const
{
    const CString root = GetDmResourceRootForUse();
    if (root.IsEmpty())
    {
        return false;
    }

    if (!FileExists(CombinePath(root, relativePath)))
    {
        return false;
    }

    if (!pictures.IsEmpty())
    {
        pictures += _T("|");
    }
    pictures += NormalizeDmPicturePath(relativePath);
    return true;
}

void CKeyboardClickerDlg::AppendPicturesFromDirectory(CString& pictures, LPCTSTR relativeDirectory, LPCTSTR filePattern) const
{
    const CString root = GetDmResourceRootForUse();
    if (root.IsEmpty())
    {
        return;
    }

    const CString sampleDirectory = CombinePath(root, relativeDirectory);
    CString pattern = CombinePath(sampleDirectory, filePattern);
    WIN32_FIND_DATA data = {};
    HANDLE find = FindFirstFile(pattern, &data);
    if (find == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            continue;
        }
        CString relativePath(relativeDirectory);
        relativePath += _T("\\");
        relativePath += data.cFileName;
        AppendPictureIfExists(pictures, relativePath);
    } while (FindNextFile(find, &data));

    FindClose(find);
}

CString CKeyboardClickerDlg::FilterExistingPictures(const CString& pictures) const
{
    CString filtered;
    CString remaining(pictures);
    int position = 0;

    while (true)
    {
        CString picture = remaining.Tokenize(_T("|"), position);
        if (picture.IsEmpty() && position == -1)
        {
            break;
        }

        picture.Trim();
        if (picture.IsEmpty())
        {
            if (position == -1)
            {
                break;
            }
            continue;
        }

        CString fileSystemPath(picture);
        fileSystemPath.Replace(_T("/"), _T("\\"));
        if (AppendPictureIfExists(filtered, fileSystemPath) && position == -1)
        {
            break;
        }

        if (position == -1)
        {
            break;
        }
    }

    return filtered;
}

CString CKeyboardClickerDlg::NormalizeDmPicturePath(const CString& path) const
{
    CString normalized(path);
    normalized.Replace(_T("\\"), _T("/"));
    return normalized;
}

CString CKeyboardClickerDlg::GetHornSampleDirectory() const
{
    return GetSkillSampleDirectory(0);
}

CString CKeyboardClickerDlg::GetSkillSampleDirectory(size_t index) const
{
    if (index >= _countof(AdvancedSkillDefinitions))
    {
        return CString();
    }

    return CombinePath(GetDmResourceRootForUse(), AdvancedSkillDefinitions[index].sampleDirectory);
}

CString CKeyboardClickerDlg::GetDebuffSampleDirectory(size_t index) const
{
    if (index >= _countof(DebuffDefinitions))
    {
        return CString();
    }

    return CombinePath(GetDmResourceRootForUse(), DebuffDefinitions[index].sampleDirectory);
}

bool CKeyboardClickerDlg::AddDebuffSample(size_t index)
{
    if (index >= _countof(DebuffDefinitions))
    {
        return false;
    }

    return AddPictureSamples(GetDebuffSampleDirectory(index), DebuffDefinitions[index].label);
}

bool CKeyboardClickerDlg::AddPictureSamples(const CString& directory, LPCTSTR label)
{
    CFileDialog dialog(TRUE, _T("bmp"), nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT, _T("Bitmap Files (*.bmp)|*.bmp|All Files (*.*)|*.*||"), this);

    std::vector<TCHAR> buffer(8192, 0);
    dialog.m_ofn.lpstrFile = buffer.data();
    dialog.m_ofn.nMaxFile = static_cast<DWORD>(buffer.size());

    if (dialog.DoModal() != IDOK)
    {
        return false;
    }

    if (!EnsureDirectoryExists(directory))
    {
        CString message;
        message.Format(_T("创建样本目录失败: %s"), static_cast<LPCTSTR>(directory));
        SetStatus(message);
        return false;
    }

    int copied = 0;
    POSITION pos = dialog.GetStartPosition();
    while (pos != nullptr)
    {
        CString source = dialog.GetNextPathName(pos);
        CString extension(_T(".bmp"));
        const int dot = source.ReverseFind(L'.');
        const int slash = (std::max)(source.ReverseFind(L'\\'), source.ReverseFind(L'/'));
        if (dot > slash)
        {
            extension = source.Mid(dot);
        }
        if (!IsBitmapFileExtension(extension))
        {
            CString message;
            message.Format(_T("已跳过非 BMP 样本: %s"), static_cast<LPCTSTR>(source));
            SetStatus(message);
            continue;
        }

        CString target = MakeUniqueSamplePath(directory, extension);
        if (CopyFile(source, target, FALSE))
        {
            ++copied;
        }
        else
        {
            CString message;
            message.Format(_T("复制样本失败: %s，错误码: %lu"), static_cast<LPCTSTR>(source), GetLastError());
            SetStatus(message);
        }
    }

    CString message;
    message.Format(_T("%s 已添加 %d 个样本。"), label, copied);
    SetStatus(message);
    return copied > 0;
}

CString CKeyboardClickerDlg::MakeUniqueSamplePath(const CString& directory, const CString& extension) const
{
    SYSTEMTIME now = {};
    GetLocalTime(&now);

    for (int i = 0; i < 1000; ++i)
    {
        CString name;
        name.Format(_T("sample_%04u%02u%02u_%02u%02u%02u_%03u_%03d%s"),
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds, i, static_cast<LPCTSTR>(extension));
        CString path = CombinePath(directory, name);
        if (!FileExists(path))
        {
            return path;
        }
    }

    return CombinePath(directory, _T("sample.bmp"));
}

CString CKeyboardClickerDlg::GetDmResourceRootForUse() const
{
    return ResolveDmResourceRoot();
}

bool CKeyboardClickerDlg::DmFindPicEx(const CRect& region, const CString& pictures, CString& result)
{
    result.Empty();
    if (m_dmObject == nullptr || pictures.IsEmpty())
    {
        return false;
    }

    try
    {
        _bstr_t raw = static_cast<Idmsoft*>(m_dmObject)->FindPicEx(region.left, region.top, region.right, region.bottom, _bstr_t(pictures), _bstr_t(DmFindPicDeltaColor), m_hornState.similarity, 0);
        const wchar_t* value = static_cast<const wchar_t*>(raw);
        if (value != nullptr)
        {
            result = value;
        }
    }
    catch (const _com_error& error)
    {
        CString message;
        message.Format(_T("FindPicEx 调用失败，HRESULT=0x%08X"), static_cast<unsigned>(error.Error()));
        SetStatus(message);
        return false;
    }

    result.Trim();
    return !result.IsEmpty() && result.Left(2) != _T("-1");
}

bool CKeyboardClickerDlg::TestHornRecognitionOnce()
{
    return DetectSkillOnce(0);
}

bool CKeyboardClickerDlg::ValidateHornRecognitionInputs(CString& hornPictures)
{
    if (m_dmObject == nullptr || !m_advancedInitialized)
    {
        SetStatus(_T("请先点击“高级功能”完成大漠初始化。"));
        return false;
    }

    if (!m_skillStates[0].hasRegion)
    {
        SetStatus(_T("请先设置号角识别范围。"));
        return false;
    }

    CString similarityText = GetWindowTextString(m_hornSimilarityEdit);
    similarityText.Trim();
    wchar_t* similarityEnd = nullptr;
    const double similarity = wcstod(similarityText, &similarityEnd);
    if (similarityEnd == static_cast<LPCWSTR>(similarityText) || *similarityEnd != L'\0' || similarity < 0.1 || similarity > 1.0)
    {
        SetStatus(_T("精准度必须是 0.10 到 1.00 之间的小数。"));
        return false;
    }
    m_hornState.similarity = similarity;

    hornPictures = BuildSkillPictureList(0);
    if (hornPictures.IsEmpty())
    {
        CString message;
        message.Format(_T("缺少号角图片，请放到 %s 或点击“号角样本”添加。"), static_cast<LPCTSTR>(CombinePath(GetDmResourceRootForUse(), HornSampleDirectory)));
        SetStatus(message);
        return false;
    }

    return true;
}

void CKeyboardClickerDlg::LogFindPicResult(LPCTSTR label, const CRect& region, const CString& pictures, const CString& result, bool found)
{
    const CString compactResult = result.IsEmpty() ? CString(_T("<空>")) : CompactForDebugLine(result, 160);
    CString message;
    message.Format(_T("%s FindPicEx %s | 范围=%ld,%ld,%ld,%ld | 精准度=%.2f | 返回=%s | 图片数=%d"),
        label,
        found ? _T("命中") : _T("未命中"),
        region.left,
        region.top,
        region.right,
        region.bottom,
        m_hornState.similarity,
        static_cast<LPCTSTR>(compactResult),
        CountPictureEntries(pictures));
    SetStatus(message);
}

CString CKeyboardClickerDlg::FormatDmRegResult(long result) const
{
    switch (result)
    {
    case 1:
        return _T("注册成功");
    case 0:
        return _T("注册失败，未知错误");
    case -1:
        return _T("无法连接网络");
    case -2:
        return _T("可能需要管理员权限运行");
    case 2:
        return _T("余额不足");
    case 3:
        return _T("绑定余额不足或账户余额不足");
    case 4:
        return _T("注册码错误");
    case 5:
        return _T("机器码或 IP 不在允许范围");
    case 6:
        return _T("非法使用插件");
    case 7:
        return _T("账户被判定为非法使用");
    case 8:
        return _T("附加码不符合设置");
    case 77:
        return _T("机器码或 IP 被判定为非法使用");
    case 777:
        return _T("同机器多注册码异常");
    case -8:
        return _T("附加码长度超过 40");
    case -9:
        return _T("附加码包含非法字符");
    default:
        {
            CString text;
            text.Format(_T("未识别返回码 %ld"), result);
            return text;
        }
    }
}

bool CKeyboardClickerDlg::ExecuteShoutSequence(size_t index)
{
    if (index >= m_shoutActionCount)
    {
        return false;
    }

    CString sequence;
    m_shoutControls[index].sequenceEdit.GetWindowText(sequence);
    sequence.Trim();
    if (sequence.IsEmpty())
    {
        return true;
    }

    int current = 0;
    CString token = sequence.Tokenize(_T(",; \t\r\n"), current);
    while (!token.IsEmpty())
    {
        token.Trim();
        if (!token.IsEmpty() && !ExecuteReportSequenceToken(token))
        {
            return false;
        }

        token = sequence.Tokenize(_T(",; \t\r\n"), current);
    }

    SetStatus(_T("报点和技能序列已执行。"));
    return true;
}

bool CKeyboardClickerDlg::ExecuteReportSequenceToken(const CString& token)
{
    CString normalized = token;
    normalized.Trim();
    normalized.MakeUpper();

    wchar_t* end = nullptr;
    const unsigned long delayMs = wcstoul(normalized, &end, 10);
    if (end != static_cast<LPCWSTR>(normalized) && *end == L'\0')
    {
        if (delayMs > 3000)
        {
            SetStatus(_T("序列中的等待时间不能超过 3000 毫秒。"));
            return false;
        }

        Sleep(static_cast<DWORD>(delayMs));
        return true;
    }

    if (normalized == _T("WHEELUP") || normalized == _T("WHEEL_UP") || normalized == _T("SCROLLUP"))
    {
        return SendMouseWheel(WHEEL_DELTA);
    }
    if (normalized == _T("WHEELDOWN") || normalized == _T("WHEEL_DOWN") || normalized == _T("SCROLLDOWN"))
    {
        return SendMouseWheel(-WHEEL_DELTA);
    }

    UINT vk = 0;
    if (!TryParseVirtualKey(normalized, vk))
    {
        CString message;
        message.Format(_T("无法识别序列项: %s"), static_cast<LPCTSTR>(token));
        SetStatus(message);
        return false;
    }

    return SendInputKey(vk);
}

bool CKeyboardClickerDlg::ReadClipboardText(CString& text) const
{
    text.Empty();
    if (!::OpenClipboard(m_hWnd))
    {
        return false;
    }

    HANDLE data = ::GetClipboardData(CF_UNICODETEXT);
    if (data == nullptr)
    {
        ::CloseClipboard();
        return false;
    }

    const wchar_t* value = static_cast<const wchar_t*>(::GlobalLock(data));
    if (value == nullptr)
    {
        ::CloseClipboard();
        return false;
    }

    text = value;
    ::GlobalUnlock(data);
    ::CloseClipboard();
    return true;
}

bool CKeyboardClickerDlg::SetClipboardText(const CString& text) const
{
    if (!::OpenClipboard(m_hWnd))
    {
        return false;
    }

    ::EmptyClipboard();

    const SIZE_T byteCount = static_cast<SIZE_T>(text.GetLength() + 1) * sizeof(wchar_t);
    HGLOBAL data = ::GlobalAlloc(GMEM_MOVEABLE, byteCount);
    if (data == nullptr)
    {
        ::CloseClipboard();
        return false;
    }

    void* buffer = ::GlobalLock(data);
    if (buffer == nullptr)
    {
        ::GlobalFree(data);
        ::CloseClipboard();
        return false;
    }

    memcpy(buffer, static_cast<LPCWSTR>(text), byteCount);
    ::GlobalUnlock(data);

    if (::SetClipboardData(CF_UNICODETEXT, data) == nullptr)
    {
        ::GlobalFree(data);
        ::CloseClipboard();
        return false;
    }

    ::CloseClipboard();
    return true;
}

bool CKeyboardClickerDlg::SetAndVerifyClipboardText(const CString& text) const
{
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        if (!SetClipboardText(text))
        {
            Sleep(20);
            continue;
        }

        Sleep(30);

        CString verifiedText;
        if (ReadClipboardText(verifiedText) && verifiedText == text)
        {
            return true;
        }

        Sleep(30);
    }

    return false;
}

void CKeyboardClickerDlg::ClearClipboard() const
{
    if (::OpenClipboard(m_hWnd))
    {
        ::EmptyClipboard();
        ::CloseClipboard();
    }
}

void CKeyboardClickerDlg::RegisterAllShoutHotkeys()
{
    for (size_t i = 0; i < m_shoutActionCount; ++i)
    {
        RegisterShoutHotkey(i);
    }
}

void CKeyboardClickerDlg::RegisterShoutHotkey(size_t index)
{
    if (index >= MaxShoutActions)
    {
        return;
    }

    UnregisterShoutHotkey(index);

    if (index >= m_shoutActionCount)
    {
        return;
    }

    const UINT vk = m_shoutControls[index].triggerKeyEdit.CapturedKey();
    if (vk == 0)
    {
        return;
    }

    if (!RegisterHotKey(m_hWnd, HotkeyShoutBase + static_cast<int>(index), 0, vk))
    {
        CString message;
        message.Format(_T("第 %u 条喊话热键注册失败。"), static_cast<unsigned>(index + 1));
        SetStatus(message);
        return;
    }

    m_shoutStates[index].hotkeyRegistered = true;
}

void CKeyboardClickerDlg::UnregisterAllShoutHotkeys()
{
    for (size_t i = 0; i < MaxShoutActions; ++i)
    {
        UnregisterShoutHotkey(i);
    }
}

void CKeyboardClickerDlg::UnregisterShoutHotkey(size_t index)
{
    if (index >= MaxShoutActions || !m_shoutStates[index].hotkeyRegistered)
    {
        return;
    }

    UnregisterHotKey(m_hWnd, HotkeyShoutBase + static_cast<int>(index));
    m_shoutStates[index].hotkeyRegistered = false;
}

void CKeyboardClickerDlg::SuspendShoutHotkeysForKey(UINT vk, std::array<bool, MaxShoutActions>& suspended)
{
    for (size_t i = 0; i < m_shoutActionCount; ++i)
    {
        if (m_shoutStates[i].hotkeyRegistered && m_shoutControls[i].triggerKeyEdit.CapturedKey() == vk)
        {
            UnregisterShoutHotkey(i);
            suspended[i] = true;
        }
    }
}

void CKeyboardClickerDlg::RestoreSuspendedShoutHotkeys(const std::array<bool, MaxShoutActions>& suspended)
{
    for (size_t i = 0; i < m_shoutActionCount; ++i)
    {
        if (suspended[i])
        {
            RegisterShoutHotkey(i);
        }
    }
}

void CKeyboardClickerDlg::ScheduleNext(size_t index)
{
    if (index >= m_jobs.size() || !m_jobs[index].enabled)
    {
        return;
    }

    SetTimer(TimerBase + index, NextDelay(m_jobs[index].intervalMs), nullptr);
}

void CKeyboardClickerDlg::SetStatus(const CString& text)
{
    if (text == m_lastStatusText)
    {
        return;
    }

    m_lastStatusText = text;
    SetDlgItemText(IDC_STATUS, MakeSingleLineStatusText(text));
    AppendDebugText(text);
}

void CKeyboardClickerDlg::AppendDebugText(const CString& text)
{
    if (!GetSafeHwnd() || GetDlgItem(IDC_DEBUG_LOG) == nullptr)
    {
        return;
    }

    SYSTEMTIME now = {};
    GetLocalTime(&now);

    const CString compactText = CompactForDebugLine(text);
    CString line;
    line.Format(_T("[%02u:%02u:%02u] %s\r\n"), now.wHour, now.wMinute, now.wSecond, static_cast<LPCTSTR>(compactText));

    CEdit* debugLog = static_cast<CEdit*>(GetDlgItem(IDC_DEBUG_LOG));
    if (debugLog == nullptr)
    {
        return;
    }

    const int currentLength = debugLog->GetWindowTextLength();
    if (currentLength + line.GetLength() > MaxDebugTextLength)
    {
        CString currentText;
        debugLog->GetWindowText(currentText);

        const int keepStart = (std::max)(0, currentText.GetLength() - DebugTrimTargetLength);
        int trimStart = currentText.Find(_T("\r\n"), keepStart);
        if (trimStart >= 0)
        {
            trimStart += 2;
            currentText = currentText.Mid(trimStart);
        }
        else
        {
            currentText = currentText.Right(DebugTrimTargetLength);
        }

        currentText.Insert(0, _T("[日志过多，已自动保留最近内容]\r\n"));
        debugLog->SetWindowText(currentText);
    }

    debugLog->SetSel(-1, -1);
    debugLog->ReplaceSel(line, FALSE);
    debugLog->SendMessage(EM_SCROLLCARET, 0, 0);
}

CString CKeyboardClickerDlg::FormatHwnd(HWND hwnd) const
{
    CString value;
    value.Format(_T("0x%p"), hwnd);
    return value;
}

CString CKeyboardClickerDlg::ReadWindowText(HWND hwnd) const
{
    CString text;
    if (!IsWindow(hwnd))
    {
        return text;
    }

    const int length = ::GetWindowTextLength(hwnd);
    if (length <= 0)
    {
        text = _T("(无窗口标题)");
        return text;
    }

    ::GetWindowText(hwnd, text.GetBuffer(length + 1), length + 1);
    text.ReleaseBuffer();
    return text;
}

bool CKeyboardClickerDlg::TryParseVirtualKey(const CString& token, UINT& vk) const
{
    vk = 0;
    CString normalized = token;
    normalized.Trim();
    normalized.MakeUpper();

    if (normalized.GetLength() == 1)
    {
        const wchar_t ch = normalized[0];
        if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9'))
        {
            vk = static_cast<UINT>(ch);
            return true;
        }
    }

    if (normalized.GetLength() >= 2 && normalized[0] == L'F')
    {
        wchar_t* end = nullptr;
        const unsigned long functionKey = wcstoul(static_cast<LPCWSTR>(normalized) + 1, &end, 10);
        if (end != static_cast<LPCWSTR>(normalized) + 1 && *end == L'\0' && functionKey >= 1 && functionKey <= 24)
        {
            vk = static_cast<UINT>(VK_F1 + static_cast<int>(functionKey) - 1);
            return true;
        }
    }

    if (normalized == _T("ENTER") || normalized == _T("RETURN"))
    {
        vk = VK_RETURN;
        return true;
    }
    if (normalized == _T("SPACE"))
    {
        vk = VK_SPACE;
        return true;
    }
    if (normalized == _T("TAB"))
    {
        vk = VK_TAB;
        return true;
    }
    if (normalized == _T("ESC") || normalized == _T("ESCAPE"))
    {
        vk = VK_ESCAPE;
        return true;
    }
    if (normalized == _T("UP"))
    {
        vk = VK_UP;
        return true;
    }
    if (normalized == _T("DOWN"))
    {
        vk = VK_DOWN;
        return true;
    }
    if (normalized == _T("LEFT"))
    {
        vk = VK_LEFT;
        return true;
    }
    if (normalized == _T("RIGHT"))
    {
        vk = VK_RIGHT;
        return true;
    }

    return false;
}

bool CKeyboardClickerDlg::ReadUInt(int controlId, UINT& value) const
{
    CString text;
    GetDlgItemText(controlId, text);
    text.Trim();

    if (text.IsEmpty())
    {
        value = 0;
        return true;
    }

    wchar_t* end = nullptr;
    const unsigned long parsedValue = wcstoul(text, &end, 10);
    if (end == static_cast<LPCWSTR>(text) || *end != L'\0' || parsedValue > UINT_MAX)
    {
        return false;
    }

    value = static_cast<UINT>(parsedValue);
    return true;
}

bool CKeyboardClickerDlg::ReadUIntFromWindow(const CWnd& window, UINT& value) const
{
    CString text;
    window.GetWindowText(text);
    text.Trim();

    if (text.IsEmpty())
    {
        value = 0;
        return true;
    }

    wchar_t* end = nullptr;
    const unsigned long parsedValue = wcstoul(text, &end, 10);
    if (end == static_cast<LPCWSTR>(text) || *end != L'\0' || parsedValue > UINT_MAX)
    {
        return false;
    }

    value = static_cast<UINT>(parsedValue);
    return true;
}

CString CKeyboardClickerDlg::GetWindowTextString(const CWnd& window) const
{
    CString text;
    window.GetWindowText(text);
    return text;
}

UINT CKeyboardClickerDlg::NextDelay(UINT intervalMs)
{
    if (m_randomDeviationMs == 0)
    {
        return intervalMs;
    }

    const UINT minDelay = intervalMs > m_randomDeviationMs ? intervalMs - m_randomDeviationMs : 1;
    const UINT maxDelay = intervalMs > UINT_MAX - m_randomDeviationMs ? UINT_MAX : intervalMs + m_randomDeviationMs;
    std::uniform_int_distribution<UINT> distribution(minDelay, maxDelay);
    return distribution(m_rng);
}

bool CKeyboardClickerDlg::IsExtendedKey(UINT vk) const
{
    switch (vk)
    {
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_DIVIDE:
    case VK_RCONTROL:
    case VK_RMENU:
        return true;
    default:
        return false;
    }
}
