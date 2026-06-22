#include "KeyboardClickerApp.h"

#include "KeyboardClickerDlg.h"

#include <afxole.h>

CKeyboardClickerApp theApp;

BOOL CKeyboardClickerApp::InitInstance()
{
    CWinApp::InitInstance();
    if (!AfxOleInit())
    {
        AfxMessageBox(_T("OLE 初始化失败，无法使用大漠 COM 插件。"));
        return FALSE;
    }
    SetRegistryKey(_T("DotConnector"));

    CKeyboardClickerDlg dialog;
    m_pMainWnd = &dialog;
    dialog.DoModal();

    return FALSE;
}
