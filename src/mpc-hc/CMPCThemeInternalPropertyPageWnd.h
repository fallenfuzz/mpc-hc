#pragma once
#include "..\src\filters\InternalPropertyPage.h"
#include "CMPCThemeUtil.h"

class CMPCThemeInternalPropertyPageWnd :
    public CInternalPropertyPageWnd
    , public CMPCThemeUtil {

    virtual ~CMPCThemeInternalPropertyPageWnd();
public:
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

