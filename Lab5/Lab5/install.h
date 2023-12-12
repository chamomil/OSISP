#pragma once
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")

#define ID_INSTALL_LISTVIEW 546
#define IDM_INSTALL 547

HWND g_hInstallListView;


struct InstallData {
	std::wstring displayName;
	std::wstring packageName;

	InstallData(const std::wstring& displayName, const std::wstring& packageName) : displayName(displayName), packageName(packageName) {}
};

void WingetInstall(std::wstring packageName) {
	std::wstring command = L"install -e --id " + packageName;
	SHELLEXECUTEINFO ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS; // == hProcess recieves a handle 
	ShExecInfo.lpVerb = L"open"; // specifies the action to be performed
	ShExecInfo.nShow = SW_SHOWNORMAL; // activates window, to hide - SW_HIDE
	ShExecInfo.lpFile = L"winget";
	ShExecInfo.lpParameters = command.c_str();
	ShellExecuteEx(&ShExecInfo);
}

void HandleInstall(HWND hMainWindow) {
	int selectedItem = ListView_GetNextItem(g_hInstallListView, -1, LVNI_SELECTED); // -1 - first one
	if (selectedItem != -1) {
		LVITEMW lvItem = { 0 };
		lvItem.mask = LVIF_PARAM;
		lvItem.iItem = selectedItem;
		lvItem.iSubItem = 0;
		ListView_GetItem(g_hInstallListView, &lvItem);

		InstallData* lparam = (InstallData*)lvItem.lParam;
		std::wstring message = L"Do you want to install " + lparam->displayName + L"?";
		DWORD res = MessageBoxW(hMainWindow, message.c_str(), L"Install", MB_ICONWARNING | MB_OKCANCEL);

		if (res == IDOK) {
			WingetInstall(lparam->packageName);
		}
	}
}

void PopulatInstallListView(HWND hListView) {
	std::vector<InstallData> items{ 
		InstallData(L"Total Registry", L"PavelYosifovich.TotalRegistry"),
		InstallData(L"Pomodoro Logger", L"zxch3n.PomodoroLogger"),
		InstallData(L"Total Commander", L"Ghisler.TotalCommander"),
		InstallData(L"micro", L"zyedidia.micro"),
		InstallData(L"Suside", L"zeankundev.suside"),
		InstallData(L"GitNote", L"zhaopengme.gitnote"),
		InstallData(L"ToDesk", L"Youqu.ToDesk"),
		InstallData(L"YouTube Music", L"Ytmdesktop.Ytmdesktop"),
		InstallData(L"YACReader", L"YACReader.YACReader"),
		InstallData(L"Spotify", L"Spotify.Spotify"),
		InstallData(L"Opera", L"Opera.OperaGX"),
		InstallData(L"Evernote", L"evernote.evernote"),
		InstallData(L"Rnote", L"flxzt.rnote"),
		InstallData(L"Notepad++", L"Notepad++.Notepad++"),
		InstallData(L"Knotes", L"1MHz.Knotes"),
	};

	ListView_DeleteAllItems(hListView);
	for (auto& data : items) {
		LVITEMW lvItem = { 0 };
		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.iItem = 0;
		lvItem.iSubItem = 0;
		lvItem.pszText = (LPWSTR)data.displayName.c_str();

		InstallData* lparam = new InstallData(data);
		lvItem.lParam = (LPARAM)lparam;

		ListView_InsertItem(hListView, &lvItem);
	}
}

HMENU CreateInstallContextMenu() {
	HMENU hMenu = CreatePopupMenu();
	AppendMenuW(hMenu, MF_STRING, IDM_INSTALL, L"Install"); // mp_string - specifies that the menu item is a text string;
	return hMenu;
}

void ShowInstallContextMenu(HWND hWnd, int xPos, int yPos) {
	HMENU hContextMenu = CreateInstallContextMenu();
	TrackPopupMenu(hContextMenu, TPM_TOPALIGN | TPM_LEFTALIGN, xPos, yPos, 0, hWnd, NULL); // 0 - means reserved, null - rectangle, must be ignored
	DestroyMenu(hContextMenu);
}

void CreateInstallListView(HWND hWnd, LPARAM lParam) {
	g_hInstallListView = CreateWindow(WC_LISTVIEW, L"List View",
		WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SORTASCENDING, 410, 10, 350, 540,
		hWnd, (HMENU)ID_INSTALL_LISTVIEW, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

	if (g_hInstallListView == NULL) {
		MessageBoxW(NULL, L"List view creation failed.", L"Error", MB_ICONERROR);
		return;
	}

	ListView_SetExtendedListViewStyle(g_hInstallListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER); // double buffer - for better painting of the widget (new versions only)
	LVCOLUMNW lvCol;
	lvCol.mask = LVCF_TEXT | LVCF_WIDTH; // cx, pszTextx are allowed
	lvCol.cx = 380;
	lvCol.pszText = (LPWSTR)L"Package to install";
	ListView_InsertColumn(g_hInstallListView, 0, &lvCol);

	PopulatInstallListView(g_hInstallListView);
}