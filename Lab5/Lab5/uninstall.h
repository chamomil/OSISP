#pragma once
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

#define IDM_UNINSTALL 13
#define IDM_UNINSTALL_RELOAD 14
#define ID_UNINSTALL_LISTVIEW 153

struct UninstallData {
	std::wstring displayName;
	std::wstring uninstallString;
};

HWND g_hUninstallListView;

void PopulateUninstallListView(HWND hListView, HKEY hive, std::wstring registryPath) {
	HKEY hKey;
	if (RegOpenKeyExW(hive, registryPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		DWORD index = 0;
		WCHAR subkeyName[255];
		DWORD subkeyNameSize = sizeof(subkeyName);

		while (RegEnumKeyExW(hKey, index, subkeyName, &subkeyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
			HKEY subKey;
			if (RegOpenKeyExW(hKey, subkeyName, 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
				std::wstring displayName;
				displayName.resize(1024);
				std::wstring uninstallString;
				uninstallString.resize(1024);
				DWORD dislpayNameStringSize = 1024;
				DWORD uninstallStringSize = 1024;

				if (RegQueryValueExW(subKey, L"DisplayName", NULL, NULL, (LPBYTE)&displayName[0], &dislpayNameStringSize) == ERROR_SUCCESS
					&& RegQueryValueExW(subKey, L"UninstallString", NULL, NULL, (LPBYTE)&uninstallString[0], &uninstallStringSize) == ERROR_SUCCESS) {

					std::wstring copy = uninstallString;
					std::transform(copy.begin(), copy.end(), copy.begin(), tolower);
					if (copy.rfind(L"msiexec.exe /i") != 0) {
						LVITEMW lvItem = { 0 };
						lvItem.mask = LVIF_TEXT | LVIF_PARAM;
						lvItem.iItem = index;
						lvItem.iSubItem = 0;
						lvItem.pszText = (LPWSTR)displayName.c_str();

						UninstallData* lparam = new UninstallData();
						lparam->uninstallString = uninstallString;
						lparam->displayName = displayName;

						lvItem.lParam = (LPARAM)lparam;

						ListView_InsertItem(hListView, &lvItem);
					}
				}
				RegCloseKey(subKey);
			}

			index++;
			subkeyNameSize = sizeof(subkeyName);
		}

		RegCloseKey(hKey);
	}
}

HMENU CreateUninstallContextMenu() {
	HMENU hMenu = CreatePopupMenu();
	AppendMenuW(hMenu, MF_STRING, IDM_UNINSTALL, L"Uninstall");
	AppendMenuW(hMenu, MF_STRING, IDM_UNINSTALL_RELOAD, L"Reload");
	return hMenu;
}

void ShowUninstallContextMenu(HWND hWnd, int xPos, int yPos) {
	HMENU hContextMenu = CreateUninstallContextMenu();
	TrackPopupMenu(hContextMenu, TPM_TOPALIGN | TPM_LEFTALIGN, xPos, yPos, 0, hWnd, NULL);
	DestroyMenu(hContextMenu);
}


void ExecuteUninstallCommand(const std::wstring& filepath) {
	if (filepath.rfind(L"winget") == 0) {
		int index = 7; // after winget
		std::wstring command = filepath.substr(index, filepath.length() - index);
		SHELLEXECUTEINFO ShExecInfo = { 0 };
		ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShExecInfo.lpVerb = L"open";
		ShExecInfo.nShow = SW_SHOWNORMAL;
		ShExecInfo.lpFile = L"winget";
		ShExecInfo.lpParameters = command.c_str();
		ShellExecuteEx(&ShExecInfo);
		return;
	}

	int index = filepath.find(L".exe");
	while (index < filepath.length() && filepath[index] != L' ') {
		index++;
	}

	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.lpVerb = L"open";
	ShExecInfo.nShow = SW_SHOWNORMAL;
	if (index != filepath.length()) {
		std::wstring fileLocation = filepath.substr(0, index);
		std::wstring command = filepath.substr(index + 1, filepath.length() - (index + 1));
		ShExecInfo.lpFile = fileLocation.c_str();
		ShExecInfo.lpParameters = command.c_str();
		ShellExecuteEx(&ShExecInfo);
	}
	else {
		ShExecInfo.lpFile = filepath.c_str();
		ShellExecuteEx(&ShExecInfo);
	}
}

void HandleUninstall(HWND hMainWindow) {
	int selectedItem = ListView_GetNextItem(g_hUninstallListView, -1, LVNI_SELECTED);
	if (selectedItem != -1) {
		LVITEMW lvItem = { 0 };
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = selectedItem;
		lvItem.iSubItem = 0;
		ListView_GetItem(g_hUninstallListView, &lvItem);

		UninstallData* lparam = (UninstallData*)lvItem.lParam;
		std::wstring message = L"Do you want to uninstall " + lparam->displayName + L"?";
		DWORD res = MessageBoxW(hMainWindow, message.c_str(), L"Uninstall", MB_ICONWARNING | MB_OKCANCEL);

		if (res == IDOK) {
			ExecuteUninstallCommand(lparam->uninstallString);
		}
	}
}

void PopulateWholeUninstallListView(HWND listView) {
	ListView_DeleteAllItems(listView);
	PopulateUninstallListView(g_hUninstallListView, HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
	PopulateUninstallListView(g_hUninstallListView, HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
	PopulateUninstallListView(g_hUninstallListView, HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");

}

void CreateUninstallListView(HWND hWnd, LPARAM lParam) {
	g_hUninstallListView = CreateWindow(WC_LISTVIEW, L"List View",
		WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SORTASCENDING, 10, 10, 380, 540,
		hWnd, (HMENU)ID_UNINSTALL_LISTVIEW, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

	if (g_hUninstallListView == NULL) {
		MessageBoxW(NULL, L"List view creation failed.", L"Error", MB_ICONERROR);
		return;
	}

	ListView_SetExtendedListViewStyle(g_hUninstallListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
	LVCOLUMNW lvCol;
	lvCol.mask = LVCF_TEXT | LVCF_WIDTH;
	lvCol.cx = 380;
	lvCol.pszText = (LPWSTR)L"Display Name";
	ListView_InsertColumn(g_hUninstallListView, 0, &lvCol);

	PopulateWholeUninstallListView(g_hUninstallListView);
}