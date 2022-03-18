#include "Windows.h"
#include "memory.h"// for memset() and malloc()
#include <vector>

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
const vec2 initialScreenSize{ 1800, 1200 };
const vec2 squareSize{ 10,10 };


LRESULT WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
void WinInit(vec2 screenSize);
void WinShow(HDC dc);

RECT clientRect;

std::vector<std::vector<RECT>> grid;

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) 
{
	WNDCLASS wcl;
	memset(&wcl, 0, sizeof(WNDCLASS));
	wcl.lpfnWndProc = WndProc;// Setting message proccesing function
	wcl.lpszClassName = L"WindowClass";
	wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&wcl);
	
	HWND hwnd;
#define WS_CUSTOMWINDOW ( WS_OVERLAPPED | \
						  WS_CAPTION | \
						  WS_SYSMENU | \
						  WS_SIZEBOX | \
						  WS_MINIMIZEBOX | \
						  WS_MAXIMIZEBOX )
	
	hwnd = CreateWindow(L"WindowClass", L"Sand Simulation", WS_CUSTOMWINDOW, 0, 0, initialScreenSize.x, initialScreenSize.y, NULL, NULL, NULL, NULL);

	HDC dc = GetDC(hwnd);

	ShowWindow(hwnd, SW_SHOWNORMAL);

	WinInit(initialScreenSize);
	
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
	{
		GetClientRect(hwnd, &clientRect);
		WinInit(vec2(clientRect.right, clientRect.bottom));
	}
	else if (msg == WM_CHAR)
	{
		if (wparam == VK_ESCAPE)
			PostQuitMessage(0);
	}
	else
		return DefWindowProc(hwnd, msg, wparam, lparam);
}
void WinInit(vec2 screenSize)
{
	vec2 squareAmount{ screenSize.x / squareSize.x, screenSize.y / squareSize.y };
	int rowAmount = squareAmount.y;
	int columnAmount = squareAmount.x;
	
	grid.resize(columnAmount);

	// Initializing the grid:
	for(int i = 0; i < columnAmount; ++i)
		for (int j = 0; j < rowAmount; ++j)
		{
			// Initializing grid elements with squares:
			RECT square;
			square.left = squareSize.x * i;
			square.right = squareSize.x * (i + 1);
			square.top = squareSize.y * j;
			square.bottom = squareSize.y * (j + 1);
			grid.at(i).push_back(square);

		}
	
	
}
void WinShow(HDC dc)
{
	HDC memDC = CreateCompatibleDC(dc);
	int cx = clientRect.right - clientRect.left;
	int cy = clientRect.bottom - clientRect.top;
	HBITMAP memBM = CreateCompatibleBitmap(dc, cx, clientRect.bottom - clientRect.top);
	SelectObject(memDC, memBM);// Attaching memBM to memDC

	// White background:
	SelectObject(memDC, GetStockObject(DC_PEN));
	SetDCPenColor(memDC, RGB(255, 255, 255));
	SelectObject(memDC, GetStockObject(DC_BRUSH));
	SetDCBrushColor(memDC, RGB(255, 255, 255));
	Rectangle(memDC, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

	
	// Grid:
	SelectObject(memDC, GetStockObject(DC_PEN));
	SetDCPenColor(memDC, RGB(0, 0, 0));
	SelectObject(memDC, GetStockObject(DC_BRUSH));
	SetDCBrushColor(memDC, RGB(255, 255, 255));

	
	for (int i = 0; i < grid.size(); ++i)
		for (int j = 0; j < grid.at(i).size(); ++j)
		{
			Rectangle(memDC,
				grid.at(i).at(j).left,
				grid.at(i).at(j).top,
				grid.at(i).at(j).right,
				grid.at(i).at(j).bottom);
		}


	BitBlt(dc, 0, 0, cx, cy, memDC, 0, 0, SRCCOPY);
	DeleteDC(memDC);
	DeleteObject(memBM);

}