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

    static constexpr int HotkeyCaptureTarget = 1;
    static constexpr UINT_PTR TimerBase = 2000;

    void CaptureTargetUnderCursor();
    void StopJobs();
    bool ReadJobs();
    bool SendKey(UINT vk);
    void ScheduleNext(size_t index);
    void SetStatus(const CString& text);
    CString FormatHwnd(HWND hwnd) const;
    CString ReadWindowText(HWND hwnd) const;
    bool ReadUInt(int controlId, UINT& value) const;
    UINT NextDelay(UINT intervalMs);
    bool IsExtendedKey(UINT vk) const;
    LPARAM BuildKeyLParam(UINT vk, bool keyUp) const;

    HWND m_targetHwnd = nullptr;
    bool m_running = false;
    UINT m_randomDeviationMs = 0;
    std::mt19937 m_rng;
    std::array<KeyJob, 3> m_jobs = {};
    std::array<CKeyCaptureEdit, 3> m_keyEdits;
};
