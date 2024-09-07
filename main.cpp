// clang-format off
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <algorithm>
// clang-format on

#define WM_TRAYICON (WM_USER + 1) // 自定义消息，用于托盘图标处理
#define IDM_EXIT 1001             // 定义退出菜单项的命令 ID

const int HOTKEY_ID = 1; // ID for the hotkey (WIN+F)
const int ALPHA = 128;   // Alpha value for the gray overlay

NOTIFYICONDATA nid;
HWND hOverlay = NULL;
RECT overlayRect, overlayFullRect;
RECT subRect[9] = {};

const int border = 1;
const char areaName[10] = {"UIOJKLM<>"};
static void SubdivideRECTInto3x3(const RECT &rect) {
  long totalWidth = rect.right - rect.left;
  long totalHeight = rect.bottom - rect.top;

  // Calculate width and height of each cell including border gaps
  long cellWidth = (totalWidth - 2 * border) / 3;
  long cellHeight = (totalHeight - 2 * border) / 3;

  // Generate 3x3 sub-RECTs
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      RECT &sub = subRect[i * 3 + j];
      sub.left = rect.left + j * (cellWidth + border);
      sub.top = rect.top + i * (cellHeight + border);
      sub.right = sub.left + cellWidth;
      sub.bottom = sub.top + cellHeight;
    }
  }
}

long calRectAear(const RECT &rect) {
  return std::max(
      1l, std::abs((rect.bottom - rect.top) * (rect.right - rect.left)));
}

static void DrawSubAear(HWND hwnd, RECT fullRect) {
  HDC hdc = GetDC(hOverlay);
  // draw background
  HBRUSH brushBckGround = CreateSolidBrush(RGB(0, 0, 0));
  FillRect(hdc, &overlayFullRect, brushBckGround);
  DeleteObject(brushBckGround);

  // draw front
  HBRUSH brushFG = CreateSolidBrush(RGB(128, 128, 128));
  for (int i = 0; i < 9; i++) {
    FillRect(hdc, &subRect[i], brushFG);

    long aear = calRectAear(subRect[i]);
    if (aear > 400) {
      PAINTSTRUCT ps;
      // 定义一个矩形
      RECT rect = subRect[i];
      // // 画出矩形，仅为了可视化
      // FrameRect(hdc, &rect, (HBRUSH)GetStockObject(RED_BRUSH));
      // 计算中心点
      int centerX = (rect.left + rect.right) / 2;
      int centerY = (rect.top + rect.bottom) / 2;
      // 字符以及其大小

      char ch = (areaName[i]); // 你想要绘制的字符
      TCHAR tch;
#ifdef UNICODE
      MultiByteToWideChar(CP_ACP, 0, &ch, 1, &tch, 1);
#else
      tch = ch;
#endif
      int frontHeight =
          static_cast<int>(std::max(20., (rect.bottom - rect.top) * 0.08));

      HFONT hFont =
          CreateFont(frontHeight, // 字符高度（像素）
                     0,           // 字符宽度（0表示自适应高度）
                     0,           // 文本倾斜度（0表示不倾斜）
                     0,           // 字符间角度（0表示水平）
                     FW_NORMAL, // 字体加粗 (FW_NORMAL, FW_BOLD, FW_HEAVY, etc.)
                     FALSE,     // 是否启用斜体
                     FALSE,     // 是否启用下划线
                     FALSE,     // 是否启用删除线
                     DEFAULT_CHARSET,          // 字符集
                     OUT_DEFAULT_PRECIS,       // 输出精度
                     CLIP_DEFAULT_PRECIS,      // 裁剪精度
                     DEFAULT_QUALITY,          // 输出质量
                     DEFAULT_PITCH | FF_SWISS, // 字体擘显
                     _T("Arial")               // 字体名称
          );
      HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

      // 设置文本输出位置
      SetTextAlign(hdc, TA_CENTER | TA_BASELINE);
      TextOut(hdc, centerX, centerY, &tch, 1);

      SelectObject(hdc, oldFont);
      DeleteObject(hFont);
      EndPaint(hOverlay, &ps);
    }
  }
  DeleteObject(brushFG);
  ReleaseDC(hOverlay, hdc);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_CREATE:
    // 初始化托盘图标
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"Overlay Application");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // 注册全局热键（WIN+ SHIFT+F）

    if (!RegisterHotKey(hwnd, HOTKEY_ID, MOD_CONTROL, 'F')) {
      MessageBox(hwnd, L"Hotkey registration failed!", L"Error",
                 MB_OK | MB_ICONERROR);
    }
    break;

  case WM_HOTKEY:
    if (wParam == HOTKEY_ID) {
      if (hOverlay == NULL) {
        // 创建全屏灰白色图层
        overlayRect.left = 1;
        overlayRect.top = 1;
        overlayRect.right = GetSystemMetrics(SM_CXSCREEN) - 1;
        overlayRect.bottom = GetSystemMetrics(SM_CYSCREEN) - 1;
        overlayFullRect = overlayRect;
        SubdivideRECTInto3x3(overlayRect);

        hOverlay =
            CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT, L"STATIC", NULL,
                            WS_POPUP | WS_VISIBLE, 0, 0, overlayRect.right,
                            overlayRect.bottom, hwnd, NULL, NULL, NULL);

        SetLayeredWindowAttributes(hOverlay, RGB(0, 0, 0), ALPHA, LWA_ALPHA);
        SetForegroundWindow(hOverlay); // 使图层接收键盘输入

        UpdateWindow(hOverlay);
        DrawSubAear(hOverlay, overlayFullRect);
        UpdateWindow(hOverlay);

      } else {
        DestroyWindow(hOverlay);
        hOverlay = NULL;
      }
    }
    break;

  case WM_TRAYICON:
    if (lParam == WM_RBUTTONUP) {
      // 处理右键点击托盘图标
      POINT pt;
      GetCursorPos(&pt);
      HMENU hMenu = CreatePopupMenu();
      AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");
      SetForegroundWindow(hwnd);
      TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0,
                     hwnd, NULL);
      DestroyMenu(hMenu);
    }
    break;

  case WM_COMMAND:
    if (LOWORD(wParam) == IDM_EXIT) {
      PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    break;

  case WM_DESTROY:
    UnregisterHotKey(hwnd, HOTKEY_ID);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

void HandleKeyDown(HWND hwnd, int key) {

  if (hOverlay != NULL) {

    switch (key) {
    case 'U': // 左侧两块
      overlayRect = subRect[0];
      break;
    case 'I': // 右侧两块
      overlayRect = subRect[1];
      break;
    case 'O': // 下侧两块
      overlayRect = subRect[2];
      break;
    case 'J': // 左侧两块
      overlayRect = subRect[3];
      break;
    case 'K': // 右侧两块
      overlayRect = subRect[4];
      break;
    case 'L': // 下侧两块
      overlayRect = subRect[5];
      break;
    case 'M': // 左侧两块
      overlayRect = subRect[6];
      break;
    case 0xBC: // 右侧两块
      overlayRect = subRect[7];
      break;
    case 0xBE: // 下侧两块
      overlayRect = subRect[8];
      break;
    }
    long aear = calRectAear(overlayRect);
    if (aear < 400) {

      bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
      bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;

      int x = (overlayRect.left + overlayRect.right) / 2;
      int y = (overlayRect.top + overlayRect.bottom) / 2;
      SetCursorPos(x, y);
      // 模拟鼠标按下和松开
      if (shiftPressed) {
        mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
      } else if (ctrlPressed) {
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        Sleep(50);
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
      } else {
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
      }
      DestroyWindow(hOverlay);
      hOverlay = NULL;
    } else {
      SubdivideRECTInto3x3(overlayRect);
      DrawSubAear(hOverlay, overlayFullRect);
    }
  }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PWSTR pCmdLine, int nCmdShow) {
  const wchar_t CLASS_NAME[] = L"OverlayWindowClass";

  WNDCLASSW wc = {};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;

  RegisterClassW(&wc);

  HWND hwnd = CreateWindowExW(0, CLASS_NAME,
                              NULL, // 创建一个隐藏的主窗口
                              0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

  if (hwnd == NULL) {
    return 0;
  }

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (msg.message == WM_KEYDOWN) {
      if (msg.wParam == VK_ESCAPE) {
        DestroyWindow(hOverlay);
        hOverlay = NULL;
      } else if (msg.wParam == 'U' || msg.wParam == 'I' || msg.wParam == 'O' ||
                 msg.wParam == 'J' || msg.wParam == 'K' || msg.wParam == 'L' ||
                 msg.wParam == 'M' || msg.wParam == 0xbc || msg.wParam == 0xbe

      ) {
        HandleKeyDown(hwnd, msg.wParam);
      }
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}
