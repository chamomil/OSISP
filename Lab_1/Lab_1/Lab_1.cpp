// Lab_1.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "Lab_1.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB1));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LAB1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hEdit; // handle for the edit
    static OPENFILENAME ofn; // structure for the file dialog

    switch (message)
    {
    case WM_CREATE:
    {
        hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"Enter text right here ", WS_CHILD | WS_HSCROLL | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
            0, 0, 1100, 500, hWnd, NULL, hInst, NULL);
        if (hEdit == NULL)
        {
            MessageBox(hWnd, L"Edit field failed to create", L"Error", MB_ICONERROR);
            return -1;
        }
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_OPEN:
            {
                WCHAR szFileName[MAX_PATH] = L"";

                ZeroMemory(&ofn, sizeof(OPENFILENAME));
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = szFileName;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

                if (GetOpenFileName(&ofn))
                {
                    HANDLE hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        DWORD dwFileSize = GetFileSize(hFile, NULL);
                        if (dwFileSize != INVALID_FILE_SIZE) 
                        {
                            LPSTR lpFileText = (LPSTR)GlobalAlloc(GPTR, (dwFileSize + 1) * sizeof(WCHAR)); //+1 veans extra byte for null termination
                            if (lpFileText != NULL)
                            {
                                DWORD dwRead;
                                if (ReadFile(hFile, lpFileText, dwFileSize, &dwRead, NULL))
                                {
                                    lpFileText[dwFileSize] = '\0'; // we add null to pass it to the other parts as a string

                                    int wideCharCount = MultiByteToWideChar(CP_UTF8, 0, lpFileText, -1, NULL, 0); //recalculating buffer size to change encoding
                                    if (wideCharCount > 0)
                                    {
                                        LPWSTR lpWideText = (LPWSTR)GlobalAlloc(GPTR, wideCharCount * sizeof(WCHAR));
                                        if (lpWideText != NULL)
                                        {
                                            MultiByteToWideChar(CP_UTF8, 0, lpFileText, -1, lpWideText, wideCharCount);
                                            SetWindowTextW(hEdit, lpWideText); 
                                            GlobalFree(lpWideText);
                                        }
                                    }
                                }
                                else
                                {
                                    MessageBox(hWnd, L"Failed to read the file", L"Error", MB_ICONERROR);
                                }
                                GlobalFree(lpFileText);
                            }
                            else
                            {
                                MessageBox(hWnd, L"Failed to allocate memory", L"Error", MB_ICONERROR);
                            }
                        }
                        CloseHandle(hFile);
                    }
                    else
                    {
                        MessageBox(hWnd, L"Failed to open the file", L"Error", MB_ICONERROR);
                    }
                }
            }
            break;
            case IDM_SAVE:
            {
                // Open a file dialog to select or create a text file for writing
                WCHAR szFileName[MAX_PATH] = L"";
                ZeroMemory(&ofn, sizeof(OPENFILENAME));
                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = hWnd;
                ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile = szFileName;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_OVERWRITEPROMPT;

                if (GetSaveFileName(&ofn))
                {
                    // Get the text from the Edit control and write it to the selected file
                    HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        int len = GetWindowTextLength(hEdit);
                        LPWSTR lpFileText = (LPWSTR)GlobalAlloc(GPTR, (len + 1) * sizeof(WCHAR));
                        if (lpFileText != NULL)
                        {
                            GetWindowText(hEdit, lpFileText, len + 1);
                            DWORD dwWritten;
                            int utf8Len = WideCharToMultiByte(CP_UTF8, 0, lpFileText, -1, NULL, 0, NULL, NULL);
                            if (utf8Len > 0) {
                                LPSTR lpFileTextUtf8 = (LPSTR)GlobalAlloc(GPTR, utf8Len);
                                if (lpFileTextUtf8 != NULL) {
                                    WideCharToMultiByte(CP_UTF8, 0, lpFileText, -1, lpFileTextUtf8, utf8Len, NULL, NULL);
                                    if (WriteFile(hFile, lpFileTextUtf8, utf8Len - 1, &dwWritten, NULL))
                                    {
                                        MessageBox(hWnd, L"Saved successfully", L"Success", MB_ICONINFORMATION);
                                    }
                                    else
                                    {
                                        MessageBox(hWnd, L"Failed to write the file", L"Error", MB_ICONERROR);
                                    }
                                    GlobalFree(lpFileTextUtf8);
                                }
                            }
                            GlobalFree(lpFileText);
                        }
                        CloseHandle(hFile);
                    }
                    else
                    {
                        MessageBox(hWnd, L"Failed to create the file", L"Error", MB_ICONERROR);
                    }
                }
            }
            break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 2));
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        if (MessageBox(hWnd, L"Really quit?", L"My application", MB_OKCANCEL) == IDOK)
        {
            DestroyWindow(hWnd);
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
