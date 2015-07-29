#include <windows.h>
#include <stdint.h>


#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;


struct win32_offscreen_buffer{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int bytesPerPixel = 4;
};

struct win32_windowDimensions{
	int Width;
	int Height;
};

win32_windowDimensions 
Win32GetWindowDimensions(HWND Window){

	win32_windowDimensions Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return(Result);
}


// TODO global for now
global_variable bool Running = true;
global_variable win32_offscreen_buffer GlobalBackbuffer;
 
internal void
RenderWierdGradient(win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset){

	int Width = Buffer.Width;
	int Height = Buffer.Height;

	uint8* Row = (uint8*)Buffer.Memory;
	int Pitch = Width*Buffer.bytesPerPixel;

	for (int Y = 0; Y < Buffer.Height; ++Y){

		uint8* Pixel = (uint8*)Row;

		for (int X = 0; X < Buffer.Width; ++X){

			//colors defined as B-G-R-(pad)

			*Pixel = (uint8)(X + BlueOffset) ;
			++Pixel;

			*Pixel = (uint8)(Y + GreenOffset);
			++Pixel;

			*Pixel = 0;
			++Pixel;

			*Pixel = 0;
			++Pixel;
		}
		Row += Pitch;
	}

}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height){

	if (Buffer->Memory){
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	
	// see global above; BITMAPINFO BitmapInfo;

	Buffer->Width = Width;
	Buffer->Height = Height;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height; //biHeight starts from bottom, so Height must be flipped
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	//Buffer->bytesPerPixel = 4;
	int BitmapMemorySize = Buffer->bytesPerPixel * (Buffer->Width*Buffer->Height);
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer Buffer, int X, int Y, int Width, int Height){
	
	StretchDIBits(DeviceContext, 
		
		0, 0, WindowWidth, WindowHeight,	
		0, 0, Buffer.Width, Buffer.Height,	
		Buffer.Memory,
		&Buffer.Info,
		DIB_RGB_COLORS, SRCCOPY);

}

LRESULT CALLBACK 
Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam){
	
	LRESULT Result = 0;
	
	switch (Message){
		case WM_SIZE:
		{
			/*win32_windowDimensions Dimensions = Win32GetWindowDimensions(Window);
			Win32ResizeDIBSection(&GlobalBackbuffer, Dimensions.Width, Dimensions.Height);*/
			OutputDebugStringA("WM_SIZE \n");
		}break;
		case WM_DESTROY:
		{
			Running = false;
			OutputDebugStringA("WM_DESTROY \n");
		}break;
		case WM_CLOSE:
		{
			Running = false;
		}break;
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATE \n");
		}break;
		case WM_PAINT:
		{
			//watch lectures by Chandler Carruth
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			
			win32_windowDimensions Dimensions = Win32GetWindowDimensions(Window);
			
			Win32DisplayBufferInWindow(DeviceContext, Dimensions.Width, Dimensions.Height, GlobalBackbuffer, X, Y, Width, Height);
			//local_persist DWORD Operation = WHITENESS;
			PatBlt(DeviceContext, X, Y, Width, Height, PATCOPY);
			//PatBlt(DeviceContext, X, Y, Width, Height, Operation);
			/*if (Operation == WHITENESS)
			{
				Operation = BLACKNESS;
			}
			else
			{
				Operation = WHITENESS;
			}*/

			EndPaint(Window, &Paint);
		}break;
		default:
		{
			OutputDebugStringA("Default \n"); 
			Result = DefWindowProc(Window, Message, WParam, LParam);
		}break;
	}
	return(Result);
}
//---------------------------------------------------------------------------------------------------------<>Entry point<>
void 
main(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode){

	WNDCLASS WindowClass = {};
		
		//win32_windowDimensions Dimensions = Win32GetWindowDimensions(Window);
		Win32ResizeDIBSection(&GlobalBackbuffer, 1288, 720);

		WindowClass.style = CS_HREDRAW|CS_VREDRAW; // these tel the window to redraw the entire image when window is resized
		WindowClass.lpfnWndProc =Win32MainWindowCallback;
//		WindowClass.cbClsExtra = ;
//		WindowClass.cbWndExtra = ;
		WindowClass.hInstance = Instance;
//		WindowClass.hIcon;
//		WindowClass.hCursor;
//		WindowClass.hbrBackground;
//		WindowClass.lpszMenuName;
		WindowClass.lpszClassName = (LPCWSTR)L"HandmadeHeroWindowClass";
	
		if (RegisterClass(&WindowClass)){
			HWND Window = CreateWindowEx(
				0,
				WindowClass.lpszClassName,
				(LPCWSTR)L"Handmade Hero",
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				0,
				0,
				Instance,
				0);
			if (Window){
				Running = true;

				int Xoffset = 0;
				int Yoffset = 0;

				while (Running){

					MSG Message;
					while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){

						if (Message.message == WM_QUIT){
							Running = false;
						}

						TranslateMessage(&Message);
						DispatchMessage(&Message);
					}
					
					RenderWierdGradient(GlobalBackbuffer, Xoffset, Yoffset);
					HDC DeviceContext = GetDC(Window);
					win32_windowDimensions Dimensions = Win32GetWindowDimensions(Window);
					Win32DisplayBufferInWindow(DeviceContext, Dimensions.Width, Dimensions.Height, GlobalBackbuffer, 0, 0, Dimensions.Width, Dimensions.Height);
					ReleaseDC(Window, DeviceContext);

					++Xoffset;
				}
			}
			else{
			//nothing here
			}
		}
		else{
			//nothing here
		}

	MessageBox(0, (LPCWSTR)L"This is Handmade Hero", (LPCWSTR)L"Handmade Hero", MB_OK | MB_ICONINFORMATION);
}	

