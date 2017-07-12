#include <windows.h>
#include <stdint.h>

// TODO: This is a global for now
static bool Running; 

static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static size_t BytesPerPixel = 4;

static void
RenderWeirdGradient(int x_offset, int y_offset)
{
	size_t pitch = BitmapWidth * BytesPerPixel;
	uint8_t *row = (uint8_t *)BitmapMemory;
	for (int y = 0; y < BitmapHeight; ++y) {
		uint32_t *pixel = (uint32_t *)row;
		for (int x = 0; x < BitmapWidth; ++x) {
			uint8_t red = 0;
			uint8_t green = y + y_offset;
			uint8_t blue = x + x_offset;
			*pixel++ = (red << 16) |(green << 8) | blue;
		}
		row += pitch;
	}
}

static void
Win32_ResizeDIBSection(int width, int height)
{
	// TODO: Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if (BitmapMemory)
	{
		VirtualFree(BitmapMemory, NULL, MEM_RELEASE);
	}

	BitmapWidth = width;
	BitmapHeight = height;

	BitmapInfo.bmiHeader.biSize = sizeof BitmapInfo.bmiHeader;
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	size_t bitmap_memory_size = BitmapWidth * BitmapHeight * BytesPerPixel;
	BitmapMemory = VirtualAlloc(NULL, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

	// TODO: Probably clear this to black
}

static void
Win32_UpdateWindow(HDC context, RECT *client_rect, int x, int y, int width, int height)
{
	int window_width = client_rect->right - client_rect->left;
	int window_height = client_rect->bottom - client_rect->top;

	StretchDIBits(context,
		/*
		x, y, width, height,
		x, y, width, height,
		*/
		0, 0, BitmapWidth, BitmapHeight,
		0, 0, window_width, window_height,
		BitmapMemory,
		&BitmapInfo,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32_MainWindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	LRESULT result = 0;

	switch (message) {
		case WM_SIZE: {
			RECT client_rect;
			GetClientRect(window, &client_rect);
			int width = client_rect.right - client_rect.left;
			int height = client_rect.bottom - client_rect.top;
			Win32_ResizeDIBSection(width, height);
		} break;

		case WM_CLOSE: {
			// TODO: Handle this with a message to the user?
			Running = false;
		} break;

		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_DESTROY: {
			// TODO: Handle this as an error - recreate window?
			Running = false;
		} break;

		case WM_PAINT: {
			PAINTSTRUCT paint;
			HDC context = BeginPaint(window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width =  paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;

			RECT client_rect;
			GetClientRect(window, &client_rect);

			Win32_UpdateWindow(context, &client_rect, x, y, width, height);
			EndPaint(window, &paint);
		} break;

		default: {
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
			int x_offset = 0;
			int y_offset = 0;

			Running = true;
			while (Running) {
				MSG message;
				while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
					if (message.message == WM_QUIT) {
						Running = false;
					}

					TranslateMessage(&message);
					DispatchMessageA(&message);
				}

				RenderWeirdGradient(x_offset, y_offset);

				HDC context = GetDC(window_handle);
				RECT client_rect;
				GetClientRect(window_handle, &client_rect);
				// Casey got the width here, but we're actually ignoring it
				// So, whatever
				Win32_UpdateWindow(context, &client_rect, 0, 0, 0, 0);
				ReleaseDC(window_handle, context);

				++x_offset;
			}
		} else {
			// TODO: Logging
		}
	} else {
		// TODO: Logging
	}

	return 0;
}