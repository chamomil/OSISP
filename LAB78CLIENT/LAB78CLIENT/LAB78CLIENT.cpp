#include <WS2tcpip.h>

#include <windows.h>

#include <string>

#include <commctrl.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

HWND g_hListView;
SOCKET g_clientSocket;
HANDLE g_hThread;
HWND hwndEdit;
HWND hwndBtn;

void SendMessageToServer(const std::wstring& message)
{
    std::string s(message.begin(), message.end());
    send(g_clientSocket, s.c_str(), s.size(), 0);
}

void AddMessageToListView(const std::string& message)
{
    auto normalMessage = std::wstring(message.begin(), message.end());
    LVITEM lvItem;
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = ListView_GetItemCount(g_hListView);
    lvItem.iSubItem = 0;
    lvItem.pszText = (LPWSTR)normalMessage.c_str();

    ListView_InsertItem(g_hListView, &lvItem);
}

DWORD WINAPI MessageListener(LPVOID lpParam)
{
    char buffer[1024];
    while (true) {
        int recvResult = recv(g_clientSocket, buffer, sizeof(buffer), 0);
        if (recvResult > 0) {
            buffer[recvResult] = '\0';
            std::string message(buffer);
            AddMessageToListView(message);
        } else if (recvResult == 0) {
            closesocket(g_clientSocket);
            MessageBox(nullptr, L"Server disconnected", L"Info", MB_OK | MB_ICONINFORMATION);
            break;
        } else {
            MessageBox(nullptr, L"Error receiving message", L"Error", MB_OK | MB_ICONERROR);
            closesocket(g_clientSocket);
            break;
        }
    }
}
bool isConnected;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE: {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        g_hListView = CreateWindowEx(
            0, WC_LISTVIEW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT,
            10, 10, 400, 300, hwnd, nullptr, nullptr, nullptr);

        //exstyle, className, window name, style, x, y, width, height, parent, id to manipulate, instance of assiciated module, pointer to passed params

        LVCOLUMN lvCol;
        lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvCol.cx = 300;
        lvCol.pszText = (LPWSTR)L"Messages";
        lvCol.iSubItem = 0;
        ListView_InsertColumn(g_hListView, 0, &lvCol);

        hwndBtn= CreateWindow(
            L"BUTTON", L"Connect",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 320, 80, 25,
            hwnd, (HMENU)1, nullptr, nullptr);

        hwndEdit = CreateWindow(
            L"Edit", L"Name",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | WS_BORDER,
            110, 320, 160, 25, hwnd, (HMENU)2, // edit control ID
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL); // application instance

        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) {
            if (!isConnected) {
                g_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (g_clientSocket == INVALID_SOCKET) {
                    MessageBox(hwnd, L"Error creating socket", L"Error", MB_OK | MB_ICONERROR);
                    return 0;
                }

                sockaddr_in serverAddr;
                serverAddr.sin_family = AF_INET;
                serverAddr.sin_port = htons(8888);
                inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); // ip to binary
                //family, address, buffer

                if (connect(g_clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
                    MessageBox(hwnd, L"Error connecting to server", L"Error", MB_OK | MB_ICONERROR);
                    closesocket(g_clientSocket);
                    return 0;
                }
                WCHAR buff[1024];
                GetWindowText(hwndEdit, buff, 1024);
                SendMessageToServer(buff);
                isConnected = true;
                SetWindowText(hwndEdit, L"");
                g_hThread = CreateThread(nullptr, 0, MessageListener, nullptr, 0, nullptr);
                SetWindowText(hwndBtn, L"Send");
            } else {
                WCHAR buff[1024];
                GetWindowText(hwndEdit, buff, 1024);
                SendMessageToServer(buff);
                SetWindowText(hwndEdit, L"");
            }
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default: {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    }

    return 0;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ChatClientClass";

    if (RegisterClass(&wc)) {
        HWND hwnd = CreateWindowEx(
            0, L"ChatClientClass", L"Chat Client",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 450, 400,
            nullptr, nullptr, hInstance, nullptr);

        if (hwnd) {
            ShowWindow(hwnd, nCmdShow);

            MSG msg;
            while (GetMessage(&msg, nullptr, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return 0;
}
