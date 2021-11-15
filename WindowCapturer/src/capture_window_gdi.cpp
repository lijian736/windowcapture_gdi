#include "capture_window_gdi.h"
#include <dwmapi.h>

GDICaptureWindow::GDICaptureWindow()
{
	m_initialized = FALSE;

	m_hwnd = NULL;
	m_hdc = NULL;
	m_mem_dc = NULL;
	m_hbmp = NULL;
	m_hbmp_old = NULL;
	m_bitmap_info = NULL;
	m_bmp_buffer = NULL;

	m_line_bytes = 0;
	m_line_stride = 0;

	m_left = m_top = m_width = m_height = 0;
}

GDICaptureWindow::~GDICaptureWindow()
{
	un_init();
}

BOOL GDICaptureWindow::init(const std::string& className, const std::string& windowName)
{
	LPCSTR lpClassName = NULL;
	LPCSTR lpWindowName = NULL;

	if (className.length() > 0)
	{
		lpClassName = className.c_str();
	}
	if (windowName.length() > 0)
	{
		lpWindowName = windowName.c_str();
	}

	if (!(lpClassName || lpWindowName))
	{
		//initialize the fullscreen
		return init(NULL);
	}

	HWND handle = FindWindowA(lpClassName, lpWindowName);
	if (handle == NULL)
	{
		return FALSE;
	}

	return init(handle);
}

BOOL GDICaptureWindow::init(HWND hwnd_p)
{
	if (m_initialized)
	{
		return TRUE;
	}

	un_init();

	m_hwnd = hwnd_p;

	LONG cx = 0;
	LONG cy = 0;
	DWORD bitcount = 0;

	//this->m_hdc = GetDC(m_hwnd);
	this->m_hdc = GetWindowDC(m_hwnd);
	if (!this->m_hdc)
	{
		return FALSE;
	}

	this->m_mem_dc = CreateCompatibleDC(this->m_hdc);
	if (!m_mem_dc)
	{
		un_init();
		return FALSE;
	}

	//the desktop screen
	if (m_hwnd == NULL)
	{
		DEVMODE devmode;
		memset(&devmode, 0, sizeof(DEVMODE));
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		//get the current display device settings
		BOOL ret = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
		if (!ret)
		{
			un_init();
			return FALSE;
		}

		cx = devmode.dmPelsWidth;
		cy = devmode.dmPelsHeight;
		bitcount = devmode.dmBitsPerPel;
		if (bitcount != 16 && bitcount != 24 && bitcount != 32)
		{
			//if bitcount equals 8, the screen display device is too old.
			un_init();
			return FALSE;
		}

		m_line_bytes = cx * bitcount / 8;
		m_line_stride = (m_line_bytes + 3) / 4 * 4;

		BITMAPINFO* bi = new BITMAPINFO();
		memset(bi, 0, sizeof(BITMAPINFO));

		BITMAPINFOHEADER* bih = &(bi->bmiHeader);
		bih->biSize = sizeof(BITMAPINFOHEADER);
		bih->biWidth = cx;
		bih->biHeight = cy;
		bih->biPlanes = 1;
		bih->biBitCount = (WORD)bitcount;
		bih->biCompression = BI_RGB;
		bih->biSizeImage = 0;

		BYTE* bits = NULL;
		this->m_hbmp = CreateDIBSection(m_hdc, bi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
		if (!(this->m_hbmp) || !bits)
		{
			delete bi;

			un_init();
			return FALSE;
		}

		this->m_bitmap_info = bi;
		this->m_bmp_buffer = bits;
		this->m_left = this->m_top = 0;
	}
	else
	{
		//the window

		//the dimension in screen coordinates
		RECT rect;
		/*if (!GetWindowRect(m_hwnd, &rect))
		{
		return FALSE;
		}*/

		HRESULT hr = DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
		if (FAILED(hr))
		{
			return FALSE;
		}

		this->m_left = rect.left;
		this->m_top = rect.top;

		cx = rect.right - rect.left;
		cy = rect.bottom - rect.top;

		BITMAPINFO* bi = new BITMAPINFO();
		memset(bi, 0, sizeof(BITMAPINFO));

		BITMAPINFOHEADER* bih = &(bi->bmiHeader);
		bih->biSize = sizeof(BITMAPINFOHEADER);
		bih->biBitCount = 32;
		bih->biWidth = cx;
		bih->biHeight = cy;
		bih->biPlanes = 1;
		bih->biCompression = BI_RGB;
		bih->biSizeImage = 0;

		BYTE* bits = NULL;
		this->m_hbmp = CreateDIBSection(m_hdc, bi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
		if (!(this->m_hbmp) || !bits)
		{
			delete bi;
			un_init();
			return FALSE;
		}

		m_line_bytes = cx * 32 / 8;
		m_line_stride = (m_line_bytes + 3) / 4 * 4;

		this->m_bitmap_info = bi;
		this->m_bmp_buffer = bits;
	}

	this->m_hbmp_old = (HBITMAP)SelectObject(m_mem_dc, this->m_hbmp);
	if (!(this->m_hbmp_old))
	{
		un_init();
		return FALSE;
	}

	this->m_width = cx;
	this->m_height = cy;
	m_initialized = TRUE;
	return TRUE;
}


BOOL GDICaptureWindow::un_init()
{
	if (m_mem_dc && m_hbmp_old)
	{
		SelectObject(m_mem_dc, m_hbmp_old);
	}

	if (m_mem_dc)
	{
		DeleteDC(m_mem_dc);
	}

	if (m_hdc)
	{
		ReleaseDC(m_hwnd, m_hdc);
	}

	if (m_hbmp)
	{
		DeleteObject(m_hbmp);
	}

	m_hdc = NULL;
	m_mem_dc = NULL;
	m_hbmp = NULL;
	m_hbmp_old = NULL;
	m_hwnd = NULL;

	m_bmp_buffer = NULL;
	if (m_bitmap_info)
	{
		delete m_bitmap_info;
		m_bitmap_info = NULL;
	}

	m_line_bytes = 0;
	m_line_stride = 0;
	m_initialized = FALSE;

	return TRUE;
}

BOOL GDICaptureWindow::capture_loop(BOOL renderCursor)
{
	if (!m_initialized)
	{
		return FALSE;
	}

	bool minimized = (GetWindowLong(m_hwnd, GWL_STYLE) & WS_MINIMIZE) != 0;

	if (m_width > 0 && m_height > 0)
	{
		BOOL ret = BitBlt(m_mem_dc, 0, 0, m_width, m_height, m_hdc, 0, 0, SRCCOPY); //SRCCOPY| CAPTUREBLT
		if (!ret)
		{
			return FALSE;
		}

		if (renderCursor)
		{
			memset(&m_cursor, 0, sizeof(CURSORINFO));
			m_cursor.cbSize = sizeof(CURSORINFO);
			GetCursorInfo(&m_cursor);
			draw_cursor(m_cursor);
		}

		GdiFlush();
		m_draw_fun(m_bitmap_info, m_bmp_buffer, minimized);
	}

	RECT rect;
	if (S_OK == DwmGetWindowAttribute(m_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect)))
		//if (GetWindowRect(hwnd, &rect))
	{
		LONG cx = rect.right - rect.left;
		LONG cy = rect.bottom - rect.top;

		if (abs(this->m_width - cx) > 1 || abs(this->m_height - cy) > 1)
		{
			HWND tmpHwnd = this->m_hwnd;
			un_init();
			init(tmpHwnd);
		}
	}

	return TRUE;
}

void GDICaptureWindow::draw_cursor(CURSORINFO& cursor)
{
	HICON icon;
	ICONINFO iconInfo;
	POINT winLeftTopCorner = { m_left, m_top };

	if (!(cursor.flags & CURSOR_SHOWING))
	{
		return;
	}

	icon = CopyIcon(cursor.hCursor);
	if (!icon)
	{
		return;
	}

	if (cursor.ptScreenPos.x <= m_left || cursor.ptScreenPos.x >= (m_left + m_width)
		|| cursor.ptScreenPos.y <= m_top || cursor.ptScreenPos.y >= (m_top + m_height))
	{
		return;
	}

	if (GetIconInfo(icon, &iconInfo))
	{
		POINT pos;

		if (m_hwnd && ClientToScreen(m_hwnd, &winLeftTopCorner))
		{
			pos.x = cursor.ptScreenPos.x - (int)iconInfo.xHotspot - winLeftTopCorner.x;
			pos.y = cursor.ptScreenPos.y - (int)iconInfo.yHotspot - winLeftTopCorner.y;

			DrawIconEx(m_mem_dc, pos.x, pos.y, icon, 0, 0, 0, NULL, DI_NORMAL);

			DeleteObject(iconInfo.hbmColor);
			DeleteObject(iconInfo.hbmMask);
		}
	}

	DestroyIcon(icon);
}


void GDICaptureWindow::set_capture_callback(OnWindowDraw fun)
{
	this->m_draw_fun = fun;
}