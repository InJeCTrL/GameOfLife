#include<Windows.h>
#include<stdio.h>

//��������
#define MT_ON				1	//�Ƿ������߳�
#define Live				3	//�ھ���ϸ������			
#define Still				2	//�ھ���ϸ������

//��ͼ�ߴ���Ϣ
#define MAP_X				32	//��ͼ����
#define MAP_Y				32	//��ͼ����
#define BLOCK_SIZE			10	//ϸ����Ԫ���С
#define BLOCK_COLOR			(RGB(255,0,0))	//��ϸ����Ԫ�������ɫ
#define thisWidth			(MAP_X * BLOCK_SIZE + MAP_X)//������
#define thisHeight			(MAP_Y * BLOCK_SIZE + MAP_Y)//����߶�

typedef struct PartInfo
{
	INT Y_Begin;//��ʼ��
	INT Y_End;//������
}PartInfo;//ϸ����ͼ��P��Ϣ

INT CellMap[MAP_Y][MAP_X] = { {0} };//ϸ����ͼ
INT tCellMap[MAP_Y][MAP_X] = { {0} };//��ʱϸ����ͼ

HDC hDC;//��ͼDC
INT Initialized = 0;//�ѳ�ʼ��
HBRUSH Brush_Live;//ϸ������ʱ��ɫ
HBRUSH Brush_Death;//ϸ������ʱ��ɫ

//���Ƶ�ͼ��
INT DrawMapLine(HDC hDC)
{
	INT i;
	INT POS_X = 0, POS_Y = 0;//��������

	//����
	for (i = 0; i <= MAP_Y; i++, POS_Y += BLOCK_SIZE + 1)
	{
		MoveToEx(hDC, 0, POS_Y, NULL);
		LineTo(hDC, thisWidth, POS_Y);
	}
	//����
	for (i = 0; i <= MAP_X; i++, POS_X += BLOCK_SIZE + 1)
	{
		MoveToEx(hDC, POS_X, 0, NULL);
		LineTo(hDC, POS_X, thisHeight);
	}

	return 0;
}
//����ϸ����ͼ��ʾ
INT UpdateCellMap(HDC hDC)
{
	INT i, j;
	INT POS_X = 0, POS_Y = 0;//��������
	RECT rct;//���η���
	
	for (i = 0; i < MAP_Y; i++)
	{
		for (j = 0; j < MAP_X; j++)
		{
			rct.left = j * (BLOCK_SIZE + 1) + 1;
			rct.top = i * (BLOCK_SIZE + 1) + 1;
			rct.bottom = rct.top + BLOCK_SIZE;
			rct.right = rct.left + BLOCK_SIZE;
			//����ϸ��
			if (CellMap[i][j])
			{
				FillRect(hDC, &rct, Brush_Live);
			}
			//�����ϸ��
			else
			{
				FillRect(hDC, &rct, Brush_Death);
			}
		}
	}

	return 0;
}
//��ȡϸ���ڳ����еĵ�Ԫ����
POINT GetCellPos(INT mouse_x, INT mouse_y)
{
	POINT tPT;

	tPT.x = mouse_x / (1 + BLOCK_SIZE);
	tPT.y = mouse_y / (1 + BLOCK_SIZE);

	return tPT;
}
//����ϸ����ͼ
INT SetCellMap(INT mouse_x, INT mouse_y)
{
	POINT pointCell = GetCellPos(mouse_x, mouse_y);//��ȡ���ָ���ϸ��λ��

	CellMap[pointCell.y][pointCell.x] ^= 1;//ϸ��״̬ȡ��

	return 0;
}
//����ϸ����ͼ
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
					if (ti < 0 || tj < 0 || ti >= MAP_Y || tj >= MAP_X || //��Χ����˸��ھ�
						(ti == i && tj == j))//�����غ�
						continue;
					s += CellMap[ti][tj];
				}
			}
			if (s == Live)
				tCellMap[i][j] = 1;//ϸ������
			else if (s != Still)
				tCellMap[i][j] = 0;//ϸ������
			s = 0;
		}
	}

	return 0;
}
//�����߳�
DWORD WINAPI MT_DoCalc(LPVOID lpParam)
{
	PartInfo PTI = *(PartInfo*)lpParam;

	DoCalcCellMap(PTI.Y_Begin, PTI.Y_End);

	return 0;
}
//Ԥ����ϸ����ͼ
INT CalcCellMap(void)
{
	SYSTEM_INFO si;
	HANDLE hThread[128] = { 0 };//�߳̾���б�
	INT numThread;//Ԥ�����߳���
	INT i = 0;
	PartInfo PtI;//ϸ����ͼ��P��Ϣ

	memcpy(tCellMap, CellMap, MAP_X * MAP_Y * sizeof(INT));
	//������߳�
	if (MT_ON)
	{
		GetSystemInfo(&si);
		numThread = si.dwNumberOfProcessors;//��ȡCPU������
		PtI.Y_End = -1;
		while (i < numThread)
		{
			PtI.Y_Begin = PtI.Y_End + 1;
			if (i == numThread - 1)
				PtI.Y_End = MAP_Y - 1;
			else
				PtI.Y_End += MAP_Y / numThread - 1;
			hThread[i++] = CreateThread(NULL, 0, MT_DoCalc, (LPVOID)(&PtI), 0, NULL);
		}
		WaitForMultipleObjects(numThread, hThread, TRUE, INFINITE);//�ȴ��������߳��˳�
		i--;
		while (i >= 0)
		{
			CloseHandle(hThread[i--]);
		}
	}
	//���߳�
	else
	{
		DoCalcCellMap(0,MAP_Y - 1);//ȫͼ����
	}
	memcpy(CellMap, tCellMap, MAP_X * MAP_Y * sizeof(INT));

	return 0;
}
//����ص�
LRESULT WINAPI CtlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT i;

	switch (message)
	{
	//���ߣ���ʼ����������
	case WM_PAINT:
	{
		if (!Initialized)
		{
			DrawMapLine(hDC);
			Initialized = 1;
		}
		UpdateCellMap(hDC);
		break;
	}
	//����ϸ��������
	case WM_LBUTTONDOWN:
	{
		if (LOWORD(lParam) < thisWidth && HIWORD(lParam) < thisHeight)
		{
			SetCellMap(LOWORD(lParam), HIWORD(lParam));
			SendMessage(hWnd, WM_PAINT, 0, 0);
		}
		break;
	}
	//ϸ���ݻ���ǰ�ƽ�һ��
	case WM_RBUTTONDOWN:
	{
		CalcCellMap();
		SendMessage(hWnd, WM_PAINT, 0, 0);
		break;
	}
	//��������
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
	WNDCLASSEX WC;//������
	HWND hwnd;//������
	INT ScreenWidth, ScreenHeight;//��Ļ��ȡ��߶�
	

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
	hwnd = CreateWindow(L"WND", L"������Ϸ", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, (ScreenWidth - thisWidth) / 2, (ScreenHeight - thisHeight) / 2, thisWidth + 2 * BLOCK_SIZE, thisHeight + 4 * BLOCK_SIZE, NULL, 0, 0, 0);
	hDC = GetDC(hwnd);//��ȡ��ͼDC
	Brush_Live = CreateSolidBrush(BLOCK_COLOR);
	Brush_Death = CreateSolidBrush(RGB(255, 255, 255));
	ShowWindow(hwnd, 1);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	ReleaseDC(NULL, Brush_Live);
	ReleaseDC(NULL, Brush_Death);
	ReleaseDC(hwnd, hDC);

	return 0;
}