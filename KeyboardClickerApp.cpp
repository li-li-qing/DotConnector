#include "KeyboardClickerApp.h"

#include "KeyboardClickerDlg.h"

#include <afxole.h>

CKeyboardClickerApp theApp;

namespace
{
CString GetExecutableDirectory()
{
    CString modulePath;
    DWORD length = 0;
    LPTSTR buffer = modulePath.GetBuffer(MAX_PATH);
    length = GetModuleFileName(nullptr, buffer, MAX_PATH);
    modulePath.ReleaseBuffer(length);

    const int slash = modulePath.ReverseFind(_T('\\'));
    if (slash >= 0)
    {
        return modulePath.Left(slash);
    }

    return _T(".");
}

void ConfigureIniProfile(CWinApp& app)
{
    const CString iniPath = GetExecutableDirectory() + _T("\\DotConnector.ini");

    free(const_cast<LPTSTR>(app.m_pszProfileName));
    app.m_pszProfileName = _tcsdup(iniPath);
    app.m_pszRegistryKey = nullptr;

    if (GetFileAttributes(iniPath) == INVALID_FILE_ATTRIBUTES)
    {
        CFile file;
        if (file.Open(iniPath, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyNone))
        {
            file.Close();
        }
    }
}
}

BOOL CKeyboardClickerApp::InitInstance()
{
    CWinApp::InitInstance();
    if (!AfxOleInit())
    {
        AfxMessageBox(_T("OLE 初始化失败，无法使用 Fairy COM 插件。"));
        return FALSE;
    }
    ConfigureIniProfile(*this);

    CKeyboardClickerDlg dialog;
    m_pMainWnd = &dialog;
    dialog.DoModal();

    return FALSE;
}
