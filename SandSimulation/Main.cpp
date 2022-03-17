
// Plan:
// * Divide screen into an array of squares
//	So, screen will be a two dimansional array of squares.
//	We can get amount of squares by dividing screen size by square size.
// 
// Let's spawn a square, where mouse is clicked.
// 
//

#include "Windows.h"
#include "memory.h"// for memset()

class vec2
{
public:
	int x, y;

	vec2(int x, int y)
	{
		this->x = x;
		this->y = y;
	}
	vec2()
	{
		this->x = 0;
		this->y = 0;
	}

};

LRESULT WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
void WinInit();
void WinShow(HDC dc);

vec2 initialScreenSize{ 1800, 1200 };
RECT ScreenSize;
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdline, int nCmdShow)
{
	WNDCLASS wcl;
	memset(&wcl, 0, sizeof(WNDCLASS));
	wcl.lpfnWndProc = WndProc;// Setting message proccesing function
	wcl.lpszClassName = L"WindowClass";
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wcl);
	
	/*(WS_OVERLAPPED | \
		WS_CAPTION | \
		WS_SYSMENU | \
		WS_THICKFRAME | \
		WS_MINIMIZEBOX | \
		WS_MAXIMIZEBOX)*/
	HWND hwnd;
#define WS_CUSTOMWINDOW (WS_VISIBLE | \
						 WS_CAPTION | \
						 WS_SYSMENU | \
						 WS_SIZEBOX | \
						 WS_MINIMIZEBOX | \
						 WS_MAXIMIZEBOX)
	
	hwnd = CreateWindow(L"WindowClass", L"Sand Simulation", WS_CUSTOMWINDOW, 0, 0, initialScreenSize.x, initialScreenSize.y, NULL, NULL, NULL, NULL);

	HDC dc = GetDC(hwnd);

	ShowWindow(hwnd, SW_SHOWNORMAL);


	MSG msg;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			WinShow(dc);
		}
	}

	return 0;
}

LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
		PostQuitMessage(0);
	else if (msg == WM_SIZE)
		GetClientRect(hwnd, &ScreenSize);
	else if (msg == WM_CHAR)
	{
		if (wparam == VK_ESCAPE)
			PostQuitMessage(0);
	}
	else
		return DefWindowProc(hwnd, msg, wparam, lparam);
}
void Wininit()
{
	// Making screen a two dimensional array of squares:
}
void WinShow(HDC dc)
{
	HDC memDC = CreateCompatibleDC(dc);
	int cx = ScreenSize.right - ScreenSize.left;
	int cy = ScreenSize.bottom - ScreenSize.top;
	HBITMAP memBM = CreateCompatibleBitmap(dc, cx, ScreenSize.bottom - ScreenSize.top);
	SelectObject(memDC, memBM);// Attaching memBM to memDC

	// White background
	SelectObject(memDC, GetStockObject(DC_PEN));
	SetDCPenColor(memDC, RGB(255, 255, 255));
	SelectObject(memDC, GetStockObject(DC_BRUSH));
	SetDCBrushColor(memDC, RGB(255, 255, 255));
	Rectangle(memDC, ScreenSize.left, ScreenSize.top, ScreenSize.right, ScreenSize.bottom);


	BitBlt(dc, 0, 0, cx, cy, memDC, 0, 0, SRCCOPY);
	DeleteDC(memDC);
	DeleteObject(memBM);
}