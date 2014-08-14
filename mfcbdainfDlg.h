// MFCBDAINF - BDA DTV topology information application
// Copyright (C) 2014 Jens Vaaben <info@jensvaaben.com>, https://www.google.com/+JensVaaben http://www.dvbstreamexplorer.com

// mfcbdainfDlg.h : header file
//

#pragma once

#include "bdainf.h"


// CmfcbdainfDlg dialog
class CmfcbdainfDlg : public CDialogEx
{
// Construction
public:
	CmfcbdainfDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MFCBDAINF_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	CTreeCtrl m_TreeCtrl;
	void ScanBDA();
	void PopulateTree(BDADEVICES&);
	void PopulateDevices(DEVICELIST&, HTREEITEM parent);
	void PopulateDevice(DEVICE&, HTREEITEM parent);
	void PopulateTopology(TOPOLOGY&, HTREEITEM parent);
	void PopulateBdaTopology(BDATOPOLOGY&, HTREEITEM parent);
	void PopulatePinTopology(PINTOPOLOGY&, HTREEITEM parent);
	void PopulatePinInfo(PININFO&, HTREEITEM parent);
	BDADEVICES m_bdadevices;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFileSaveastextfile();
	afx_msg void OnHelpAbout();
	afx_msg void OnActionScanbdahardwaretopology();
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
};
