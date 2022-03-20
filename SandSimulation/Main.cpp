#include "Windows.h"
#include "memory.h"// for memset() and malloc()
#include <vector>
#include <string>
#include <random>
#include <time.h>

int randIntInRange(int min, int max)
{
	return (rand() % (max + 1 - min)) + min;
}

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
vec2 screenSize{ 1000, 800 };
vec2 cellSize{ 10,10 };

vec2 cellAmount;
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
bool operator == (const cell& c1, const cell& c2)
{
	return (c1.body.left == c2.body.left &&
		c1.body.right == c2.body.right &&
		c1.body.top == c2.body.top &&
		c1.body.bottom == c2.body.bottom &&
		c1.type == c2.type );
}

// Window:
LRESULT WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
void WinInit(vec2 screenSize);
void WinShow(HDC dc);
// Cell manipulation:
cell* CellGetCovered(vec2 cursorPos);
void CellChangeType(vec2 cursorPos, cellType type);
void CellDelete(vec2 cursorPos);
// Cell behaviour:
void WinProcess();
void goLeftFirst(int i, int j, cell* current);
void goRightFirst(int i, int j, cell* current);

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
						  WS_MINIMIZEBOX | \
						  WS_MAXIMIZEBOX | \
						  WS_MAXIMIZE /*| \
						  WS_SIZEBOX*/)

	hwnd = CreateWindow(L"WindowClass", L"Sand Simulation", WS_CUSTOMWINDOW, 0, 0, screenSize.x, screenSize.y, NULL, NULL, NULL, NULL);

	HDC dc = GetDC(hwnd);

	ShowWindow(hwnd, SW_SHOWNORMAL);

	WinInit(screenSize);

	LPPOINT cursorPos;
	cursorPos = (LPPOINT)malloc(sizeof(*cursorPos));

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
			// Getting mouse coordinates:
			GetCursorPos(cursorPos);
			ScreenToClient(hwnd, cursorPos);

			if ((GetKeyState(VK_LBUTTON) & 0x8000) != 0)
			{
				if (cursorPos->y > 20)// When cursor is not over the title bar
				{
					CellChangeType(vec2(cursorPos->x, cursorPos->y), cellType::sand);
				}
			}
			/// NOICE!
			if ((GetKeyState(VK_RBUTTON) & 0x8000) != 0)
			{
				CellDelete(vec2(cursorPos->x, cursorPos->y));
			}

			WinShow(dc);
			WinProcess();
		}
	}

	return 0;
}

int LMBit = 0;
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
	////else if (msg == WM_LBUTTONDOWN) // When LMB is clicked
	//else if ((GetKeyState(VK_LBUTTON) & 0x8000) != 0) // When LMB is pressed
	//else if (GetAsyncKeyState(VK_LBUTTON)) // Better LMB pressed, cause it doesn't go so crazy as previous.
	//{
	//	vec2 cursorPos{ LOWORD(lparam), HIWORD(lparam) };
	// 
	//	if (cursorPos.y > 20)// When cursor is not over the title bar
	//	{
	//		CellChangeType(cursorPos, cellType::sand);
	//	}
	//	else// When it is over the title bar, we want default message processing function to take control,
	//		// because otherwise resizing and closing buttons doesn't work.
	//	{
	//		DefWindowProc(hwnd, msg, wparam, lparam);
	//	}
	//}
	//// Switched to processing mouse holding in main.
	else
		return DefWindowProc(hwnd, msg, wparam, lparam);
}
void WinInit(vec2 newScreenSize)
{
	screenSize = newScreenSize;
	cellAmount = vec2( screenSize.x / cellSize.x, screenSize.y / cellSize.y );
	rowAmount = cellAmount.y;
	columnAmount = cellAmount.x;
	
	grid.resize(columnAmount);

	// Initializing the grid:
	for(int i = 0; i < columnAmount; ++i)
		for (int j = 0; j < rowAmount; ++j)
		{
			// Initializing grid cells:
			cell c;
			c.body.left = cellSize.x * i;
			c.body.right = cellSize.x * (i + 1);
			c.body.top = cellSize.y * j;
			c.body.bottom = cellSize.y * (j + 1);

			c.type = cellType::air;

			
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

cell* CellGetCovered(vec2 cursorPos)
{
	for (int i = 0; i < columnAmount; ++i)
		for (int j = 0; j < rowAmount; ++j)
		{
			if (i == 9) break;
			cell* current = &grid.at(i).at(j);
			if (cursorPos.x > current->body.left &&
				cursorPos.y > current->body.top &&
				cursorPos.x < current->body.right &&
				cursorPos.y < current->body.bottom)
			{
				return &grid.at(i).at(j);
			}
		}
}
void CellChangeType(vec2 cursorPos, cellType type)
{
	for (int i = 0; i < columnAmount; ++i)
		for (int j = 0; j < rowAmount; ++j)
		{
			if (i == 9) break;
			// Not drowing column, cells of which tend to change its type when not needed, perhaps
			// due to strange behaviour of message capture function, which sends that cursor is
			// howering this cell, when it's not. So I've decided to just turn them off.
			cell* current = &grid.at(i).at(j);
			if (cursorPos.x > current->body.left &&
				cursorPos.y > current->body.top &&
				cursorPos.x < current->body.right &&
				cursorPos.y < current->body.bottom)
			{
			//	if (current->type == cellType::air)
			//	{
			//		if (j >= 3)
			//		{
			//			{
			//				grid[i][j - 3].type = type;
			//				// For some reason by default cursor coordinates are 3 cells lower, than should be,
			//				// so we're correcting that by andjusting spawning height.
			//			}
			//		}
			//		else
			//		{
			//			grid[i][j].type = type;
			//			// For cases, when we can't go higher.
			//		}
			//	}
			//}
			//else if (cursorPos.y > screenSize.y)// For cases, when we get lower
			//{
			//	if (cursorPos.x > current->body.left &&
			//		cursorPos.x < current->body.right) // If cursor is in borders of current cell by X axis
			//	{
			//		int cellsDown = round((cursorPos.y - screenSize.y) / cellSize.y);// How many cells lower we are, in
			//		// accordance to amount of cells we have in Y axis.
			//		// e.g. screenSize.y = 100, cursorPos.y = 110, cellSize = 10 -> We are 1 cell lower.
			//		int temp = cellAmount.y + cellsDown - 3;
			//		if( temp <= cellAmount.y)// if corrected cell spawning point is within cellAmount.y
			//			if(grid[i][temp - 1].type == cellType::air)
			//				grid[i][temp - 1].type = type;
			//	}
			///// Since I've switched LMB processing to main, we have NO need in this ****, no more!
			
				grid.at(i).at(j).type = type;
			
			}

		}
}
void CellDelete(vec2 cursorPos)
{
	for (int i = 0; i < columnAmount; ++i)
		for (int j = 0; j < rowAmount; ++j)
		{
			if (i == 9) break;
			// Not drowing column, cells of which tend to change its type when not needed, perhaps
			// due to strange behaviour of message capture function, which sends that cursor is
			// howering this cell, when it's not. So I've decided to just turn them off.
			cell* current = &grid.at(i).at(j);
			if (cursorPos.x > current->body.left &&
				cursorPos.y > current->body.top &&
				cursorPos.x < current->body.right &&
				cursorPos.y < current->body.bottom)
			{
				grid.at(i).at(j).type = cellType::air;
			}

		}
}
void WinProcess()
{
	// Iterating from down to up, for the reason not to process the cells, which type just has been recently changed in the same loop.
	for (int i = columnAmount - 1; i >= 0; --i)
		for (int j = rowAmount - 1; j >= 0; --j)
		{
			cell* current = &grid.at(i).at(j);

			if (current->type == cellType::sand)
			{
				if (j + 1 < cellAmount.y)
				{
					cell* bottomNeighboor = &grid[i][j + 1];

					if (bottomNeighboor->type == cellType::air)
					{
						bottomNeighboor->type = current->type;
						current->type = cellType::air;
					}
				}

				srand(time(0));
				int rand = randIntInRange(0, 100);
				if ( rand % 2 == 0)
				{
					goLeftFirst(i, j, current);
				}
				else
				{
					goRightFirst(i, j, current);
				}

				// Why the hell I've changed else if to if and everything started to work properly!?
			}


		}
}
void goLeftFirst(int i, int j, cell* current)
{
	if (i - 1 >= 0 && j + 1 < cellAmount.y)
	{
		cell* leftDownNeighboor = &grid[i - 1][j + 1];

		if (leftDownNeighboor->type == cellType::air)
		{
			leftDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	if (i + 1 < cellAmount.x && j + 1 < cellAmount.y)
	{
		cell* rightDownNeighboor = &grid[i + 1][j + 1];

			if (rightDownNeighboor->type == cellType::air)
			{
				rightDownNeighboor->type = current->type;
					current->type = cellType::air;
			}
	}
	else{}
}
void goRightFirst(int i, int j, cell* current)
{
	if (i + 1 < cellAmount.x && j + 1 < cellAmount.y)
	{
		cell* rightDownNeighboor = &grid[i + 1][j + 1];

		if (rightDownNeighboor->type == cellType::air)
		{
			rightDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	if (i - 1 >= 0 && j + 1 < cellAmount.y)
	{
		cell* leftDownNeighboor = &grid[i - 1][j + 1];

		if (leftDownNeighboor->type == cellType::air)
		{
			leftDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	else {}
}