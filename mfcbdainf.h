// MFCBDAINF - BDA DTV topology information application
// Copyright (C) 2014 Jens Vaaben <info@jensvaaben.com>, https://www.google.com/+JensVaaben http://www.dvbstreamexplorer.com

// mfcbdainf.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CmfcbdainfApp:
// See mfcbdainf.cpp for the implementation of this class
//

class CmfcbdainfApp : public CWinApp
{
public:
	CmfcbdainfApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CmfcbdainfApp theApp;