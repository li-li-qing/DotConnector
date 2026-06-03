#pragma once

#include <afxwin.h>

constexpr UINT WM_KEY_CAPTURED = WM_APP + 1;

class CKeyCaptureEdit : public CEdit
{
public:
    UINT CapturedKey() const { return m_vk; }
    void SetCapturedKey(UINT vk);

protected:
    afx_msg UINT OnGetDlgCode();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    DECLARE_MESSAGE_MAP()

private:
    CString FormatKeyName(UINT vk) const;
    UINT m_vk = 0;
};
