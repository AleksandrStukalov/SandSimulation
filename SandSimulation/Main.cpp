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
	sand,
	water
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
// Cell behaviour:
void WinProcess();
void goLeftDownFirst(int x, int y, cell* current);
void goRightDownFirst(int x, int y, cell* current);
void goLeftFirst(int x, int y, cell* current);
void goRightFirst(int x, int y, cell* current);

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

			cellType type;
			// Creating sand when LMB is held:
			if ((GetKeyState(VK_LBUTTON) & 0x8000) != 0)
			{
				if (GetKeyState('S') < 0) type = cellType::sand;
				else if (GetKeyState('W') < 0) type = cellType::water;
				else type = cellType::air;
				
				if (cursorPos->y > 20)// When cursor is not over the title bar
				{
					CellChangeType(vec2(cursorPos->x, cursorPos->y), type);
				}
			}
			// "Deleting" cell when RMB is held:
			if ((GetKeyState(VK_RBUTTON) & 0x8000) != 0)
			{
				CellChangeType(vec2(cursorPos->x, cursorPos->y), cellType::air);
			}

			WinShow(dc);
			WinProcess();
		}
	}

	return 0;
}
// Window:
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
	for(int x = 0; x < columnAmount; ++x)
		for (int y = 0; y < rowAmount; ++y)
		{
			// Initializing grid cells:
			cell c;
			c.body.left = cellSize.x * x;
			c.body.right = cellSize.x * (x + 1);
			c.body.top = cellSize.y * y;
			c.body.bottom = cellSize.y * (y + 1);

			c.type = cellType::air;

			
			grid.at(x).push_back(c);
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
	for (int x = 0; x < grid.size(); ++x)
		for (int y = 0; y < grid.at(x).size(); ++y)
		{
			cell* current = &grid.at(x).at(y);

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
			else if (current->type == cellType::water)
			{
				SelectObject(memDC, GetStockObject(DC_PEN));
				SetDCPenColor(memDC, RGB(0, 0, 0));
				SelectObject(memDC, GetStockObject(DC_BRUSH));
				SetDCBrushColor(memDC, RGB(0, 117, 200));// beige color

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
// Cell manipulation:
cell* CellGetCovered(vec2 cursorPos)
{
	for (int x = 0; x < columnAmount; ++x)
		for (int y = 0; y < rowAmount; ++y)
		{
			if (x == 9) break;
			cell* current = &grid.at(x).at(y);
			if (cursorPos.x > current->body.left &&
				cursorPos.y > current->body.top &&
				cursorPos.x < current->body.right &&
				cursorPos.y < current->body.bottom)
			{
				return current;
			}
		}
	return nullptr;
}
void CellChangeType(vec2 cursorPos, cellType type)
{
	cell* cellCovered = CellGetCovered(cursorPos);
	if (cellCovered != nullptr)
		cellCovered->type = type;
	
}
// Cell behaviour:
void WinProcess()
{
	// Iterating from down to up, for the reason not to process the cells, which type just has been recently changed in the same loop.
	for (int x = columnAmount - 1; x >= 0; --x)
		for (int y = rowAmount - 1; y >= 0; --y)
		{
			cell* current = &grid.at(x).at(y);

			if (current->type == cellType::sand)
			{
				if (y + 1 < cellAmount.y)
				{
					cell* bottomNeighboor = &grid[x][y + 1];

					if (bottomNeighboor->type == cellType::air)
					{
						bottomNeighboor->type = current->type;
						current->type = cellType::air;
					}
					if (bottomNeighboor->type == cellType::water)
					{
						cellType tempType = bottomNeighboor->type;
						bottomNeighboor->type = current->type;
						current->type = tempType;
					}
				}

				srand(time(0));
				int rand = randIntInRange(0, 100);
				if ( rand % 2 == 0)
				{
					// Go left down
					if (x - 1 >= 0 && y + 1 < cellAmount.y)
					{
						cell* leftDownNeighboor = &grid[x - 1][y + 1];

						if (leftDownNeighboor->type == cellType::air)
						{
							leftDownNeighboor->type = current->type;
							current->type = cellType::air;
						}
						if(leftDownNeighboor->type == cellType::water)
						{
							leftDownNeighboor->type = current->type;
								current->type = cellType::water;
						}
					}
					// Go right down

				}
				else
				{
					if (x + 1 < cellAmount.x && y + 1 < cellAmount.y)
					{
						cell* rightDownNeighboor = &grid[x + 1][y + 1];

						if (rightDownNeighboor->type == cellType::air)
						{
							rightDownNeighboor->type = current->type;
							current->type = cellType::air;
						}
						if (rightDownNeighboor->type == cellType::water)
						{
							rightDownNeighboor->type = current->type;
							current->type = cellType::water;
						}
					}
				}
			}
			if (current->type == cellType::water)
			{
				// Sand behavior:
				if (y + 1 < cellAmount.y)
				{
					cell* bottomNeighboor = &grid[x][y + 1];

					if (bottomNeighboor->type == cellType::air)
					{
						bottomNeighboor->type = current->type;
						current->type = cellType::air;
					}
				}

				srand(time(0));
				int rand = randIntInRange(0, 100);
				if (rand % 2 == 0)
				{
					goLeftDownFirst(x, y, current);
				}
				else
				{
					goRightDownFirst(x, y, current);
				}
				// + Going to sides when possible:
				rand = randIntInRange(0, 100);
				if (rand % 2 == 0)
				{
					goRightFirst(x, y, current);
				}
				else
				{
					goLeftFirst(x, y, current);
				}
				
			}

		}
}
void goLeftDownFirst(int x, int y, cell* current)
{
	// Go left down
	if (x - 1 >= 0 && y + 1 < cellAmount.y)
	{
		cell* leftDownNeighboor = &grid[x - 1][y + 1];

		if (leftDownNeighboor->type == cellType::air)
		{
			leftDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	// Go right down
	if (x + 1 < cellAmount.x && y + 1 < cellAmount.y)
	{
		cell* rightDownNeighboor = &grid[x + 1][y + 1];

			if (rightDownNeighboor->type == cellType::air)
			{
				rightDownNeighboor->type = current->type;
					current->type = cellType::air;
			}
	}
	else{} // Stay put
}
void goRightDownFirst(int x, int y, cell* current)
{
	// Go right down
	if (x + 1 < cellAmount.x && y + 1 < cellAmount.y)
	{
		cell* rightDownNeighboor = &grid[x + 1][y + 1];

		if (rightDownNeighboor->type == cellType::air)
		{
			rightDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	// Go left down
	if (x - 1 >= 0 && y + 1 < cellAmount.y)
	{
		cell* leftDownNeighboor = &grid[x - 1][y + 1];

		if (leftDownNeighboor->type == cellType::air)
		{
			leftDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	else {}// Stay put
}
void goLeftFirst(int x, int y, cell* current)
{
	// Go left
	if (x - 1 >= 0)
	{
		cell* leftDownNeighboor = &grid[x - 1][y];

		if (leftDownNeighboor->type == cellType::air)
		{
			leftDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	// Go right
	if (x + 1 < cellAmount.x)
	{
		cell* rightDownNeighboor = &grid[x + 1][y];

		if (rightDownNeighboor->type == cellType::air)
		{
			rightDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	else {}// Stay put.
}
void goRightFirst(int x, int y, cell* current)
{
	// Go right
	if (x + 1 < cellAmount.x)
	{
		cell* rightDownNeighboor = &grid[x + 1][y];

		if (rightDownNeighboor->type == cellType::air)
		{
			rightDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	// Go left
	if (x - 1 >= 0)
	{
		cell* leftDownNeighboor = &grid[x - 1][y];

		if (leftDownNeighboor->type == cellType::air)
		{
			leftDownNeighboor->type = current->type;
			current->type = cellType::air;
		}
	}
	else {}// Stay put.
}
