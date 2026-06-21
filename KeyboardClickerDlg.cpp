#include "KeyboardClickerDlg.h"

#include "resource.h"

#include <algorithm>
#include <cstring>

namespace
{
constexpr LPCTSTR SettingsSection = _T("Settings");
constexpr int LayoutMargin = 12;
constexpr int LayoutGap = 10;
constexpr int TargetGroupHeight = 70;
constexpr int ClickGroupHeight = 210;
constexpr int StatusHeight = 20;
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

int ClickControlId(size_t index, int offset)
{
    return ClickControlBaseLocal + static_cast<int>(index) * ClickControlStride + offset;
}

int ActionControlId(size_t index, int offset)
{
    return ActionControlBaseLocal + static_cast<int>(index) * ActionControlStride + offset;
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

    InitializeClickActionControls();
    InitializeShoutActionControls();
    m_clickScrollBar.Create(WS_CHILD | WS_VISIBLE | SBS_VERT, CRect(0, 0, 0, 0), this, IDC_CLICK_SCROLL);
    m_shoutScrollBar.Create(WS_CHILD | WS_VISIBLE | SBS_VERT, CRect(0, 0, 0, 0), this, IDC_SHOUT_SCROLL);

    SetDlgItemText(IDC_RANDOM_DEVIATION, _T("0"));
    ConfigureDefaultClickActions();
    ConfigureDefaultShoutActions();
    LoadSettings();

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
    SaveSettings();
    StopJobs();
    UnregisterHotKey(m_hWnd, HotkeyCaptureTarget);
    UnregisterAllShoutHotkeys();
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
    if (nHotKeyId >= HotkeyShoutBase && nHotKeyId < HotkeyShoutBase + static_cast<int>(MaxShoutActions))
    {
        HandleShoutHotkey(static_cast<size_t>(nHotKeyId - HotkeyShoutBase));
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

void CKeyboardClickerDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    LayoutControls();
}

void CKeyboardClickerDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
    CDialogEx::OnGetMinMaxInfo(lpMMI);
    lpMMI->ptMinTrackSize.x = 760;
    lpMMI->ptMinTrackSize.y = 500;
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

    UNREFERENCED_PARAMETER(lParam);
    SaveSettings();
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
    const int clickY = LayoutMargin + TargetGroupHeight + LayoutGap;
    const int rowsTop = clickY + ClickRowsOffsetY;
    const int rowsBottom = clickY + ClickGroupHeight - ClickFooterReservedHeight;
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
    const int shoutY = LayoutMargin + TargetGroupHeight + LayoutGap + ClickGroupHeight + LayoutGap;
    const int shoutH = client.Height() - shoutY - StatusHeight - LayoutMargin * 2;
    const int rowsTop = shoutY + ShoutRowsOffsetY;
    const int rowsBottom = shoutY + (std::max)(100, shoutH) - 12;
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

    CRect client;
    GetClientRect(&client);
    const int margin = LayoutMargin;
    const int gap = LayoutGap;
    const int width = client.Width();
    const int height = client.Height();
    const int groupWidth = width - margin * 2;
    const int innerLeft = margin + 14;
    const int innerRight = width - margin - 14;

    const int targetY = margin;
    const int targetH = TargetGroupHeight;
    MoveDlgItem(IDC_TARGET_GROUP, margin, targetY, groupWidth, targetH);
    MoveDlgItem(IDC_TARGET_HWND_LABEL, innerLeft, targetY + 24, 62, 18);
    MoveDlgItem(IDC_TARGET_HWND, innerLeft + 68, targetY + 21, 170, 22);
    MoveDlgItem(IDC_TARGET_HINT, innerLeft + 250, targetY + 24, innerRight - (innerLeft + 250), 18);
    MoveDlgItem(IDC_TARGET_TITLE_LABEL, innerLeft, targetY + 48, 62, 18);
    MoveDlgItem(IDC_TARGET_TITLE, innerLeft + 68, targetY + 45, innerRight - (innerLeft + 68), 22);

    const int clickY = targetY + targetH + gap;
    const int clickH = ClickGroupHeight;
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
        m_clickScrollBar.MoveWindow(innerRight - ScrollBarWidth, clickRowY, ScrollBarWidth, clickH - ClickRowsOffsetY - ClickFooterReservedHeight);
    }
    const int clickFooterY = clickY + clickH - 34;
    MoveDlgItem(IDC_RANDOM_DEVIATION_LABEL, innerRight - 276, clickFooterY + 5, 96, 18);
    MoveDlgItem(IDC_RANDOM_DEVIATION, innerRight - 176, clickFooterY + 2, 54, 22);
    MoveDlgItem(IDC_START, innerRight - 112, clickFooterY - 1, 50, 26);
    MoveDlgItem(IDC_STOP, innerRight - 54, clickFooterY - 1, 50, 26);

    const int statusH = StatusHeight;
    const int shoutY = clickY + clickH + gap;
    const int shoutH = height - shoutY - statusH - margin * 2;
    MoveDlgItem(IDC_SHOUT_GROUP, margin, shoutY, groupWidth, shoutH);
    MoveDlgItem(IDC_ADD_SHOUT_ACTION, innerRight - 70, shoutY + 16, 62, 24);

    const int shoutHeaderY = shoutY + ShoutHeaderOffsetY;
    const int shoutRowY = shoutY + ShoutRowsOffsetY;
    const int cdW = 58;
    const int shoutListRight = innerRight - ScrollBarWidth - 8;
    const int cdX = shoutListRight - cdW;
    const int seqX = cdX - 154;
    const int waitX = seqX - 64;
    const int msgX = innerLeft + 168;
    const int msgW = waitX - msgX - 10;
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
    MoveDlgItem(IDC_STATUS, margin, height - margin - statusH, groupWidth, statusH);
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

void CKeyboardClickerDlg::ShowShoutActionRows()
{
    if (CWnd* addButton = GetDlgItem(IDC_ADD_SHOUT_ACTION))
    {
        addButton->EnableWindow(m_shoutActionCount < MaxShoutActions);
    }
    ClampScrollOffsets();
    UpdateScrollBars();
}

void CKeyboardClickerDlg::OnAddShoutAction()
{
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
    if (index >= m_shoutActionCount)
    {
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

    if (!PrepareForegroundTarget())
    {
        return false;
    }

    CString text;
    m_shoutControls[index].messageEdit.GetWindowText(text);
    text.Trim();
    if (text.IsEmpty())
    {
        SetStatus(_T("团队消息不能为空。"));
        return false;
    }

    UINT reportDelayMs = 0;
    if (!ReadUIntFromWindow(m_shoutControls[index].chatDelayEdit, reportDelayMs) || reportDelayMs > 1000)
    {
        SetStatus(_T("聊天间隔必须是 0 到 1000 毫秒。"));
        return false;
    }
    SaveSettings();

    CString oldClipboardText;
    const bool hadClipboardText = ReadClipboardText(oldClipboardText);
    if (!SetAndVerifyClipboardText(text))
    {
        SetStatus(_T("剪贴板未更新为报点文字，已取消发送。"));
        return false;
    }

    m_shoutStates[index].lastMessageTick = now;
    Sleep(30);
    SendInputKey(VK_RETURN);
    Sleep(reportDelayMs);
    SendInputShortcut(VK_CONTROL, 'V');
    Sleep(reportDelayMs);
    SendInputKey(VK_RETURN);
    Sleep(reportDelayMs);

    if (hadClipboardText)
    {
        SetClipboardText(oldClipboardText);
    }
    else
    {
        ClearClipboard();
    }

    SetStatus(_T("报点已发送。"));
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

        const bool reportSent = SendShoutMessage(index);
        if (reportSent && startDelayMs > 0)
        {
            CString waitMessage;
            waitMessage.Format(_T("报点已发送，等待 %u 毫秒后放技能。"), static_cast<unsigned>(startDelayMs));
            SetStatus(waitMessage);
            Sleep(startDelayMs);
        }

        if (reportSent)
        {
            ExecuteShoutSequence(index);
        }
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

    ::SetForegroundWindow(root);
    Sleep(40);
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
            vk = VK_F1 + static_cast<UINT>(functionKey) - 1;
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

