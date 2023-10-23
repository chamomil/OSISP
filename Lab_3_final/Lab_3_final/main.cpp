#include <windows.h>
#include <CommCtrl.h>
#include <string>
#include <vector>
#include <shlobj.h> // for folder browsing

#pragma comment(lib, "comctl32.lib")

#define ID_TREE_VIEW 551
#define IDM_OPEN_FOLDER 554

HWND hTreeView;
HWND hMainWindow;
struct TreeViewItem {
    std::wstring fullPath;
    std::wstring shortPath;
    HANDLE thread;
    HTREEITEM currentItem;
};

HANDLE hMutex;

std::vector<std::wstring> fileChanges;


HTREEITEM AddItemToTreeView(HWND hTreeView, HTREEITEM hParent, LPCWSTR text)
{
    TVINSERTSTRUCT tvInsert;
    tvInsert.hParent = hParent;
    tvInsert.hInsertAfter = TVI_LAST;
    tvInsert.item.mask = TVIF_TEXT;
    tvInsert.item.pszText = (LPWSTR)text;

    return TreeView_InsertItem(hTreeView, &tvInsert);
}

HTREEITEM FindChildItemByName(HTREEITEM hParent, LPCWSTR targetName) {
    HTREEITEM hChild = TreeView_GetChild(hTreeView, hParent);

    while (hChild) {
        TVITEM tvi;
        ZeroMemory(&tvi, sizeof(TVITEM));
        tvi.mask = TVIF_TEXT;
        tvi.hItem = hChild;
        tvi.pszText = new WCHAR[MAX_PATH]; // allocating memory for item text
        tvi.cchTextMax = MAX_PATH;

        TreeView_GetItem(hTreeView, &tvi); // getting the text of the current item

        if (wcscmp(tvi.pszText, targetName) == 0) { // comparing item's text to the target name
            delete[] tvi.pszText;
            return hChild;
        }

        delete[] tvi.pszText;

        hChild = TreeView_GetNextSibling(hTreeView, hChild);
    }
    return NULL; // Item not found
}

DWORD WINAPI MonitorDirectoryChanges(LPVOID lparam)
{
    auto item = (TreeViewItem*)lparam;
    HANDLE hDir = CreateFile( // open dir
        item->fullPath.c_str(),
        FILE_LIST_DIRECTORY, // grants to read a list of content
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // file can be shared while program is using it
        NULL, // security attributes
        OPEN_EXISTING, // creation options
        FILE_FLAG_BACKUP_SEMANTICS, // this flag is to obtain handle to a dir
        NULL); // template

    if (hDir == INVALID_HANDLE_VALUE) {
        return 1;
    }

    BYTE buffer[4096]; // regular size
    DWORD bytesReturned;

    while (true) {
        if (ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            FALSE, // false means function monitors only changes within this dir
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
            &bytesReturned, // amount of bytes transferred to buffer
            NULL, // indicates, that obtained data is not going to be used asynchronousely
            NULL)) { // pointer to completion routine

            FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)buffer; // fni is a change
            int offset = -1; // initialization of variables before the cycle
            std::wstring prevName = L"";
            std::wstring log = L"error";
            do {
                std::wstring filename = std::wstring(fni->FileName, fni->FileNameLength / 2);
                offset = fni->NextEntryOffset; // the number of bytes that must be skipped to get to the next record

                if (fni->Action == FILE_ACTION_ADDED) {
                    WIN32_FIND_DATA findFileData;
                    HANDLE hFind = FindFirstFile((item->fullPath + L"\\" + filename).c_str(), &findFileData);
                    if (hFind != INVALID_HANDLE_VALUE) {
                        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { // for dirs
                            HTREEITEM hItem = AddItemToTreeView(hTreeView, item->currentItem, findFileData.cFileName);

                            // associate the full path with the tree item as item data
                            TreeViewItem* lparam = new TreeViewItem{ item->fullPath + L"\\" + findFileData.cFileName, std::wstring(findFileData.cFileName), NULL, hItem };
                            TVITEMEX tvItem;
                            tvItem.hItem = hItem;
                            tvItem.mask = TVIF_PARAM;
                            tvItem.lParam = (LPARAM)lparam;

                            TreeView_SetItem(hTreeView, &tvItem);
                            AddItemToTreeView(hTreeView, hItem, L""); // dummy item
                            TreeView_Expand(hTreeView, hItem, TVE_EXPAND);
                        }
                        else { // for regular files
                            AddItemToTreeView(hTreeView, item->currentItem, findFileData.cFileName);
                        }
                    }
                    log = L"new object created, filename: " + filename;
                }

                if (fni->Action == FILE_ACTION_REMOVED) {
                    auto fullname = item->fullPath + L"\\" + filename;
                    auto itemToDelete = FindChildItemByName(item->currentItem, filename.c_str());
                    if (itemToDelete) {
                        TreeView_DeleteItem(hTreeView, itemToDelete);
                    }
                    log = L"object removed, filename: " + filename;
                }

                if (fni->Action == FILE_ACTION_RENAMED_OLD_NAME) {
                    prevName = filename; // saving old name to change it in program
                }

                if (fni->Action == FILE_ACTION_RENAMED_NEW_NAME) {
                    auto prevFullname = item->fullPath + L"\\" + prevName;
                    auto fullname = item->fullPath + L"\\" + filename;
                    auto itemToRename = FindChildItemByName(item->currentItem, prevName.c_str());
                    if (itemToRename) {
                        TVITEMEX tvItem;
                        tvItem.hItem = itemToRename;
                        tvItem.mask = TVIF_PARAM;
                        
                        if (TreeView_GetItem(hTreeView, &tvItem)) { // for dirs
                            TreeViewItem* lparam = (TreeViewItem*)tvItem.lParam;
                            if (lparam) {
                                lparam->fullPath = fullname;
                                lparam->shortPath = filename;
                            }
                        }
                        
                        {
                            TVITEMEX tvItem;
                            tvItem.hItem = itemToRename;
                            tvItem.mask = TVIF_TEXT;
                            tvItem.pszText = (LPWSTR)filename.c_str();
                            TreeView_SetItem(hTreeView, &tvItem);
                        }
                        
                    }
                    log = L"object renamed, old filename: " + prevName + L", new filename: " + filename;
                }

                fni = (FILE_NOTIFY_INFORMATION*)((char*)fni + offset);
            } while (offset != 0); // cycle ends when no other changes were found


            DWORD dwWaitResult = WaitForSingleObject(hMutex, INFINITE); // waiting for signal
            if (dwWaitResult == WAIT_OBJECT_0) { // if signaled
                fileChanges.push_back(log);
                ReleaseMutex(hMutex);
            }

        }
        else {
            break;
        }
        Sleep(50);
    }

    CloseHandle(hDir);
    return 0;
}

// populating the tree-view with directories and files
void PopulateTreeView(HTREEITEM hParent, const std::wstring& parentPath)
{
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((parentPath + L"\\*").c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { // for dirs
                    HTREEITEM hItem = AddItemToTreeView(hTreeView, hParent, findFileData.cFileName);

                    // associate the full path with the tree item as item data
                    TreeViewItem* lparam = new TreeViewItem{ parentPath + L"\\" + findFileData.cFileName, std::wstring(findFileData.cFileName), NULL, hItem };
                    TVITEMEX tvItem;
                    tvItem.hItem = hItem;
                    tvItem.mask = TVIF_PARAM;
                    tvItem.lParam = (LPARAM)lparam;

                    TreeView_SetItem(hTreeView, &tvItem);
                    AddItemToTreeView(hTreeView, hItem, L""); // dummy item
                }
                else { // for other files
                    AddItemToTreeView(hTreeView, hParent, findFileData.cFileName);
                }
            }
        } while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
}

void InitTreeView(const std::wstring& dir)
{
    auto root = AddItemToTreeView(hTreeView, NULL, dir.c_str());
    // associate the full path with the tree item as item data
    TreeViewItem* lparam = new TreeViewItem{ dir, dir, NULL, root };
    TVITEMEX tvItem;
    tvItem.hItem = root;
    tvItem.mask = TVIF_PARAM;
    tvItem.lParam = (LPARAM)lparam;

    TreeView_SetItem(hTreeView, &tvItem);
    AddItemToTreeView(hTreeView, root, L""); // add a dummy item
}

void OnCreate(HWND hWnd)
{
    hTreeView = CreateWindowEx(0, WC_TREEVIEW, L"", WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_INFOTIP, 0, 0, 700, 500, hWnd, (HMENU)ID_TREE_VIEW, NULL, NULL);
    InitTreeView(L"C:\\");
    // infotip - for notifications
}

void OnNotify(LPARAM lParam)
{
    LPNMHDR pnmh = (LPNMHDR)lParam; //handle for tree-view notifications

    if (pnmh->idFrom == ID_TREE_VIEW && pnmh->code == TVN_ITEMEXPANDED) { //itemexpanded notifies when a parent item's list of child items
        //is about to expand or collapse
        LPNMTREEVIEW pNMTV = (LPNMTREEVIEW)lParam;
        HTREEITEM hItem = pNMTV->itemNew.hItem;
        TVITEMEX tvItem;
        tvItem.hItem = hItem;
        tvItem.mask = TVIF_PARAM | TVIF_STATE;
        tvItem.stateMask = TVIS_EXPANDED;

        if (TreeView_GetItem(hTreeView, &tvItem)) { // retrieves all of a tree-view item's attributes
            // checks if the item data contains a full path (which means it's a directory)
            if (tvItem.state & TVIS_EXPANDED) {
                TreeViewItem* lparam = (TreeViewItem*)tvItem.lParam;

                if (lparam != nullptr) {
                    std::wstring fullPath = lparam->fullPath;

                    /*HTREEITEM hChild = TreeView_GetChild(hTreeView, hItem);
                    if (hChild != NULL) {
                        TreeView_DeleteItem(hTreeView, hChild);
                    }*/

                    lparam->thread = CreateThread(NULL, NULL, MonitorDirectoryChanges, lparam, NULL, NULL);
                    // pointer to security attributes (whether to allow to inherit handle or not), stack size (null means default), pointer to executed func,
                    //pointer to a variable passed to thread, flags (null - runs immideately after creation), pointer to a variable that receives the thread id (null means it's not returned)
                    PopulateTreeView(hItem, fullPath);
                }
            }
            else { // not expanded (changed)
                HTREEITEM hParent = TreeView_GetParent(hTreeView, hItem);
                LPARAM lparam = tvItem.lParam;
                TreeView_DeleteItem(hTreeView, hItem);
                HTREEITEM hNewItem = AddItemToTreeView(hTreeView, hParent, ((TreeViewItem*)lparam)->shortPath.c_str());

                TVITEMEX tvNewItem;
                tvNewItem.hItem = hNewItem;
                tvNewItem.mask = TVIF_PARAM;
                tvNewItem.lParam = lparam;

                TreeView_SetItem(hTreeView, &tvNewItem); // sets attributes from tvNewItem

                AddItemToTreeView(hTreeView, hNewItem, L"");
                TreeView_SortChildren(hTreeView, hParent, FALSE); // false - for recursion
            }
        }
    }

    if (pnmh->idFrom == ID_TREE_VIEW && pnmh->code == TVN_DELETEITEM) {
        LPNMTREEVIEW pNMTV = (LPNMTREEVIEW)lParam;
        HTREEITEM hItem = pNMTV->itemOld.hItem;
        TVITEMEX tvItem;
        tvItem.hItem = hItem;
        tvItem.mask = TVIF_PARAM;

        if (TreeView_GetItem(hTreeView, &tvItem)) {
            auto data = (TreeViewItem*)tvItem.lParam;
            if (data != nullptr && data->thread != nullptr) {
                TerminateThread(data->thread, 0);
            }
        }
    }
}

void OpenFolderDialog()
{
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"Select a folder:";
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != NULL) {
        wchar_t selectedPath[MAX_PATH];
        SHGetPathFromIDList(pidl, selectedPath);
        TreeView_DeleteAllItems(hTreeView);
        InitTreeView(selectedPath);
        CoTaskMemFree(pidl); // free mem
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg) {
    case WM_CREATE:
        OnCreate(hWnd);
        break;
    case WM_NOTIFY: {
        OnNotify(lParam);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDM_OPEN_FOLDER) {
            OpenFolderDialog();
        }
        break;
    case WM_DESTROY:
        if (fileChanges.size() > 0) {
            std::wstring fullLog;
            for (auto& log : fileChanges) {
                fullLog += log + L"\n";
            }
            MessageBox(hMainWindow, fullLog.c_str(), L"file changes", MB_OK);
        }

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hMutex = CreateMutex(NULL, FALSE, NULL);
    if (hMutex == NULL) {
        return 1;
    }

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"TreeView", NULL };
    //size, style, pointer to window procedure, extra mem for struct, extra mem for instance, handle to instance that contains window procedure, icon, cursor,
    //brush, pointer to class menu, pointer to str, small icon
    RegisterClassEx(&wc);
    hMainWindow = CreateWindow(wc.lpszClassName, L"TreeView", WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, 100, 100, 800, 600, GetDesktopWindow(), NULL, wc.hInstance, NULL);
    ShowWindow(hMainWindow, nCmdShow);

    HMENU hMenu = CreateMenu();
    AppendMenu(hMenu, MF_STRING, IDM_OPEN_FOLDER, L"Open Directory");
    SetMenu(hMainWindow, hMenu);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnregisterClass(wc.lpszClassName, wc.hInstance);
    CloseHandle(hMutex);
    return 0;
}
