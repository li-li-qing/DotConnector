#include "KeyboardClickerApp.h"

#include "KeyboardClickerDlg.h"

CKeyboardClickerApp theApp;

BOOL CKeyboardClickerApp::InitInstance()
{
    CWinApp::InitInstance();
    SetRegistryKey(_T("DotConnector"));

    CKeyboardClickerDlg dialog;
    m_pMainWnd = &dialog;
    dialog.DoModal();

    return FALSE;
}
