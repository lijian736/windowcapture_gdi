#ifndef _H_CAPTURE_WINDOW_GDI_H_
#define _H_CAPTURE_WINDOW_GDI_H_

#include <vector>
#include <string>
#include <functional>
#include <windows.h>

typedef std::function< void(BITMAPINFO* bmpInfo, unsigned char* data, bool minimized) > OnWindowDraw;

//when the windows screen display resolution changes, 
//the GDICaptureWindow shoud init again.
class GDICaptureWindow
{
public:
	GDICaptureWindow();
	virtual ~GDICaptureWindow();

	/**
	* @brief initialize the capturer
	* @param className -- the window class name
	*        windowName -- the window name
	* @return TRUE, FALSE
	*/
	BOOL init(const std::string& className, const std::string& windowName);

	/**
	* @brief un initialize
	* @return TRUE, FALSE
	*/
	BOOL un_init();

	/**
	* @brief capture loop
	* @param renderCursor -- should render the cursor
	* @return TRUE, FALSE
	*/
	BOOL capture_loop(BOOL renderCursor);

	/**
	* @brief set the capture callback function
	* @param fun -- the capture callback function
	*/
	void set_capture_callback(OnWindowDraw fun);

private:
	/**
	* @brief initialize
	* @param hwnd -- the captured window handle, if hwnd is NULL, then init the fullscreen
	* @return TRUE, FALSE
	*/
	BOOL init(HWND hwnd);

	/**
	* @brief draw the cursor
	* @param cursor -- the cursor info
	*/
	void draw_cursor(CURSORINFO& cursor);

private:
	BOOL m_initialized;

	//the cursor info
	CURSORINFO m_cursor;
	//the window handle
	HWND m_hwnd;
	//the device context
	HDC m_hdc;
	//the memory device context
	HDC m_mem_dc;
	//the bmp
	HBITMAP m_hbmp;
	//the old bmp
	HBITMAP m_hbmp_old;

	//the window left
	int m_left;
	//the window top
	int m_top;
	//the window width
	int m_width;
	//the window height
	int m_height;

	//the bmp info
	BITMAPINFO* m_bitmap_info;
	//the bmp buffer
	BYTE* m_bmp_buffer;

	DWORD m_line_bytes;
	DWORD m_line_stride;

	//the callback function
	OnWindowDraw m_draw_fun;
};

#endif
