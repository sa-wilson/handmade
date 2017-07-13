#include <windows.h>
#include <stdint.h>

struct Win32_Offscreen_Buffer
{
	void *memory;
	int width;
	int height;
	int pitch;
	int bytes_per_pixel;
	BITMAPINFO info;
};

struct Win32_Window_Dimension {
	int width;
	int height;
};

// TODO: This is a global for now
static bool GlobalRunning; 
static Win32_Offscreen_Buffer GlobalBackbuffer;

Win32_Window_Dimension
Win32_GetWindowDimension(HWND window)
{
	Win32_Window_Dimension result;

	RECT client_rect;
	GetClientRect(window, &client_rect);
	result.width = client_rect.right - client_rect.left;
	result.height = client_rect.bottom - client_rect.top;

	return result;
}

static void
RenderWeirdGradient(Win32_Offscreen_Buffer buffer, int blue_offset, int green_offset)
{
	// Lets see what the optomiser does

	uint8_t *row = (uint8_t *)buffer.memory;
	for (int y = 0; y < buffer.height; ++y) {
		uint32_t *pixel = (uint32_t *)row;
		for (int x = 0; x < buffer.width; ++x) {
			uint8_t red = x * y;
			uint8_t green = y * y + green_offset;
			uint8_t blue = x * x + blue_offset;
			*pixel++ = (red << 16) | (green << 8) | blue;
		}
		row += buffer.pitch;
	}
}

static void
Win32_ResizeDIBSection(Win32_Offscreen_Buffer *buffer, int width, int height)
{
	// TODO: Bulletproof this.
	// Maybe don't free first, free after, then free first if that fails.

	if (buffer->memory)
	{
		VirtualFree(buffer->memory, NULL, MEM_RELEASE);
	}

	buffer->width = width;
	buffer->height = height;
	buffer->bytes_per_pixel = 4;
	buffer->pitch = width * buffer->bytes_per_pixel;

	// NOTE: When the biHeight is negative Windows treats the bitmap as top down rather than bottom up
	buffer->info.bmiHeader.biSize = sizeof buffer->info.bmiHeader;
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height;
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;

	size_t bitmap_memory_size = buffer->width * buffer->height * buffer->bytes_per_pixel;
	buffer->memory = VirtualAlloc(NULL, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);

	// TODO: Probably clear this to black
}

static void
Win32_CopyBufferToWindow(HDC context, int window_width, int window_height,
	Win32_Offscreen_Buffer buffer,
	int x, int y, int width, int height)
{
	// TODO: Aspect Ratio correction
	StretchDIBits(context,
		/*
		x, y, width, height,
		x, y, width, height,
		*/
		0, 0, window_width, window_height,
		0, 0, buffer.width, buffer.height,
		buffer.memory,
		&buffer.info,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32_MainWindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
	LRESULT result = 0;

	switch (message) {
		case WM_SIZE: {
		} break;

		case WM_CLOSE: {
			// TODO: Handle this with a message to the user?
			GlobalRunning = false;
		} break;

		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_DESTROY: {
			// TODO: Handle this as an error - recreate window?
			GlobalRunning = false;
		} break;

		case WM_PAINT: {
			PAINTSTRUCT paint;
			HDC context = BeginPaint(window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width =  paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;

			Win32_Window_Dimension dimension = Win32_GetWindowDimension(window);
			Win32_CopyBufferToWindow(context, dimension.width, dimension.height,
				GlobalBackbuffer, x, y, width, height);
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
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code)
{
	WNDCLASS window_class = {};

	Win32_ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	window_class.style = /*CS_OWNDC |*/ CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = Win32_MainWindowCallback;
	window_class.hInstance = instance;
	//	WindowClass.hIcon;
	window_class.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClassA(&window_class)) {
		HWND window = CreateWindowExA(
			0, window_class.lpszClassName, "Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, instance, NULL);
		if (window) {
			int blue_offset = 0;
			int green_offset = 0;

			GlobalRunning = true;
			while (GlobalRunning) {
				MSG message;
				while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
					if (message.message == WM_QUIT) {
						GlobalRunning = false;
					}

					TranslateMessage(&message);
					DispatchMessageA(&message);
				}

				RenderWeirdGradient(GlobalBackbuffer, blue_offset, green_offset);

				HDC context = GetDC(window);

				Win32_Window_Dimension dimension = Win32_GetWindowDimension(window);
				Win32_CopyBufferToWindow(context, dimension.width, dimension.height,
					GlobalBackbuffer, 0, 0, dimension.width, dimension.height);
				ReleaseDC(window, context);

				++blue_offset;
				green_offset += 2;
			}
		} else {
			// TODO: Logging
		}
	} else {
		// TODO: Logging
	}

	return 0;
}