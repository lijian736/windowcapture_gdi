
// WindowCapturerDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "WindowCapturer.h"
#include "WindowCapturerDlg.h"
#include "afxdialogex.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CWindowCapturerDlg 对话框



CWindowCapturerDlg::CWindowCapturerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_WINDOWCAPTURER_DIALOG, pParent)
	, m_capture_type(0), m_has_error(FALSE), m_capture_thread_handle(NULL),
	m_stop_capture_event_handle(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWindowCapturerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_FULLSCREEN, m_capture_type);
	DDX_Control(pDX, IDC_COMBO_WIN_LIST, m_combo_list);
}

BEGIN_MESSAGE_MAP(CWindowCapturerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_RADIO_FULLSCREEN, &CWindowCapturerDlg::OnBnClickedRadioFullscreen)
	ON_BN_CLICKED(IDC_RADIO_WINDOW, &CWindowCapturerDlg::OnBnClickedRadioWindow)
	ON_CBN_SELCHANGE(IDC_COMBO_WIN_LIST, &CWindowCapturerDlg::OnCbnSelchangeComboWinList)
	ON_BN_CLICKED(IDC_BTN_CAPTURE, &CWindowCapturerDlg::OnBnClickedBtnCapture)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_STOP, &CWindowCapturerDlg::OnBnClickedBtnStop)
END_MESSAGE_MAP()


// CWindowCapturerDlg 消息处理程序

BOOL CWindowCapturerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	EnumAllCapturedWindows(m_window_list);
	for (std::vector<WindowAttributes>::const_iterator it = m_window_list.begin(); it != m_window_list.end(); it++)
	{
		m_combo_list.AddString(TEXT(it->title.c_str()));
	}

	m_combo_list.SetCurSel(0);
	m_combo_list.EnableWindow(FALSE);

	m_capture_thread_handle = NULL;
	m_stop_capture_event_handle = CreateEventA(NULL, FALSE, FALSE, NULL);
	if (!m_stop_capture_event_handle)
	{
		m_has_error = TRUE;
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CWindowCapturerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CWindowCapturerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CWindowCapturerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CWindowCapturerDlg::OnBnClickedRadioFullscreen()
{
	this->UpdateData(TRUE);
	m_combo_list.EnableWindow(FALSE);
}


void CWindowCapturerDlg::OnBnClickedRadioWindow()
{
	this->UpdateData(TRUE);
	m_combo_list.EnableWindow(TRUE);
	OnCbnSelchangeComboWinList();
}


void CWindowCapturerDlg::OnCbnSelchangeComboWinList()
{
	StopDisplayThumbnail(m_thumbnail);

	int sel = m_combo_list.GetCurSel();
	if (sel >= 0)
	{
		WindowAttributes wa = m_window_list[sel];

		CRect r;
		GetDlgItem(IDC_STATIC_THUMBNAIL)->GetWindowRect(&r);
		ScreenToClient(&r);

		DisplayThumbnail(this->GetSafeHwnd(), wa.handle, m_thumbnail, r.left, r.top, r.right, r.bottom);
	}
}

void CWindowCapturerDlg::on_capture(BITMAPINFO* bmpInfo, unsigned char* data, bool minimized)
{
	CWnd* pDstWnd = GetDlgItem(IDC_STATIC_PREVIEW);
	HWND dstHWND = pDstWnd->GetSafeHwnd();
	HDC dc = ::GetDC(dstHWND);
	RECT rect;
	pDstWnd->GetWindowRect(&rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	StretchDIBits(dc, 0, 0, width, height, 0, 0, bmpInfo->bmiHeader.biWidth, bmpInfo->bmiHeader.biHeight,
		data, bmpInfo, DIB_RGB_COLORS, SRCCOPY);
}

DWORD WINAPI CWindowCapturerDlg::capture_thread(LPVOID ptr)
{
	CoInitialize(NULL);

	CWindowCapturerDlg* dlg = (CWindowCapturerDlg*)ptr;
	dlg->capture_thread_run();

	CoUninitialize();

	return 0;
}

void CWindowCapturerDlg::capture_thread_run()
{
	GDICaptureWindow* capture = new (std::nothrow) GDICaptureWindow();
	if (!capture)
	{
		return;
	}

	BOOL init;
	if (m_capture_type == 0)
	{
		init = capture->init("", "");
	}
	else
	{
		int sel = m_combo_list.GetCurSel();
		WindowAttributes wa = m_window_list[sel];

		init = capture->init(wa.className, wa.title);
	}

	if (!init)
	{
		delete capture;
		return;
	}

	capture->set_capture_callback(std::bind(&CWindowCapturerDlg::on_capture, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	DWORD ret = WaitForSingleObject(m_stop_capture_event_handle, 10);
	while (ret == WAIT_TIMEOUT)
	{
		capture->capture_loop(TRUE);

		ret = WaitForSingleObject(m_stop_capture_event_handle, 10);
	}

	delete capture;
}

void CWindowCapturerDlg::OnBnClickedBtnCapture()
{
	if (m_has_error)
	{
		return;
	}

	if (!m_capture_thread_handle)
	{
		m_capture_thread_handle = CreateThread(NULL, NULL, CWindowCapturerDlg::capture_thread, this, NULL, NULL);
		if (!m_capture_thread_handle)
		{
			return;
		}
	}
}


void CWindowCapturerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	if (m_capture_thread_handle)
	{
		SetEvent(m_stop_capture_event_handle);
		WaitForSingleObject(m_capture_thread_handle, INFINITE);
		CloseHandle(m_capture_thread_handle);
		m_capture_thread_handle = NULL;
	}
}


void CWindowCapturerDlg::OnBnClickedBtnStop()
{
	if (m_capture_thread_handle)
	{
		SetEvent(m_stop_capture_event_handle);
		WaitForSingleObject(m_capture_thread_handle, INFINITE);
		CloseHandle(m_capture_thread_handle);
		m_capture_thread_handle = NULL;
	}

	this->Invalidate(TRUE);
}
