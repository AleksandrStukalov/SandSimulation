#include "Windows.h"
#include "memory.h"// for memset() and malloc()
#include <vector>
#include <string>
#include <random>
#include <time.h>

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

// Modifiable values:
vec2 screenSize{ 200, 200 };
vec2 cellSize{ 5,5 };
int brushRadiuss{ 7 };
COLORREF waterColor = RGB(0, 117, 200);
COLORREF sandColor = RGB(245, 245, 220);

vec2 cellAmount;
int rowAmount;
int columnAmount;

enum class cellStep
{
	put,
	down,
	downLeft,
	downRight,
	left,
	right
};

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
	bool isUpdated = false;// for the reason not to update the cells, that was already updated.
	cellStep previousStep;
	int x, y;// grid coordinates.
};
bool operator == (const cell& c1, const cell& c2)
{
	return (c1.body.left == c2.body.left &&
		c1.body.right == c2.body.right &&
		c1.body.top == c2.body.top &&
		c1.body.bottom == c2.body.bottom &&
		c1.type == c2.type);
}

int randIntInRange(int min, int max)
{
	return (rand() % (max + 1 - min)) + min;
}
// Function declarations:
// Window:
LRESULT WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
void WinInit(vec2 screenSize);
void WinShow(HDC dc, vec2 cursorPos);
// Cell manipulation:
cell* CellGetCovered(vec2 cursorPos);
void CellSpawn(vec2 cursorPos, cellType type);
// Cell behaviour:
void WinProcess();
void CellSwapTypes(cell* c1, cell* c2);
bool CellIfPossibleGoDown(cell* current, int x, int y);
bool CellIfPossibleGoDownLeft(cell* current, int x, int y);
bool CellIfPossibleGoDownRight(cell* current, int x, int y);
bool CellIfPossibleGoLeft(cell* current, int x, int y);
bool CellIfPossibleGoRight(cell* current, int x, int y);

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

			cellType type;// Type of the cells to spawn
			// Creating cells when LMB is held:
			if ((GetKeyState(VK_LBUTTON) & 0x8000) != 0)
			{
				if (GetKeyState('S') < 0)
				{
					type = cellType::sand;
				}
				else if (GetKeyState('W') < 0)
				{
					type = cellType::water;
				}
				else type = cellType::air;

				if (cursorPos->y > 20)// When cursor is not over the title bar
				{
					CellSpawn(vec2(cursorPos->x, cursorPos->y), type);
				}
			}
			// "Deleting" cell when RMB is held:
			if ((GetKeyState(VK_RBUTTON) & 0x8000) != 0)
			{
				CellSpawn(vec2(cursorPos->x, cursorPos->y), cellType::air);
			}

			// Drawing cells:
			WinShow(dc, vec2(cursorPos->x, cursorPos->y));
			// Relocate cells:
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
	else
		return DefWindowProc(hwnd, msg, wparam, lparam);
}
void WinInit(vec2 newScreenSize)
{
	screenSize = newScreenSize;
	cellAmount = vec2(screenSize.x / cellSize.x, screenSize.y / cellSize.y);
	rowAmount = cellAmount.y;
	columnAmount = cellAmount.x;

	grid.resize(columnAmount);

	// Initializing the grid:
	for (int x = 0; x < columnAmount; ++x)
		for (int y = 0; y < rowAmount; ++y)
		{
			// Initializing grid cells:
			cell c;
			c.body.left = cellSize.x * x;
			c.body.right = cellSize.x * (x + 1);
			c.body.top = cellSize.y * y;
			c.body.bottom = cellSize.y * (y + 1);

			/*if (y > cellAmount.y / 2)
				c.type = cellType::water;
			else
				*/c.type = cellType::air;

			c.x = x;
			c.y = y;

			grid.at(x).push_back(c);
		}


}
void WinShow(HDC dc, vec2 cursorPos)
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
	for (int x = 0; x < columnAmount; ++x)
		for (int y = 0; y < rowAmount; ++y)
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
				SetDCBrushColor(memDC, sandColor);

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
				SetDCBrushColor(memDC, waterColor);

				Rectangle(memDC,
					current->body.left,
					current->body.top,
					current->body.right,
					current->body.bottom);
			}
			
		}

	// Cursor:
	SelectObject(memDC, GetStockObject(DC_PEN));
	SetDCPenColor(memDC, RGB(0, 0, 0));
	SelectObject(memDC, GetStockObject(DC_BRUSH));
	if (GetKeyState('S') < 0)
	{
		SetDCBrushColor(memDC, sandColor);
	}
	else if (GetKeyState('W') < 0)
	{
		SetDCBrushColor(memDC, waterColor);
	}
	else SetDCBrushColor(memDC, RGB(255, 255, 255));
	Ellipse(memDC, cursorPos.x - brushRadiuss, cursorPos.y - brushRadiuss, cursorPos.x + brushRadiuss, cursorPos.y + brushRadiuss);
	
	// Copying image from memDC to our window dc:
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
			if (x == 9) break;// Ignoring the column of cells, which tends to change type when not needed
			cell* current = &grid.at(x).at(y);
			if (cursorPos.x > current->body.left && // if it's in borders of current cell
				cursorPos.y > current->body.top &&
				cursorPos.x < current->body.right &&
				cursorPos.y < current->body.bottom)
			{
				return current;
			}
		}
	return nullptr;
}
void CellSpawn(vec2 cursorPos, cellType type)
{
	cell* covered = CellGetCovered(cursorPos);
	if (covered != nullptr)
	{
		// TODO: Make nearby cells also change type	
		// For cursor I've previously came up with a thicng, that we caņ just create an ellipse, and then, iterating over the grid,
		// check wether or not current cell is inside this ellipse.

		for (int x = 0; x < columnAmount; ++x)
			for (int y = 0; y < rowAmount; ++y)
			{
				cell* current = &grid.at(x).at(y);
				int brushTop = cursorPos.y - brushRadiuss;
				int brushRight = cursorPos.x + brushRadiuss;
				int brushBottom = cursorPos.y + brushRadiuss;
				if (current->body.left >= (cursorPos.x - brushRadiuss) &&
					current->body.top >= brushTop &&
					current->body.right <= brushRight &&
					current->body.bottom <= brushBottom)
				{
					current->type = type;
				}
				
			}

		/*covered->type = type;*/
	}

}
// Cell behaviour:
char direction = 'l';// Everytime this variable will be changed to alternative side, after going to one.
// i.e. CellGoDownLeft() executes when direction = 'l' and after execution sets direction to 'r', and conversely(vice versa).
void WinProcess()
{
	// Iterating from down to up, by the reason not to process the cells, which type just has been changed in the same loop.
	for (int x = columnAmount - 1; x >= 0; --x)
		for (int y = rowAmount - 1; y >= 0; --y)
		{
			cell* current = &grid.at(x).at(y);
			if (current->isUpdated == false)
			{
				if (current->type == cellType::sand)
				{
					if (CellIfPossibleGoDown(current, x, y)) continue;// If we went down, then part below has no need to be executed;
					// Right down then left down or conversely:
					else if (direction == 'l')
					{
						// Go down left then down right if possible:
						if (CellIfPossibleGoDownLeft(current, x, y) ||
							CellIfPossibleGoDownRight(current, x, y))
						{
							direction = 'r';
							continue;
						}
					}
					else if (direction == 'r')
					{
						// Go down right then down left if possible:
						if (CellIfPossibleGoDownRight(current, x, y) ||
							CellIfPossibleGoDownLeft(current, x, y))
						{
							direction = 'l';
							continue;
						}
					}
					else {} // Stay put
				}

				if (current->type == cellType::water)
				{
					// Sand behavior:
					if (CellIfPossibleGoDown(current, x, y))
					{
						current->isUpdated = true;
						goto end;
					}
					// Down Left and down right:
					if (direction == 'l')
					{
						// Go down left then down right if possible:
						if (CellIfPossibleGoDownLeft(current, x, y) ||
							CellIfPossibleGoDownRight(current, x, y))
						{
							direction = 'r';
							current->isUpdated = true;
							goto end;
						}
					}
					else if (direction == 'r')
					{
						// Go down right then down left if possible:
						if (CellIfPossibleGoDownRight(current, x, y) ||
							CellIfPossibleGoDownLeft(current, x, y))
						{
							direction = 'l';
							current->isUpdated = true;
							goto end;
						}
					}

					// Keep moving left or right:
					if (current->previousStep == cellStep::left)
					{
						CellIfPossibleGoLeft(current, x, y);
						current->isUpdated = true;
						goto end;
					}
					else if (current->previousStep == cellStep::right)
					{
						CellIfPossibleGoRight(current, x, y);
						current->isUpdated = true;
						goto end;
					}

					if (current->previousStep == cellStep::put)
					{
						// Left and right:
						if (direction == 'l')
						{
							if (CellIfPossibleGoLeft(current, x, y) ||
								CellIfPossibleGoRight(current, x, y))
							{
								direction = 'r';
								current->isUpdated = true;
								goto end;
							}
						}
						if (direction == 'r')
						{
							if (CellIfPossibleGoRight(current, x, y) ||
								CellIfPossibleGoLeft(current, x, y))
							{
								direction = 'l';
								current->isUpdated = true;
								goto end;
							}
						}

					}
					
					/*if(CellIfPossibleGoRight(current, x, y)) continue;
					if(CellIfPossibleGoLeft(current, x, y)) continue;*/

						
				end:
					if(current->isUpdated == false)
						current->previousStep = cellStep::put;
				}
			}
		}

	// Reloading grid:
	for (int x = columnAmount - 1; x >= 0; --x)
		for (int y = rowAmount - 1; y >= 0; --y)
			grid.at(x).at(y).isUpdated = false;
}

void CellSwapTypes(cell* c1, cell* c2)
{
	cellType tempType = c1->type;
	c1->type = c2->type;
	c2->type = tempType;

	c2->isUpdated = true;
}

bool CellIfPossibleGoDown(cell* current, int x, int y)
{
	if (y + 1 < cellAmount.y)// if we haven't reached the bottom end
	{
		cell* bottomNeighboor = &grid[x][y + 1];
		if (current->type == cellType::sand)
		{
			if (bottomNeighboor->type == cellType::air/* || bottomNeighboor->type == cellType::water*/)
			{
				CellSwapTypes(current, bottomNeighboor);// Move down
				bottomNeighboor->previousStep = cellStep::down;
				return true;
			}
			if (bottomNeighboor->type == cellType::water)
			{
				// Make sand sink:
				bottomNeighboor->type = cellType::sand;
				// Make water flow on the surface randomly:
				srand(time(0));
				int rand;
				while (true)
				{
					rand = randIntInRange(-5, 5);
					if (x + rand < cellAmount.x &&
						x + rand >= 0)
						break;
				}
				cell* randCell = &grid.at(x + rand).at(y);
				while (true)
				{
					if (randCell->type == cellType::air)// if we've reached the water surface
					{
						randCell->type = cellType::water;
						break;
					}
					else// if we are still underwater
					{
						// Take cell on top and repeat process
						randCell = &grid.at(randCell->x).at(randCell->y - 1);
					}
					
				}
				current->type = cellType::air;

				bottomNeighboor->previousStep = cellStep::down;
				return true;
			}
			
		}
		if (current->type == cellType::water)
		{
			if (bottomNeighboor->type == cellType::air)
			{
				CellSwapTypes(current, bottomNeighboor);// Move down
				bottomNeighboor->previousStep = cellStep::down;
				return true;
			}
		}
	}
	return false;
}
bool CellIfPossibleGoDownLeft(cell* current, int x, int y)
{
	if (x - 1 >= 0 &&		 // if we haven't reached the left end
		y + 1 < cellAmount.y)// if we haven't reached the bottom end
	{
		cell* downLeftNeighboor = &grid[x - 1][y + 1];
		if (current->type == cellType::sand)
		{
			if (downLeftNeighboor->type == cellType::air || downLeftNeighboor->type == cellType::water)
			{
				CellSwapTypes(current, downLeftNeighboor);// Move left down
				downLeftNeighboor->previousStep = cellStep::downLeft;
				return true;
			}
		}
		if (current->type == cellType::water)
		{
			if (downLeftNeighboor->type == cellType::air)
			{
				CellSwapTypes(current, downLeftNeighboor);// Move left down
				downLeftNeighboor->previousStep = cellStep::downLeft;
				return true;
			}
		}
	}
	return false;
}
bool CellIfPossibleGoDownRight(cell* current, int x, int y)
{
	if (x + 1 < cellAmount.x && // if we haven't reached the right end
		y + 1 < cellAmount.y)	// if we haven't reached the bottom end
	{
		cell* downRightNeighboor = &grid[x + 1][y + 1];
		if (current->type == cellType::sand)
		{
			if (downRightNeighboor->type == cellType::air || downRightNeighboor->type == cellType::water)
			{
				CellSwapTypes(current, downRightNeighboor);// Move right down
				downRightNeighboor->previousStep = cellStep::downRight;
				return true;
			}
		}
		if (current->type == cellType::water)
		{
			if (downRightNeighboor->type == cellType::air)
			{
				CellSwapTypes(current, downRightNeighboor);// Move right down
				downRightNeighboor->previousStep = cellStep::downRight;
				return true;
			}
		}
	}
	return false;
}
bool CellIfPossibleGoLeft(cell* current, int x, int y)
{
	if (x - 1 >= 0)// if we haven't reached left end
	{
		cell* leftNeighboor = &grid[x - 1][y];
		if (current->type == cellType::sand)
		{
			if (leftNeighboor->type == cellType::air || leftNeighboor->type == cellType::water)
			{
				CellSwapTypes(current, leftNeighboor);// Move left
				leftNeighboor->previousStep = cellStep::left;
				return true;
			}
		}
		if (current->type == cellType::water)
		{
			if (leftNeighboor->type == cellType::air)
			{
				CellSwapTypes(current, leftNeighboor);// Move left
				leftNeighboor->previousStep = cellStep::left;
				return true;
			}
		}
	}
	return false;
}
bool CellIfPossibleGoRight(cell* current, int x, int y)
{
	if (x + 1 < cellAmount.x)// if we haven't reached right end
	{
		cell* rightNeighboor = &grid[x + 1][y];

		if (current->type == cellType::sand)
		{
			if (rightNeighboor->type == cellType::air || rightNeighboor->type == cellType::water)
			{
				CellSwapTypes(current, rightNeighboor);// Move right
				rightNeighboor->previousStep = cellStep::right;
				return true;
			}
		}
		if (current->type == cellType::water)
		{
			if (rightNeighboor->type == cellType::air)
			{
				CellSwapTypes(current, rightNeighboor);// Move right
				rightNeighboor->previousStep = cellStep::right;
				return true;
			}
		}
	}
	return false;
}