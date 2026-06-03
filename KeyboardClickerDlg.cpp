#include "KeyboardClickerDlg.h"

#include "resource.h"

BEGIN_MESSAGE_MAP(CKeyboardClickerDlg, CDialogEx)
    ON_WM_DESTROY()
    ON_WM_HOTKEY()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_START, &CKeyboardClickerDlg::OnStart)
    ON_BN_CLICKED(IDC_STOP, &CKeyboardClickerDlg::OnStop)
    ON_MESSAGE(WM_KEY_CAPTURED, &CKeyboardClickerDlg::OnKeyCaptured)
END_MESSAGE_MAP()

CKeyboardClickerDlg::CKeyboardClickerDlg()
    : CDialogEx(IDD_KEYBOARDCLICKER_DIALOG)
    , m_rng(std::random_device{}())
{
}

BOOL CKeyboardClickerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_keyEdits[0].SubclassDlgItem(IDC_KEY1, this);
    m_keyEdits[1].SubclassDlgItem(IDC_KEY2, this);
    m_keyEdits[2].SubclassDlgItem(IDC_KEY3, this);

    SetDlgItemText(IDC_INTERVAL1, _T("1000"));
    SetDlgItemText(IDC_INTERVAL2, _T("1000"));
    SetDlgItemText(IDC_INTERVAL3, _T("1000"));
    SetDlgItemText(IDC_RANDOM_DEVIATION, _T("0"));

    GetDlgItem(IDC_STOP)->EnableWindow(FALSE);

    if (!RegisterHotKey(m_hWnd, HotkeyCaptureTarget, 0, VK_F9))
    {
        SetStatus(_T("F9 热键注册失败，可能已被其他程序占用。"));
    }
    else
    {
        SetStatus(_T("就绪。把鼠标移到目标窗口/控件上，然后按 F9。"));
    }

    return TRUE;
}

void CKeyboardClickerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

void CKeyboardClickerDlg::OnDestroy()
{
    StopJobs();
    UnregisterHotKey(m_hWnd, HotkeyCaptureTarget);
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

    CDialogEx::OnHotKey(nHotKeyId, nKey1, nKey2);
}

void CKeyboardClickerDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent >= TimerBase && nIDEvent < TimerBase + m_jobs.size())
    {
        const size_t index = static_cast<size_t>(nIDEvent - TimerBase);
        if (m_running && m_jobs[index].enabled)
        {
            SendKey(m_jobs[index].vk);
            ScheduleNext(index);
        }
        return;
    }

    CDialogEx::OnTimer(nIDEvent);
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
    SetStatus(_T("运行中。"));
}

void CKeyboardClickerDlg::OnStop()
{
    StopJobs();
}

LRESULT CKeyboardClickerDlg::OnKeyCaptured(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    SetStatus(_T("按键已获取。设置间隔后点击开始。"));
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
        SetStatus(_T("已停止。"));
    }
}

bool CKeyboardClickerDlg::ReadJobs()
{
    const int intervalIds[] = { IDC_INTERVAL1, IDC_INTERVAL2, IDC_INTERVAL3 };
    if (!ReadUInt(IDC_RANDOM_DEVIATION, m_randomDeviationMs))
    {
        SetStatus(_T("随机偏差必须是 0 或更大的毫秒数。"));
        return false;
    }

    for (size_t i = 0; i < m_jobs.size(); ++i)
    {
        m_jobs[i] = {};
        const UINT vk = m_keyEdits[i].CapturedKey();
        if (vk == 0)
        {
            continue;
        }

        UINT intervalMs = 0;
        if (!ReadUInt(intervalIds[i], intervalMs) || intervalMs == 0)
        {
            CString message;
            message.Format(_T("组合 %u 的间隔必须大于 0 毫秒。"), static_cast<unsigned>(i + 1));
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
    if (!IsWindow(m_targetHwnd))
    {
        StopJobs();
        SetStatus(_T("目标窗口/控件已失效。"));
        return false;
    }

    const LPARAM downParam = BuildKeyLParam(vk, false);
    const LPARAM upParam = BuildKeyLParam(vk, true);

    ::PostMessage(m_targetHwnd, WM_KEYDOWN, vk, downParam);
    ::PostMessage(m_targetHwnd, WM_KEYUP, vk, upParam);
    return true;
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
    SetDlgItemText(IDC_STATUS, text);
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

LPARAM CKeyboardClickerDlg::BuildKeyLParam(UINT vk, bool keyUp) const
{
    const UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    LPARAM value = 1 | (static_cast<LPARAM>(scanCode) << 16);

    if (IsExtendedKey(vk))
    {
        value |= (1 << 24);
    }

    if (keyUp)
    {
        value |= (1 << 30) | (1 << 31);
    }

    return value;
}
