
// WindowCapturerDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "capture_util.h"
#include "capture_window_gdi.h"

// CWindowCapturerDlg �Ի���
class CWindowCapturerDlg : public CDialogEx
{
// ����
public:
	CWindowCapturerDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WINDOWCAPTURER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnBnClickedRadioFullscreen();
	afx_msg void OnBnClickedRadioWindow();
	afx_msg void OnCbnSelchangeComboWinList();
	afx_msg void OnBnClickedBtnCapture();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnDestroy();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	static DWORD WINAPI capture_thread(LPVOID ptr);
	void capture_thread_run();
	void on_capture(BITMAPINFO* bmpInfo, unsigned char* data, bool minimized);

private:
	BOOL m_has_error;

	//the capture type, fullscreen or window
	int m_capture_type;
	//the window list
	CComboBox m_combo_list;
	//the windows attributes list
	std::vector<WindowAttributes> m_window_list;
	//the thumbnail
	HTHUMBNAIL m_thumbnail;

	//the capture thread
	HANDLE m_capture_thread_handle;
	//the stop capture event
	HANDLE m_stop_capture_event_handle;
};
