// TetrisGame_zjy.cpp
// by Vincent Zhang
// 2018.4.10

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "Shapes.h"

#define ID_TIMER  1
#define TIME_INTERVAL 1000   // 下落时间间隔1秒

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID    CALLBACK TimerProc(HWND, UINT, UINT, DWORD);

/*---------------宏-------------------*/
#define BOARD_WIDTH 180
#define BOARD_HEIGHT 400
#define LONG_SLEEP 300

#define COLS 15  // 列数
#define ROWS 30  // 行数
#define EXTENDED_COLS 23
#define EXTENDED_ROWS 34

#define BOARD_LEFT 4
#define BOARD_RIGHT 18
#define BOARD_TOP 0
#define BOARD_BOTTOM 29
/*-----------------------------------*/


/*-------------参数声明---------------*/
// static int shapes[7][4][4];
static int shape[4][4];
static int score = 0;

static int shape_row = 0;  // 当前形状所在行
static int shape_col = EXTENDED_COLS / 2 - 2; // 当前形状所在列

static int **gBoard;

static int lattices_top = 40;   // 上面留白
static int lattices_left = 20;  // 左侧留白
static int width = BOARD_WIDTH / COLS;                    //每个格子的宽度
static int height = (BOARD_HEIGHT - lattices_top) / ROWS; //每个格子的高度

static HBRUSH grey_brush = CreateSolidBrush(RGB(210, 210, 210));
static HBRUSH white_brush = CreateSolidBrush(RGB(130, 130, 130));
static HPEN hPen = CreatePen(PS_SOLID, 1, RGB(147, 155, 166));

static bool gIsPause = false;  // 判断是否暂停
/*-----------------------------------*/

/*-------------函数声明---------------*/
void InitGame(HWND);
void InitData();

void TypeInstruction(HWND);

void RandShape();  // 随机选择一个图形

void AddScore();   // 清空一行后加100分

void UpdateShapeRect(HWND hwnd); // 更新下落形状矩形区域
void UpdateAllBoard(HWND hwnd);  // 更新游戏范围

void FallToGround();
void MoveDown(HWND hwnd);   // 下降一格
void RePaintBoard(HDC hdc); // 重绘游戏界面
void PaintCell(HDC hdc, int x, int y, int color); // 绘制指定一个格子
void ClearFullLine();       // 清空满行

void RotateShape(HWND hwnd);  // 图形变形
void MoveHori(HWND hwnd, int direction);  // 水平移动
void RotateMatrix();          // 逆时针翻转图形
void ReRotateMatrix();        // 顺时针翻转图形
bool IsLegel();               // 检测图形是否超出范围

void RespondKey(HWND hwnd, WPARAM wParam); // 响应按键

void PauseGame(HWND hwnd); // 暂停游戏
void WakeGame(HWND hwnd);  // 继续游戏


bool JudgeLose();          // 判断是否输
void LoseGame(HWND hwnd);  // 游戏输了之后处理
void ExitGame(HWND hwnd);  // 游戏结束
/*-----------------------------------*/



// 程序入口 WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = TEXT("TetrisGame_Zjy");
	HWND         hwnd;
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("Program requires Windows NT!"),
			szAppName, MB_ICONERROR);
		return 0;
	}

	// 这里将窗口设置为无法调节大小并且不能最大化
	hwnd = CreateWindow(szAppName, TEXT("Tetris Game"),
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		BOARD_WIDTH + 220, BOARD_HEIGHT + 70,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	// 打印说明
	TypeInstruction(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HDC hdc;
	static HDC hdcBuffer;
	static HBITMAP hBitMap;
	static PAINTSTRUCT ps;

	switch (message)
	{
	case WM_CREATE:
		SetTimer(hwnd, ID_TIMER, TIME_INTERVAL, TimerProc);
		InitGame(hwnd);
		TypeInstruction(hwnd);
		return 0;

	// 最小化恢复后需要重绘说明
	case WM_SIZE:
		TypeInstruction(hwnd);
		return 0;

	case WM_KEYDOWN:
		RespondKey(hwnd, wParam);
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		RePaintBoard(hdc);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		KillTimer(hwnd, ID_TIMER);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

// 计时器响应事件
VOID CALLBACK TimerProc(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime)
{	
	// 计时器每秒向下移动
	MoveDown(hwnd);
}

// 初始化游戏
void InitGame(HWND hwnd) {

	gBoard = new int* [EXTENDED_ROWS];
	for (int i = 0; i < EXTENDED_ROWS; i++) {
		gBoard[i] = new int[EXTENDED_COLS];
	}

	srand(time(0));
	
	InitData();
	
	UpdateAllBoard(hwnd);
}

// 初始化游戏数据
void InitData() {

	// 将游戏面板清零
	for (int i = 0; i < EXTENDED_ROWS; i++) {
		for (int j = 0; j < EXTENDED_COLS; j++) {
			gBoard[i][j] = 0;
		}
	}

	// 将外围填充1，为了判断是否超出范围
	for (int i = 0; i < EXTENDED_ROWS; i++) {
		for (int j = 0; j < BOARD_LEFT; j++) {
			gBoard[i][j] = 1;
		}
	}
	for (int i = 0; i < EXTENDED_ROWS; i++) {
		for (int j = BOARD_RIGHT + 1; j < EXTENDED_COLS; j++) {
			gBoard[i][j] = 1;
		}
	}
	for (int i = BOARD_BOTTOM + 1; i < EXTENDED_ROWS; i++) {
		for (int j = 0; j < EXTENDED_COLS; j++) {
			gBoard[i][j] = 1;
		}
	}

	gIsPause = false;

	// 初始化分数
	score = 0;
	
	// 随机生成图形
	RandShape();
	
	return;
}

// 打印说明
void TypeInstruction(HWND hwnd) {

	TEXTMETRIC  tm;
	int cxChar, cxCaps, cyChar, cxClient, cyClient, iMaxWidth;

	HDC hdc = GetDC(hwnd);

	// 保存行高字宽信息
	GetTextMetrics(hdc, &tm);
	cxChar = tm.tmAveCharWidth;
	cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2;
	cyChar = tm.tmHeight + tm.tmExternalLeading;

	int startX = 180;
	int startY = 40;

	TCHAR Instruction[100];

	wsprintf(Instruction, TEXT("INSTRUCTION "));
	TextOut(hdc, startX + 40, startY, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("↑       Change Shape"));
	TextOut(hdc, startX + 40, startY + cyChar * 3, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("←       Move Left"));
	TextOut(hdc, startX + 40, startY + cyChar * 5, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("→       Move Right"));
	TextOut(hdc, startX + 40, startY + cyChar * 7, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("↓       Move Down"));
	TextOut(hdc, startX + 40, startY + cyChar * 9, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("Space Pause the game"));
	TextOut(hdc, startX + 40, startY + cyChar * 11, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("Esc     Exit the game"));
	TextOut(hdc, startX + 40, startY + cyChar * 13, Instruction, lstrlen(Instruction));

	wsprintf(Instruction, TEXT("©   2018.4   Vincent Zhang"));
	TextOut(hdc, startX + 40, startY + cyChar * 18, Instruction, lstrlen(Instruction));

	ReleaseDC(hwnd, hdc);
}

// 随机选中一个图形
void RandShape() {

	int shape_num = rand() % 7;

	for (int i = 0; i < 4; i++) 
		for (int j = 0; j < 4; j++) 
			shape[i][j] = shapes[shape_num][i][j];

}

// 更新整个游戏界面
void UpdateAllBoard(HWND hwnd) {

	static RECT rect;

	rect.left = lattices_left;
	rect.right = lattices_left + COLS * width + width;
	rect.top = lattices_top - 30;
	rect.bottom = lattices_top + ROWS * height;

	InvalidateRect(hwnd, &rect, false);

}

// 更新下落形状所在矩形区域内
void UpdateShapeRect(HWND hwnd) {

	static RECT rect;

	rect.left = lattices_left;
	rect.right = lattices_left + COLS * width + width;
	rect.top = lattices_top + (shape_row - 1) * height;
	rect.bottom = lattices_top + (shape_row + 4) * height;
	
	InvalidateRect(hwnd, &rect, false);
}

void RePaintBoard(HDC hdc) {

	SetBkColor(hdc, RGB(255, 255, 255));
	SelectObject(hdc, hPen);   //选用画笔
	TCHAR score_str[50];

	// 绘制当前分数
	wsprintf(score_str, TEXT("Score: %6d     "), score);
	TextOut(hdc, 20, 15, score_str, lstrlen(score_str));

	// 绘制游戏界面背景
	for (int i = BOARD_TOP; i <= BOARD_BOTTOM; i++) {
		for (int j = BOARD_LEFT; j <= BOARD_RIGHT; j++) {
			PaintCell(hdc, i, j, gBoard[i][j]);
		}
	}

	// 绘制正在下降的图形
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j<4; j++) {
			if (shape[i][j] == 1)
				PaintCell(hdc, shape_row + i, shape_col + j, shape[i][j]);
		}
	}
}

// 打印指定位置指定颜色的方格
void PaintCell(HDC hdc, int x, int y, int color) {

	// 超出范围直接结束
	if (x < BOARD_TOP || x > BOARD_BOTTOM || 
		y < BOARD_LEFT || y > BOARD_RIGHT) {
		return;
	}

	x -= BOARD_TOP;
	y -= BOARD_LEFT;

	// 将坐标换算为实际的像素点
	int _left = lattices_left + y * width;
	int _right = lattices_left + y * width + width;
	int _top = lattices_top + x * height;
	int _bottom = lattices_top + x * height + height;

	// 绘制边框
	MoveToEx(hdc, _left, _top, NULL);
	LineTo(hdc, _right, _top);
	MoveToEx(hdc, _left, _top, NULL);
	LineTo(hdc, _left, _bottom);
	MoveToEx(hdc, _left, _bottom, NULL);
	LineTo(hdc, _right, _bottom);
	MoveToEx(hdc, _right, _top, NULL);
	LineTo(hdc, _right, _bottom);

	// 如果color为1则用灰色画笔
	SelectObject(hdc, grey_brush);
	if (color == 0) {
		SelectObject(hdc, white_brush);
	}

	// 填充
	Rectangle(hdc, _left, _top, _right, _bottom);
}

// 响应按键
void RespondKey(HWND hwnd, WPARAM wParam) {

	if (wParam == VK_ESCAPE) {//ESC退出
		ExitGame(hwnd);
		return;
	}
	if (wParam == VK_SPACE) {//空格暂停
		gIsPause = !gIsPause;
		if (gIsPause == true) {
			PauseGame(hwnd);
			return;
		}
		else {
			WakeGame(hwnd);
			return;
		}
	}
	
	if (!gIsPause) {
		if (wParam == VK_UP) {
			RotateShape(hwnd);
			return;
		}
		if (wParam == VK_DOWN) {
			MoveDown(hwnd);
			return;
		}
		if (wParam == VK_LEFT) {
			MoveHori(hwnd, 0);
			return;
		}
		if (wParam == VK_RIGHT) {
			MoveHori(hwnd, 1);
			return;
		}
	}
}

// 停止计数器
void PauseGame(HWND hwnd) {
	KillTimer(hwnd, ID_TIMER);
}

// 重启计数器
void WakeGame(HWND hwnd) {
	SetTimer(hwnd, ID_TIMER, TIME_INTERVAL, TimerProc);
}

// 退出游戏
void ExitGame(HWND hwnd) {
	
	// 先暂停游戏
	SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);
	
	// 是否退出
	int flag = MessageBox(NULL, TEXT("Do you want exit?"), TEXT("EXIT"), MB_YESNO);

	if (flag == IDYES) {
		SendMessage(hwnd, WM_DESTROY, NULL, 0);
	}
	else if (flag == IDNO) {
		return;
	}

}

// 图形变形
void RotateShape(HWND hwnd) {
	
	RotateMatrix();
	
	if (!IsLegel()) {
		ReRotateMatrix();
	}

	UpdateShapeRect(hwnd);

	return;
}

// 判断形状是否在游戏界面中
bool IsLegel() {

	for (int i = 0; i<4; i++)
		for (int j = 0; j<4; j++)
			if (shape[i][j] == 1 && gBoard[shape_row + i][shape_col + j] == 1)
				return false;
	
	return true;
}

// 逆时针旋转当前下落的形状
void RotateMatrix() {

	int (*a)[4] = shape;

	int s = 0;
	
	for (int n = 4; n >= 1; n -= 2) {
		for (int i = 0; i < n - 1; i++) {
			int t = a[s + i][s];
			a[s + i][s] = a[s][s + n - i - 1];
			a[s][s + n - i - 1] = a[s + n - i - 1][s + n - 1];
			a[s + n - i - 1][s + n - 1] = a[s + n - 1][s + i];
			a[s + n - 1][s + i] = t;
		}
		s++;
	}

}

// 如果超出范围，将形状恢复（顺时针旋转）
void ReRotateMatrix() {
	int (*a)[4] = shape;
	int s = 0;
	for (int n = 4; n >= 1; n -= 2) {
		for (int i = 0; i<n - 1; i++) {
			int t = a[s + i][s];
			a[s + i][s] = a[s + n - 1][s + i];
			a[s + n - 1][s + i] = a[s + n - i - 1][s + n - 1];
			a[s + n - i - 1][s + n - 1] = a[s][s + n - i - 1];
			a[s][s + n - i - 1] = t;
		}
		s++;
	}
}

// 下落形状下降一格
void MoveDown(HWND hwnd) {
	
	shape_row++;
	
	if (!IsLegel()) {
		shape_row--;

		if (JudgeLose()) {
			LoseGame(hwnd);
			return;
		}
		FallToGround();
		ClearFullLine();
		UpdateAllBoard(hwnd);

		// 重置下落形状位置
		shape_row = 0;
		shape_col = EXTENDED_COLS / 2 - 2;
		
		RandShape();
	}
	
	UpdateShapeRect(hwnd);
}

// 判断是否输了
bool JudgeLose() {

	if (shape_row == 0)
		return true;
	
	return false;

}

// 游戏结束
void LoseGame(HWND hwnd) {

	SendMessage(hwnd, WM_KEYDOWN, VK_SPACE, 0);

	TCHAR words[100];
	wsprintf(words, TEXT("You lose the Game. Your score is %d. \nDo you want try again?"), score);

	int flag = MessageBox(NULL, words, TEXT("EXIT"), MB_YESNO);

	if (flag == IDYES) {
		SendMessage(hwnd, WM_CREATE, NULL, 0);
		return;
	}
	else if (flag == IDNO) {
		SendMessage(hwnd, WM_DESTROY, NULL, 0);
		return;
	}

}

// 图形落地，更新背景数组
void FallToGround() {
	for (int i = 0; i<4; i++) {
		for (int j = 0; j<4; j++) {
			gBoard[shape_row + i][shape_col + j] = shape[i][j] == 1 ? 1 : gBoard[shape_row + i][shape_col + j];
		}
	}
}

void ClearFullLine() {
	for (int i = shape_row; i <= shape_row + 3; i++) {
		if (i > BOARD_BOTTOM)continue;
		bool there_is_blank = false;

		// 判断一行是否有空格
		for (int j = BOARD_LEFT; j <= BOARD_RIGHT; j++) {
			if (gBoard[i][j] == 0) {
				there_is_blank = true;
				break;
			}
		}
		if (!there_is_blank) {
			AddScore();
			for (int r = i; r >= 1; r--) {
				for (int c = BOARD_LEFT; c <= BOARD_RIGHT; c++) {
					gBoard[r][c] = gBoard[r - 1][c];
				}
			}
		}
	}
}

// 清除一行加100分
void AddScore() {
	score += 100;
}

// 下落形状水平方向移动
void MoveHori(HWND hwnd, int direction) {

	int temp = shape_col;

	// direction 为0则左移否则右移
	if (direction == 0) 
		shape_col--;
	else
		shape_col++;
	
	// 如果移动后位置超出边界
	if (!IsLegel()) {
		shape_col = temp;
	}

	UpdateShapeRect(hwnd);

	return;
}
