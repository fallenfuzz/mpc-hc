#include "stdafx.h"
#include "CMPCThemeInternalPropertyPageWnd.h"

CMPCThemeInternalPropertyPageWnd::~CMPCThemeInternalPropertyPageWnd() {
}
BEGIN_MESSAGE_MAP(CMPCThemeInternalPropertyPageWnd, CInternalPropertyPageWnd)
    ON_WM_CTLCOLOR()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


HBRUSH CMPCThemeInternalPropertyPageWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {
    HBRUSH ret;
    ret = getCtlColor(pDC, pWnd, nCtlColor);
    if (nullptr != ret) {
        return ret;
    } else {
        return __super::OnCtlColor(pDC, pWnd, nCtlColor);
    }
}

BOOL CMPCThemeInternalPropertyPageWnd::OnEraseBkgnd(CDC* pDC) {
    bool ret = MPCThemeEraseBkgnd(pDC, this, CTLCOLOR_DLG);
    if (ret) {
        return ret;
    } else {
        return __super::OnEraseBkgnd(pDC);
    }
}
