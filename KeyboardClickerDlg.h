#pragma once

#include <array>
#include <random>

#include <afxwin.h>
#include <afxdialogex.h>

#include "KeyCaptureEdit.h"

class CKeyboardClickerDlg : public CDialogEx
{
public:
    CKeyboardClickerDlg();

protected:
    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* pDX) override;

    afx_msg void OnDestroy();
    afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnStart();
    afx_msg void OnStop();
    afx_msg LRESULT OnKeyCaptured(WPARAM wParam, LPARAM lParam);
    DECLARE_MESSAGE_MAP()

private:
    struct KeyJob
    {
        UINT vk = 0;
        UINT intervalMs = 0;
        bool enabled = false;
    };

    struct ClickActionControls
    {
        CStatic indexLabel;
        CKeyCaptureEdit keyEdit;
        CEdit intervalEdit;
    };

    struct ShoutActionControls
    {
        CStatic indexLabel;
        CKeyCaptureEdit triggerKeyEdit;
        CEdit triggerCountEdit;
        CEdit chatDelayEdit;
        CEdit messageEdit;
        CEdit skillWaitEdit;
        CEdit sequenceEdit;
        CKeyCaptureEdit cooldownSkillKeyEdit;
    };

    struct ShoutActionState
    {
        bool hotkeyRegistered = false;
        UINT pressCount = 0;
        ULONGLONG lastPressTick = 0;
        ULONGLONG lastMessageTick = 0;
    };

    static constexpr int HotkeyCaptureTarget = 1;
    static constexpr int HotkeyShoutBase = 10;
    static constexpr size_t MaxClickActions = 50;
    static constexpr size_t MaxShoutActions = 50;
    static constexpr ULONGLONG ShoutSequenceMs = 1000;
    static constexpr ULONGLONG ShoutCooldownMs = 5000;
    static constexpr UINT_PTR TimerBase = 2000;

    void CaptureTargetUnderCursor();
    void StopJobs();
    void LoadSettings();
    void SaveSettings() const;
    bool ReadJobs();
    bool SendKey(UINT vk);
    bool SendShoutMessage(size_t index);
    void HandleShoutHotkey(size_t index);
    void TriggerCooldownSkill(size_t index);
    void OnAddClickAction();
    void OnRemoveClickAction();
    void OnAddShoutAction();
    void InitializeClickActionControls();
    void ConfigureDefaultClickActions();
    void ShowClickActionRows();
    void LayoutControls();
    void MoveDlgItem(int controlId, int x, int y, int width, int height);
    size_t VisibleClickRowsPerColumn() const;
    size_t VisibleClickRowCapacity() const;
    size_t VisibleShoutRowCapacity() const;
    void ClampScrollOffsets();
    void UpdateScrollBars();
    void InitializeShoutActionControls();
    void ConfigureDefaultShoutActions();
    void ShowShoutActionRows();
    void RegisterAllShoutHotkeys();
    void RegisterShoutHotkey(size_t index);
    void UnregisterAllShoutHotkeys();
    void UnregisterShoutHotkey(size_t index);
    void SuspendShoutHotkeysForKey(UINT vk, std::array<bool, MaxShoutActions>& suspended);
    void RestoreSuspendedShoutHotkeys(const std::array<bool, MaxShoutActions>& suspended);
    bool PrepareForegroundTarget();
    bool SendInputKey(UINT vk);
    bool SendInputShortcut(UINT modifierVk, UINT vk);
    bool SendMouseWheel(int delta);
    bool ExecuteShoutSequence(size_t index);
    bool ExecuteReportSequenceToken(const CString& token);
    bool ReadClipboardText(CString& text) const;
    bool SetClipboardText(const CString& text) const;
    bool SetAndVerifyClipboardText(const CString& text) const;
    void ClearClipboard() const;
    void ScheduleNext(size_t index);
    void SetStatus(const CString& text);
    CString FormatHwnd(HWND hwnd) const;
    CString ReadWindowText(HWND hwnd) const;
    bool TryParseVirtualKey(const CString& token, UINT& vk) const;
    bool ReadUInt(int controlId, UINT& value) const;
    bool ReadUIntFromWindow(const CWnd& window, UINT& value) const;
    CString GetWindowTextString(const CWnd& window) const;
    UINT NextDelay(UINT intervalMs);
    bool IsExtendedKey(UINT vk) const;

    HWND m_targetHwnd = nullptr;
    bool m_running = false;
    size_t m_clickActionCount = 0;
    size_t m_shoutActionCount = 0;
    size_t m_clickScrollOffset = 0;
    size_t m_shoutScrollOffset = 0;
    UINT m_randomDeviationMs = 0;
    std::mt19937 m_rng;
    CScrollBar m_clickScrollBar;
    CScrollBar m_shoutScrollBar;
    std::array<KeyJob, MaxClickActions> m_jobs = {};
    std::array<ClickActionControls, MaxClickActions> m_clickControls;
    std::array<ShoutActionControls, MaxShoutActions> m_shoutControls;
    std::array<ShoutActionState, MaxShoutActions> m_shoutStates;
};
