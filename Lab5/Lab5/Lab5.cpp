#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

#include "uninstall.h"
#include "install.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CREATE:
	{
		CreateUninstallListView(hWnd, lParam);
		CreateInstallListView(hWnd, lParam);
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDM_UNINSTALL:
			HandleUninstall(hWnd);
			break;
		case IDM_UNINSTALL_RELOAD:
			PopulateWholeUninstallListView(g_hUninstallListView);
			break;
		case IDM_INSTALL:
			HandleInstall(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == ID_UNINSTALL_LISTVIEW && ((LPNMHDR)lParam)->code == NM_RCLICK) {
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			ShowUninstallContextMenu(hWnd, cursorPos.x, cursorPos.y);
		}
		if (((LPNMHDR)lParam)->idFrom == ID_INSTALL_LISTVIEW && ((LPNMHDR)lParam)->code == NM_RCLICK) {
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			ShowInstallContextMenu(hWnd, cursorPos.x, cursorPos.y);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	WNDCLASSW wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"RegistryDisplayNames";
	RegisterClassW(&wc);

	HWND hWnd = CreateWindowW(L"RegistryDisplayNames", L"Registry Display Names",
		WS_SYSMENU | WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

	if (hWnd == NULL) {
		MessageBoxW(NULL, L"Window creation failed.", L"Error", MB_ICONERROR);
		return 1;
	}

	ShowWindow(hWnd, nCmdShow);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
