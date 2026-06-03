#include "KeyCaptureEdit.h"

BEGIN_MESSAGE_MAP(CKeyCaptureEdit, CEdit)
    ON_WM_GETDLGCODE()
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
END_MESSAGE_MAP()

UINT CKeyCaptureEdit::OnGetDlgCode()
{
    return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

void CKeyCaptureEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    UNREFERENCED_PARAMETER(nRepCnt);
    UNREFERENCED_PARAMETER(nFlags);

    SetCapturedKey(nChar);

    if (CWnd* parent = GetParent())
    {
        parent->SendMessage(WM_KEY_CAPTURED, static_cast<WPARAM>(GetDlgCtrlID()), static_cast<LPARAM>(nChar));
    }
}

void CKeyCaptureEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    UNREFERENCED_PARAMETER(nChar);
    UNREFERENCED_PARAMETER(nRepCnt);
    UNREFERENCED_PARAMETER(nFlags);
}

void CKeyCaptureEdit::SetCapturedKey(UINT vk)
{
    m_vk = vk;
    SetWindowText(FormatKeyName(vk));
}

CString CKeyCaptureEdit::FormatKeyName(UINT vk) const
{
    TCHAR name[128] = {};
    const UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
    LONG lParam = static_cast<LONG>(scanCode << 16);

    switch (vk)
    {
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_INSERT:
    case VK_DELETE:
    case VK_DIVIDE:
    case VK_NUMLOCK:
        lParam |= (1 << 24);
        break;
    default:
        break;
    }

    if (GetKeyNameText(lParam, name, static_cast<int>(_countof(name))) > 0)
    {
        return CString(name);
    }

    CString fallback;
    fallback.Format(_T("VK_%02X"), vk);
    return fallback;
}
