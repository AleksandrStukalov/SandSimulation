#include "Windows.h"
#include "memory.h"// for memset() and malloc()
#include <vector>
#include <string>

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
vec2 screenSize{ 1800, 1200 };
vec2 squareSize{ 10,10 };

vec2 squareAmount;
int rowAmount;
int columnAmount;


enum class cellType
{
	air = 0,
	sand = 1
};

class cell
{
public:
	RECT body;
	cellType type;
};

LRESULT WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
void WinInit(vec2 screenSize);
void WinShow(HDC dc);
void changeCellType(vec2 cursorpos, cellType type);

RECT clientRect;

std::vector<std::vector<cell>> grid;

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
						  WS_MINIMIZEBOX )
	
	hwnd = CreateWindow(L"WindowClass", L"Sand Simulation", WS_CUSTOMWINDOW, 0, 0, screenSize.x, screenSize.y, NULL, NULL, NULL, NULL);

	HDC dc = GetDC(hwnd);

	ShowWindow(hwnd, SW_SHOWNORMAL);

	WinInit(screenSize);
	
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
	else if (msg == WM_LBUTTONDOWN)
	{
		vec2 cursorPos{ LOWORD(lparam), HIWORD(lparam) };
		changeCellType(cursorPos, cellType::sand);
	}
	else
		return DefWindowProc(hwnd, msg, wparam, lparam);
}
void WinInit(vec2 newScreenSize)
{
	screenSize = newScreenSize;
	squareAmount = vec2( screenSize.x / squareSize.x, screenSize.y / squareSize.y );
	rowAmount = squareAmount.y;
	columnAmount = squareAmount.x;
	
	grid.resize(columnAmount);

	// Initializing the grid:
	for(int i = 0; i < columnAmount; ++i)
		for (int j = 0; j < rowAmount; ++j)
		{
			// Initializing grid cells:
			cell c;
			c.body.left = squareSize.x * i;
			c.body.right = squareSize.x * (i + 1);
			c.body.top = squareSize.y * j;
			c.body.bottom = squareSize.y * (j + 1);

			// Turning cells in the first row into sand. 
			if (j == rowAmount - 1)
			{
				c.type = cellType::sand;
			}
			else
			{
				c.type = cellType::air;
			}

			
			grid.at(i).push_back(c);
		}
	
	
}
void WinShow(HDC dc)
{
	HDC memDC = CreateCompatibleDC(dc);
	int cx = clientRect.right - clientRect.left;
	int cy = clientRect.bottom - clientRect.top;
	HBITMAP memBM = CreateCompatibleBitmap(dc, cx, clientRect.bottom - clientRect.top);
	SelectObject(memDC, memBM);// Attaching memBM to memDC

	// Background:
	SelectObject(memDC, GetStockObject(DC_PEN));
	SetDCPenColor(memDC, RGB(255, 255, 255));
	SelectObject(memDC, GetStockObject(DC_BRUSH));
	SetDCBrushColor(memDC, RGB(255, 255, 255));
	Rectangle(memDC, clientRect.left, clientRect.top, clientRect.right, clientRect.bottom);

	// Grid:
	for (int i = 0; i < grid.size(); ++i)
		for (int j = 0; j < grid.at(i).size(); ++j)
		{
			cell* current = &grid.at(i).at(j);

			if (current->type == cellType::air)
			{
				// Not drawing them
			}
			else if (current->type == cellType::sand)
			{
				SelectObject(memDC, GetStockObject(DC_PEN));
				SetDCPenColor(memDC, RGB(0, 0, 0));
				SelectObject(memDC, GetStockObject(DC_BRUSH));
				SetDCBrushColor(memDC, RGB(245, 245, 220));// beige color

				Rectangle(memDC,
					current->body.left,
					current->body.top,
					current->body.right,
					current->body.bottom);
			}
			
		}


	BitBlt(dc, 0, 0, cx, cy, memDC, 0, 0, SRCCOPY);
	DeleteDC(memDC);
	DeleteObject(memBM);

}
void changeCellType(vec2 cursorPos, cellType type)
{
	for (int i = 0; i < columnAmount; ++i)
		for (int j = 0; j < rowAmount; ++j)
		{
			cell* current = &grid.at(i).at(j);
			if (cursorPos.x > current->body.left &&
				cursorPos.y > current->body.top &&
				cursorPos.x < current->body.right &&
				cursorPos.y < current->body.bottom)
			{
				current->type = cellType::sand;
			}
		}
}