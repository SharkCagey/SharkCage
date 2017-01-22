#include "stdafx.h"
#include "DialogManager.h"
#include <Windows.h>


HWND WINAPI DialogManager::CreateDialogParam(HINSTANCE hinst,
                                             LPCTSTR pszTemplate,
                                             HWND hwndParent,
                                             DLGPROC lpDlgProc,
                                             LPARAM dwInitParam) {
    HWND hdlg = NULL;
    HRSRC templateResource = FindResource(hinst, pszTemplate, RT_DIALOG);
    if (templateResource) {
        HGLOBAL allocTemplateResource = LoadResource(hinst, templateResource);
        if (allocTemplateResource) {
            LPVOID pTemplate = LockResource(allocTemplateResource); // fixed 1pm
            if (pTemplate) {
                hdlg = CreateDialogIndirectParam(hinst, (LPCDLGTEMPLATEW) pTemplate, hwndParent, lpDlgProc, dwInitParam);
            }
            FreeResource(allocTemplateResource);
        }
    }
    return hdlg;
}
void createFrameWindow() {
}
