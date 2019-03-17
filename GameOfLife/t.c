#include<Windows.h>
#include<stdio.h>

//其它配置
#define MT_ON				1	//是否开启多线程
#define Live				3	//邻居数细胞生存			
#define Still				2	//邻居数细胞不变
#define WM_CalcBegin		(WM_USER + 666)	//线程计算消息
#define WM_ThDestroy		(WM_USER + 111)	//线程终止消息

//地图尺寸信息
#define MAP_X				50	//地图列数
#define MAP_Y				50	//地图行数
#define BLOCK_SIZE			10	//细胞单元格大小
#define BLOCK_COLOR			(RGB(255,0,0))	//活细胞单元格填充颜色
#define BLOCK_DCOLOR		(RGB(0, 255, 255))	//死细胞单元格填充颜色
#define thisWidth			(MAP_X * BLOCK_SIZE + MAP_X)//程序宽度
#define thisHeight			(MAP_Y * BLOCK_SIZE + MAP_Y)//程序高度

typedef struct PartInfo
{
	INT nPart;//Part编号
	INT Y_Begin;//起始行
	INT Y_End;//结束行
}PartInfo;//细胞地图分P信息

INT lastCellMap[MAP_Y][MAP_X] = { {0} };//上一次细胞地图
INT CellMap[MAP_Y][MAP_X] = { {0} };//细胞地图
INT tCellMap[MAP_Y][MAP_X] = { {0} };//临时细胞地图

HDC hDC;//绘图DC
INT Initialized = 0;//已初始化
HBRUSH Brush_Live;//细胞生存时颜色
HBRUSH Brush_Death;//细胞死亡时颜色
HANDLE hThread[128] = { 0 };//线程句柄列表
DWORD tID[128] = { 0 };//线程ID
INT numThread = 0;//预启用线程数
INT ThreadCalcComp[128] = { 0 };//标志线程计算结束

//绘制地图线
INT DrawMapLine(HDC hDC)
{
	INT i;
	INT POS_X = 0, POS_Y = 0;//横纵坐标

	//横线
	for (i = 0; i <= MAP_Y; i++, POS_Y += BLOCK_SIZE + 1)
	{
		MoveToEx(hDC, 0, POS_Y, NULL);
		LineTo(hDC, thisWidth, POS_Y);
	}
	//竖线
	for (i = 0; i <= MAP_X; i++, POS_X += BLOCK_SIZE + 1)
	{
		MoveToEx(hDC, POS_X, 0, NULL);
		LineTo(hDC, POS_X, thisHeight);
	}

	return 0;
}
//更新细胞地图显示
INT UpdateCellMap(HDC hDC)
{
	INT i, j;
	INT POS_X = 0, POS_Y = 0;//横纵坐标
	RECT rct;//矩形方块
	
	for (i = 0; i < MAP_Y; i++)
	{
		for (j = 0; j < MAP_X; j++)
		{
			rct.left = j * (BLOCK_SIZE + 1) + 1;
			rct.top = i * (BLOCK_SIZE + 1) + 1;
			rct.bottom = rct.top + BLOCK_SIZE;
			rct.right = rct.left + BLOCK_SIZE;
			//初始化时绘制背景 或 细胞状态发生变化再重画
			if (!Initialized || (lastCellMap[i][j] != CellMap[i][j]))
			{
				//填充活细胞
				if (CellMap[i][j])
				{
					FillRect(hDC, &rct, Brush_Live);
				}
				//填充死细胞
				else
				{
					FillRect(hDC, &rct, Brush_Death);
				}
			}
		}
	}
	memcpy(lastCellMap, CellMap, MAP_X * MAP_Y * sizeof(INT));

	return 0;
}
//获取细胞在程序中的单元坐标
POINT GetCellPos(INT mouse_x, INT mouse_y)
{
	POINT tPT;

	tPT.x = mouse_x / (1 + BLOCK_SIZE);
	tPT.y = mouse_y / (1 + BLOCK_SIZE);

	return tPT;
}
//设置细胞地图
INT SetCellMap(INT mouse_x, INT mouse_y)
{
	POINT pointCell = GetCellPos(mouse_x, mouse_y);//获取鼠标指向的细胞位置

	CellMap[pointCell.y][pointCell.x] ^= 1;//细胞状态取反

	return 0;
}
//计算细胞地图
INT DoCalcCellMap(INT Y_Begin, INT Y_End)
{
	INT i, j, ti, tj, s = 0;

	for (i = Y_Begin; i <= Y_End; i++)
	{
		for (j = 0; j < MAP_X; j++)
		{
			for (ti = i - 1; ti < i + 2; ti++)
			{
				for (tj = j - 1; tj < j + 2; tj++)
				{
					if (ti < 0 || tj < 0 || ti >= MAP_Y || tj >= MAP_X || //周围不足八个邻居
						(ti == i && tj == j))//样点重合
						continue;
					s += CellMap[ti][tj];
				}
			}
			if (s == Live)
				tCellMap[i][j] = 1;//细胞生存
			else if (s != Still)
				tCellMap[i][j] = 0;//细胞死亡
			s = 0;
		}
	}

	return 0;
}
//计算线程
DWORD WINAPI MT_Do(LPVOID lpParam)
{
	PartInfo PTI = *(PartInfo*)lpParam;//保存计算、绘图范围
	MSG msg;

	//消息循环等待计算消息
	while (TRUE)
	{
		if (GetMessage(&msg, NULL, 0, 0))
		{
			switch (msg.message)
			{
			//收到退出消息
			case WM_ThDestroy:
				return 0;
			//收到开始计算消息
			case WM_CalcBegin:
				DoCalcCellMap(PTI.Y_Begin, PTI.Y_End);
				ThreadCalcComp[PTI.nPart] = 1;//标记本线程已完成计算
				break;
			default:
				break;
			}
		}
	}
}
//预计算细胞地图
INT CalcCellMap(void)
{
	INT i = 0;

	memcpy(tCellMap, CellMap, MAP_X * MAP_Y * sizeof(INT));
	//允许多线程
	if (MT_ON)
	{
		//向各计算线程发送计算消息
		for (i = 0; i < numThread; i++)
		{
			PostThreadMessage(tID[i], WM_CalcBegin, 0, 0);
		}
		//等待所有计算线程完成计算
		for (i = 0; i < numThread; i++)
		{
			//有未完成的线程，继续等待
			if (!ThreadCalcComp[i])
				i = -1;
		}
		memset(ThreadCalcComp, 0, numThread * sizeof(INT));//重置计算完成标志
	}
	//单线程
	else
	{
		DoCalcCellMap(0,MAP_Y - 1);//全图计算
	}
	memcpy(CellMap, tCellMap, MAP_X * MAP_Y * sizeof(INT));

	return 0;
}
//窗体回调
LRESULT WINAPI CtlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT i;

	switch (message)
	{
	//画线，初始化生命棋盘
	case WM_PAINT:
	{
		UpdateCellMap(hDC);
		if (!Initialized)
		{
			DrawMapLine(hDC);
			Initialized = 1;
		}
		break;
	}
	//设置细胞生或死
	case WM_LBUTTONDOWN:
	{
		if (LOWORD(lParam) < thisWidth && HIWORD(lParam) < thisHeight)
		{
			SetCellMap(LOWORD(lParam), HIWORD(lParam));
			SendMessage(hWnd, WM_PAINT, 0, 0);
		}
		break;
	}
	//细胞演化向前推进一次
	case WM_RBUTTONDOWN:
	{
		CalcCellMap();
		SendMessage(hWnd, WM_PAINT, 0, 0);
		break;
	}
	//窗体销毁
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	default:
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASSEX WC;//窗体类
	HWND hwnd;//主窗体
	INT ScreenWidth, ScreenHeight;//屏幕宽度、高度
	SYSTEM_INFO si;
	INT i = 0;
	PartInfo PtI[128] = { {0, 0} };//细胞地图分P信息

	WC.cbSize = sizeof(WNDCLASSEX);
	WC.style = CS_HREDRAW | CS_VREDRAW;
	WC.lpfnWndProc = CtlProc;
	WC.cbClsExtra = 0;
	WC.cbWndExtra = 0;
	WC.hInstance = hInstance;
	WC.hIcon = 0;
	WC.hCursor = 0;
	WC.hbrBackground = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
	WC.lpszMenuName = 0;
	WC.lpszClassName = L"WND";
	WC.hIconSm = 0;

	ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	RegisterClassEx(&WC);
	hwnd = CreateWindow(L"WND", L"生命游戏", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, (ScreenWidth - thisWidth) / 2, (ScreenHeight - thisHeight) / 2, thisWidth + 5 * BLOCK_SIZE, thisHeight + 5 * BLOCK_SIZE, NULL, 0, 0, 0);
	hDC = GetDC(hwnd);//获取绘图DC
	Brush_Live = CreateSolidBrush(BLOCK_COLOR);
	Brush_Death = CreateSolidBrush(BLOCK_DCOLOR);
	//允许多线程，创建计算线程
#if (MT_ON == 1)
	GetSystemInfo(&si);
	numThread = si.dwNumberOfProcessors;//获取CPU核心数
	while (i < numThread)
	{
		if (!i)
			PtI[i].Y_Begin = 0;
		else
			PtI[i].Y_Begin = PtI[i - 1].Y_End + 1;
		if (i == numThread - 1)
			PtI[i].Y_End = MAP_Y - 1;
		else
			PtI[i].Y_End = PtI[i].Y_Begin + MAP_Y / numThread - 1;
		PtI[i].nPart = i;
		hThread[i++] = CreateThread(NULL, 0, MT_Do, (LPVOID)(&(PtI[i])), 0, &tID[i]);
	}
#endif
	ShowWindow(hwnd, 1);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	ReleaseDC(hwnd, Brush_Live);	//释放细胞存活画刷
	ReleaseDC(hwnd, Brush_Death);	//释放细胞死亡画刷
	ReleaseDC(hwnd, hDC);			//释放绘图DC
#if (MT_ON == 1)
	//释放线程句柄
	for (i = 0; i < numThread; i++)
	{
		PostThreadMessage(tID[i], WM_ThDestroy, 0, 0);//发送退出线程消息
		WaitForSingleObject(hThread[i], INFINITE);//等待计算线程退出
		CloseHandle(hThread[i]);
	}
#endif

	return 0;
}