// MFCBDAINF - BDA DTV topology information application
// Copyright (C) 2014 Jens Vaaben <info@jensvaaben.com>, https://www.google.com/+JensVaaben http://www.dvbstreamexplorer.com


// mfcbdainfDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mfcbdainf.h"
#include "mfcbdainfDlg.h"
#include "afxdialogex.h"
#include <stdio.h>
#include "afxcmn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

bool SaveXMLDoc(LPCWSTR file, BDADEVICES& d);
bool LoadXMLDoc(LPCWSTR file, BDADEVICES& d);

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	CLinkCtrl m_link;
	virtual BOOL OnInitDialog();
	afx_msg void OnNMClickLink(NMHDR *pNMHDR, LRESULT *pResult);
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LINK, m_link);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_NOTIFY(NM_CLICK, IDC_LINK, &CAboutDlg::OnNMClickLink)
END_MESSAGE_MAP()


// CmfcbdainfDlg dialog



CmfcbdainfDlg::CmfcbdainfDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CmfcbdainfDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CmfcbdainfDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CmfcbdainfDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_SAVEASTEXTFILE, &CmfcbdainfDlg::OnFileSaveastextfile)
	ON_COMMAND(ID_HELP_ABOUT, &CmfcbdainfDlg::OnHelpAbout)
	ON_COMMAND(ID_ACTION_SCANBDAHARDWARETOPOLOGY, &CmfcbdainfDlg::OnActionScanbdahardwaretopology)
	ON_COMMAND(ID_FILE_OPEN, &CmfcbdainfDlg::OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, &CmfcbdainfDlg::OnFileSave)
END_MESSAGE_MAP()


// CmfcbdainfDlg message handlers

BOOL CmfcbdainfDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	RECT rect;
	GetClientRect(&rect);

	m_TreeCtrl.Create(WS_VISIBLE|WS_TABSTOP|WS_CHILD|TVS_HASBUTTONS|TVS_LINESATROOT|TVS_HASLINES,rect,this,IDD_TREEVIEW);
	ScanBDA();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CmfcbdainfDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CmfcbdainfDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CmfcbdainfDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CmfcbdainfDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if(::IsWindow(m_TreeCtrl.m_hWnd))
	{
		RECT clientrect;
		GetClientRect(&clientrect);
		m_TreeCtrl.MoveWindow(&clientrect);
	}
}

void CmfcbdainfDlg::ScanBDA()
{
	scan_bda_decives(m_bdadevices);
	PopulateTree(m_bdadevices);
}

void CmfcbdainfDlg::PopulateTree(BDADEVICES& d)
{
	m_TreeCtrl.DeleteAllItems();
	HTREEITEM htreetuner = m_TreeCtrl.InsertItem(L"KSCATEGORY_BDA_NETWORK_TUNER");
	HTREEITEM htreereceiver_component = m_TreeCtrl.InsertItem(L"KSCATEGORY_BDA_RECEIVER_COMPONENT");

	PopulateDevices(d.bda_source,htreetuner);
	PopulateDevices(d.bda_reciever,htreereceiver_component);
}

void CmfcbdainfDlg::PopulateDevices(DEVICELIST& d, HTREEITEM parent)
{
	for(size_t n=0;n<d.size();n++)
	{
		HTREEITEM item = m_TreeCtrl.InsertItem(d[n].DeviceDesc.c_str(),parent);
		PopulateDevice(d[n],item);
	}
}

void CmfcbdainfDlg::PopulateDevice(DEVICE& d, HTREEITEM parent)
{
	WCHAR buf[1024];
	swprintf_s(buf,_countof(buf),L"device path: %s",d.device_path.c_str());
	m_TreeCtrl.InsertItem(buf,parent);

	swprintf_s(buf,_countof(buf),L"Service: %s",d.Service.c_str());
	m_TreeCtrl.InsertItem(buf,parent);

	swprintf_s(buf,_countof(buf),L"device_instance_id: %s",d.device_instance_id.c_str());
	m_TreeCtrl.InsertItem(buf,parent);


	HTREEITEM htreebdatopology = m_TreeCtrl.InsertItem(L"KSPROPSETID_BdaTopology",parent);
	HTREEITEM htreetopology = m_TreeCtrl.InsertItem(L"KSPROPSETID_Topology",parent);
	HTREEITEM htreepintopology = m_TreeCtrl.InsertItem(L"KSPROPSETID_Pin",parent);

	PopulateBdaTopology(d.bdatopology,htreebdatopology);
	PopulateTopology(d.topology,htreetopology);
	PopulatePinTopology(d.pintopology,htreepintopology);
}

void CmfcbdainfDlg::PopulateTopology(TOPOLOGY& d, HTREEITEM parent)
{
	WCHAR buf[1024];
	HTREEITEM htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_TOPOLOGY_CATEGORIES",parent);
	for(size_t n=0;n<d.categories.size();n++)
	{
		std::wstring strguid;
		guid_to_string(&d.categories[n],strguid);
		swprintf_s(buf,_countof(buf),L"%s",strguid.c_str());
		m_TreeCtrl.InsertItem(buf,htree);
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_TOPOLOGY_CONNECTIONS",parent);
	for(size_t n=0;n<d.connection.size();n++)
	{
		printf("[%u]\n",n);
		swprintf_s(buf,_countof(buf),L"%u",n);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);
		swprintf_s(buf,_countof(buf),L"FromNode=%lu",d.connection[n].FromNode);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"FromNodePin=%lu",d.connection[n].FromNodePin);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"ToNode=%lu",d.connection[n].ToNode);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"ToNodePin=%lu",d.connection[n].ToNodePin);
		m_TreeCtrl.InsertItem(buf,htree2);
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_TOPOLOGY_NAME",parent);
	for(size_t n=0;n<d.name.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"[%u] %s",n,d.name[n].c_str());
		m_TreeCtrl.InsertItem(buf,htree);
	}


	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_TOPOLOGY_NODES",parent);
}

void CmfcbdainfDlg::PopulateBdaTopology(BDATOPOLOGY& d, HTREEITEM parent)
{
	WCHAR buf[1024];

	HTREEITEM htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_BDA_TEMPLATE_CONNECTIONS",parent);
	for(size_t n=0;n<d.bda_template_connection.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"FromNodeType=%lu FromNodePinType=%lu ToNodeType=%lu ToNodePinType=%lu",
			d.bda_template_connection[n].FromNodeType,
			d.bda_template_connection[n].FromNodePinType,
			d.bda_template_connection[n].ToNodeType,
			d.bda_template_connection[n].ToNodePinType
			);
		m_TreeCtrl.InsertItem(buf,htree);
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_BDA_NODE_PROPERTIES",parent);
	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=d.bda_node_properties.begin();it!=d.bda_node_properties.end();it++)
	{
		swprintf_s(buf,_countof(buf),L"node ID [%lu]",it->first);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);
		for(size_t n=0;n<it->second.size();n++)
		{
			std::wstring guid;
			guid_to_string(&(it->second[n]),guid);
			swprintf_s(buf,_countof(buf),L"%s",guid.c_str());
			m_TreeCtrl.InsertItem(buf,htree2);
		}
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_BDA_NODE_METHODS",parent);
	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=d.bda_node_methods.begin();it!=d.bda_node_methods.end();it++)
	{
		swprintf_s(buf,_countof(buf),L"node ID [%lu]",it->first);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);
		for(size_t n=0;n<it->second.size();n++)
		{
			std::wstring guid;
			guid_to_string(&(it->second[n]),guid);
			swprintf_s(buf,_countof(buf),L"%s",guid.c_str());
			m_TreeCtrl.InsertItem(buf,htree2);
		}
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_BDA_NODE_EVENTS",parent);
	for(std::map<unsigned long,std::vector<GUID> >::const_iterator it=d.bda_node_events.begin();it!=d.bda_node_events.end();it++)
	{
		swprintf_s(buf,_countof(buf),L"node ID [%lu]",it->first);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);
		for(size_t n=0;n<it->second.size();n++)
		{
			std::wstring guid;
			guid_to_string(&(it->second[n]),guid);
			swprintf_s(buf,_countof(buf),L"%s",guid.c_str());
			m_TreeCtrl.InsertItem(buf,htree2);
		}
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_BDA_NODE_DESCRIPTORS",parent);
	for(size_t n=0;n<d.bdanode_descriptor.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"[%lu]",n);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);
		swprintf_s(buf,_countof(buf),L"ulBdaNodeType [%lu]",d.bdanode_descriptor[n].ulBdaNodeType);
		m_TreeCtrl.InsertItem(buf,htree2);
		std::wstring guid1, guid2;
		guid_to_string(&d.bdanode_descriptor[n].guidFunction,guid1);
		guid_to_string(&d.bdanode_descriptor[n].guidName,guid2);
		swprintf_s(buf,_countof(buf),L"guidFunction %s",guid1.c_str());
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"guidName %s",guid2.c_str());
		m_TreeCtrl.InsertItem(buf,htree2);

	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_BDA_NODE_TYPES",parent);
	for(size_t n=0;n<d.bda_node_types.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"[%d] %lu",n,d.bda_node_types[n]);
		m_TreeCtrl.InsertItem(buf,htree);
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_BDA_PIN_TYPES",parent);
	for(size_t n=0;n<d.bda_pin_types.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"[%d] %lu",n,d.bda_pin_types[n]);
		m_TreeCtrl.InsertItem(buf,htree);
	}
}

void CmfcbdainfDlg::PopulatePinTopology(PINTOPOLOGY& d, HTREEITEM parent)
{
	WCHAR buf[1024];

	swprintf_s(buf,_countof(buf),L"KSPROPERTY_PIN_CTYPES: %lu",d.pin_ctypes);


	HTREEITEM htree = m_TreeCtrl.InsertItem(buf,parent);
	for(size_t n=0;n<d.pininfo.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"[%lu]",n);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);
		PopulatePinInfo(d.pininfo[n],htree2);
	}
}

void CmfcbdainfDlg::PopulatePinInfo(PININFO& d, HTREEITEM parent)
{
	WCHAR buf[1024];

	HTREEITEM htree = NULL;

	if(d.category_valid)
	{

		std::wstring strguid;
		guid_to_string(&d.category,strguid);

		swprintf_s(buf,_countof(buf),L"KSPROPERTY_PIN_CATEGORY: %s",strguid.c_str());
		m_TreeCtrl.InsertItem(buf,parent);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_CATEGORY not retrieved",parent);
	}

	if(d.cinstances_valid)
	{
		htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_CINSTANCES",parent);

		swprintf_s(buf,_countof(buf),L"PossibleCount: %lu",d.cinstances.PossibleCount);
		m_TreeCtrl.InsertItem(buf,htree);

		swprintf_s(buf,_countof(buf),L"CurrentCount=%lu",d.cinstances.CurrentCount);
		m_TreeCtrl.InsertItem(buf,htree);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_CINSTANCES not retrieved",parent);
	}

	if(d.communication_valid)
	{
		std::wstring strtmp;
		if(d.communication==KSPIN_COMMUNICATION_NONE)
		{
			strtmp=L"KSPIN_COMMUNICATION_NONE";
		}
		else if(d.communication==KSPIN_COMMUNICATION_SINK)
		{
			strtmp=L"KSPIN_COMMUNICATION_SINK";
		}
		else if(d.communication==KSPIN_COMMUNICATION_SOURCE)
		{
			strtmp=L"KSPIN_COMMUNICATION_SOURCE";
		}
		else if(d.communication==KSPIN_COMMUNICATION_BOTH)
		{
			strtmp=L"KSPIN_COMMUNICATION_BOTH";
		}
		else if(d.communication==KSPIN_COMMUNICATION_BRIDGE)
		{
			strtmp=L"KSPIN_COMMUNICATION_BRIDGE";
		}
		else
		{
			WCHAR buf[64];
			_itow_s(d.communication,buf,_countof(buf),10);
			strtmp=buf;
		}
		swprintf_s(buf,_countof(buf),L"KSPROPERTY_PIN_COMMUNICATION: %s",strtmp.c_str());
		m_TreeCtrl.InsertItem(buf,parent);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_COMMUNICATION not retrieved",parent);
	}

	if(d.constraineddataranges_valid)
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_CONSTRAINEDDATARANGES not retrieved",parent);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_CONSTRAINEDDATARANGES not retrieved",parent);
	}

	if(d.dataflow_valid)
	{
		std::wstring strtmp;
		if(d.dataflow==KSPIN_DATAFLOW_IN)
		{
			strtmp=L"KSPIN_DATAFLOW_IN";
		}
		else if(d.dataflow==KSPIN_DATAFLOW_OUT)
		{
			strtmp=L"KSPIN_DATAFLOW_OUT";
		}
		else
		{
			WCHAR buf[64];
			_itow_s(d.dataflow,buf,_countof(buf),10);
			strtmp=buf;
		}
		swprintf_s(buf,_countof(buf),L"KSPROPERTY_PIN_DATAFLOW: %s",strtmp.c_str());
		m_TreeCtrl.InsertItem(buf,parent);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_DATAFLOW not retrieved",parent);
	}

	if(d.dataintersection_valid)
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_DATAINTERSECTION",parent);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_DATAINTERSECTION not retrieved",parent);
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_DATARANGES",parent);
	for(size_t n=0;n<d.dataranges.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"[%lu]",n);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);

		std::wstring MajorFormat, SubFormat, Specifier;

		guid_to_string(&d.dataranges[n].MajorFormat,MajorFormat);
		guid_to_string(&d.dataranges[n].SubFormat,SubFormat);
		guid_to_string(&d.dataranges[n].Specifier,Specifier);

		swprintf_s(buf,_countof(buf),L"FormatSize: %lu",d.dataranges[n].FormatSize);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"Flags: %lu",d.dataranges[n].Flags);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"SampleSize: %lu",d.dataranges[n].SampleSize);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"Reserved: %lu",d.dataranges[n].Reserved);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"MajorFormat: %s",MajorFormat.c_str());
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"SubFormat: %s",SubFormat.c_str());
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"Specifier: %s",Specifier.c_str());
		m_TreeCtrl.InsertItem(buf,htree2);
	}

	if(d.globalcinstances_valid)
	{
		htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_GLOBALCINSTANCES",parent);
		swprintf_s(buf,_countof(buf),L"PossibleCount=%lu",d.globalcinstances.PossibleCount);
		m_TreeCtrl.InsertItem(buf,htree);
		swprintf_s(buf,_countof(buf),L"CurrentCount=%lu",d.globalcinstances.CurrentCount);
		m_TreeCtrl.InsertItem(buf,htree);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_GLOBALCINSTANCES not retrieved",parent);
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_INTERFACES",parent);
	for(size_t n=0;n<d.interfaces.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"[%lu]",n);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);

		std::wstring strguid;
		guid_to_string(&d.interfaces[n].Set ,strguid);
		swprintf_s(buf,_countof(buf),L"Set: %s",strguid.c_str());
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"Id: %lu",d.interfaces[n].Id);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"Flags: 0x%04x",d.interfaces[n].Flags);
		m_TreeCtrl.InsertItem(buf,htree2);
	}

	htree = m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_MEDIUMS",parent);
	for(size_t n=0;n<d.mediums.size();n++)
	{
		swprintf_s(buf,_countof(buf),L"[%lu]",n);
		HTREEITEM htree2 = m_TreeCtrl.InsertItem(buf,htree);

		std::wstring strguid;
		guid_to_string(&d.mediums[n].Set ,strguid);
		swprintf_s(buf,_countof(buf),L"Set: %s",strguid.c_str());
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"Id: %lu",d.mediums[n].Id);
		m_TreeCtrl.InsertItem(buf,htree2);
		swprintf_s(buf,_countof(buf),L"Flags: 0x%04x",d.mediums[n].Flags);
		m_TreeCtrl.InsertItem(buf,htree2);
	}

	swprintf_s(buf,_countof(buf),L"KSPROPERTY_PIN_NAME: %s",d.name.c_str());
	m_TreeCtrl.InsertItem(buf,parent);

	if(d.necessaryinstances_valid)
	{
		swprintf_s(buf,_countof(buf),L"KSPROPERTY_PIN_NECESSARYINSTANCES: %lu",d.necessaryinstances);
		m_TreeCtrl.InsertItem(buf,parent);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_NECESSARYINSTANCES not retrieved",parent);
	}

	if(d.physicalconnection_valid)
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_PHYSICALCONNECTION",parent);
	}
	else
	{
		m_TreeCtrl.InsertItem(L"KSPROPERTY_PIN_PHYSICALCONNECTION not retrieved",parent);
	}
}

void CmfcbdainfDlg::OnFileSaveastextfile()
{
	CFileDialog dlg(FALSE,L"txt",NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,L"Text files (*.txt)|*.txt|All files (*.*)|*.*||",this,0,TRUE);
	if(dlg.DoModal()==IDOK)
	{

		CString str = dlg.GetPathName();
		FILE* f;
		errno_t e = _wfopen_s(&f,(LPCWSTR)str,L"wt");
		if(e==0)
		{
			dump_device_list(f,m_bdadevices);
			fclose(f);
		}
	}
}

void CmfcbdainfDlg::OnHelpAbout()
{
	CAboutDlg dlg;
	dlg.DoModal();
}


void CmfcbdainfDlg::OnActionScanbdahardwaretopology()
{
	ScanBDA();
}


BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_link.SetItemUrl(0,L"https://www.google.com/+JensVaaben");
	m_link.SetItemUrl(1,L"http://www.dvbstreamexplorer.com");

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CAboutDlg::OnNMClickLink(NMHDR *pNMHDR, LRESULT *pResult)
{
	PNMLINK pClick = (PNMLINK)pNMHDR;
	ShellExecute(NULL,L"open",pClick->item.szUrl,NULL,NULL,SW_SHOWDEFAULT );
	*pResult = 0;
}


void CmfcbdainfDlg::OnFileOpen()
{
	CFileDialog dlg(TRUE,L"txt",NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,L"XML files (*.xml)|*.xml|All files (*.*)|*.*||",this,0,TRUE);
	if(dlg.DoModal()==IDOK)
	{

		CString str = dlg.GetPathName();
		try
		{
			LoadXMLDoc(str,m_bdadevices);
			PopulateTree(m_bdadevices);
		}
		catch(_com_error e)
		{
			CString str;
			str.Format(IDS_ERRORLOAD,e.ErrorMessage());
			MessageBox(str,NULL,MB_OK|MB_ICONERROR );
		}
	}
}


void CmfcbdainfDlg::OnFileSave()
{
	CFileDialog dlg(FALSE,L"txt",NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,L"XML files (*.xml)|*.xml|All files (*.*)|*.*||",this,0,TRUE);
	if(dlg.DoModal()==IDOK)
	{

		CString str = dlg.GetPathName();
		try
		{
			SaveXMLDoc(str,m_bdadevices);
		}
		catch(_com_error e)
		{
			CString str;
			str.Format(IDS_ERRORSAVE,e.ErrorMessage());
			MessageBox(str,NULL,MB_OK|MB_ICONERROR );
		}
	}
}
