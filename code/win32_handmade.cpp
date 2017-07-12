#include <windows.h>

// TODO: This is a global for now
static bool Running; 
static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static HBITMAP BitmapHandle;
static HDC BitmapDeviceContext;

static void
Win32_ResizeDIBSection(int width, int height)
{
	// TODO: Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if (BitmapHandle) {
		DeleteObject(BitmapHandle);
	} 

	if (!BitmapDeviceContext) {
		// TODO: Should we recreate these under certain special circumstances?
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	BitmapInfo.bmiHeader.biSize = sizeof BitmapInfo.bmiHeader;
	BitmapInfo.bmiHeader.biWidth = width;
	BitmapInfo.bmiHeader.biHeight = height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	BitmapHandle = CreateDIBSection(
		BitmapDeviceContext, &BitmapInfo,
		DIB_RGB_COLORS, &BitmapMemory,
		NULL, NULL);
}

static void
Win32_UpdateWindow(HDC context, int x, int y, int width, int height)
{
	StretchDIBits(context,
		x, y, width, height,
		x, y, width, height,
		BitmapMemory,
		&BitmapInfo,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32_MainWindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	LRESULT result = 0;

	switch (message) {
		case WM_SIZE:
		{
			RECT client_rect;
			GetClientRect(window, &client_rect);
			int width = client_rect.right - client_rect.left;
			int height = client_rect.bottom - client_rect.top;
			Win32_ResizeDIBSection(width, height);
		} break;

		case WM_CLOSE:
		{
			// TODO: Handle this with a message to the user?
			Running = false;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_DESTROY:
		{
			// TODO: Handle this as an error - recreate window?
			Running = false;
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC context = BeginPaint(window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width =  paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			Win32_UpdateWindow(context, x, y, width, height);
			EndPaint(window, &paint);
		} break;

		default:
		{
//			OutputDebugStringA("default\n");
			result = DefWindowProc(window, message, w_param, l_param);
		} break;
	}

	return result;
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line,
	int show_code)
{
	WNDCLASS window_class = {};

	// TODO: Check if HREDRAW, VREDRAW, OWNDC still matter
	window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = Win32_MainWindowCallback;
	window_class.hInstance = instance;
	//	WindowClass.hIcon;
	window_class.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&window_class)) {
		HWND window_handle = CreateWindowExA(
			0, window_class.lpszClassName, "Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, instance, NULL);
		if (window_handle) {
			Running = true;
			while (Running) {
				MSG message;
				BOOL message_result = GetMessageA(&message, NULL, 0, 0);
				if (message_result > 0) {
					TranslateMessage(&message);
					DispatchMessageA(&message);
				} else {
					break;
				}
			}
		} else {
			// TODO: Logging
		}
	} else {
		// TODO: Logging
	}

	return 0;
}