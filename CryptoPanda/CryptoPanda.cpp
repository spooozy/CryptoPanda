#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>
#include <dpapi.h>
#include "Encoder.h"
#include <filesystem>
#include <sstream>
#include <commctrl.h>

#define NEW_STORAGE 1
#define ADD_STORAGE 2
#define BACK 3
#define ADD_STORAGE_PATH 4
#define ADD_STORAGE_NAME 5
#define ADD_STORAGE_PASS 6
#define ADD_STORAGE_REP 7
#define PASS_CHECK 8
#define PASS_CANCEL 9
#define LOCK_STORAGE 10
#define ADD_IN_STORAGE 11
#define ADD_FILE_OK 12
#define ADD_FILE_CANCEL 13


#pragma comment (lib, "Crypt32")

int clientWidth = 0, clientHeight = 0, stListWndWidth, stListWndHeight, listScrollId = 0, listItmHeight = 65, scrollPos = 0, leftStorages = 0, stItmsToShow = 0,
controllHeight = 20, storageID = 0, fileID = 0;
RECT clientRect;
HWND mainWnd, stListWnd, addStorageWnd, newDirInput, newNameInput, newPassInput, newPassRepInput, dialogWnd, passCheckEdit, viewStorageWnd, addFileInStorageWnd,
newFilePathEdit, newFileNameEdit, fileInStorageListWnd;
HMENU fileInStorageFunc;
HINSTANCE hInst;
Encoder *encoder = nullptr;

struct StorageInfo {
    std::wstring path;
    std::wstring name;
    std::wstring folders;
    bool locked;
};

struct FileNames {
    std::wstring name;
    std::wstring encodedName;
};

std::vector<StorageInfo> storages;
std::vector<FileNames> filesInStorage;


void DeleteFilesFromFileTable();
std::wstring GetPassword();
void GetFilesInStorage();
void UpdateScroll();
void UpdateClientSize();
void ReadEntriesFromFile(const std::wstring& filename);
void DrawList(HWND hwnd, HDC hdc);
void CreateChildWnd();
void NewStorageText(HDC hdc);
void AddNewStorage();
bool СontainsInvalidChars(const std::wstring& input);
bool CreateFolder(const std::wstring& path, const std::wstring& folderName);
std::vector<BYTE> LoadPasswordByIndex(const std::wstring& filename, size_t index);
std::vector<BYTE> UnprotectData(const BYTE* data, DWORD dataSize);
void SaveToFile(const std::wstring& filename, const std::vector<BYTE>& data);
std::vector<BYTE> ProtectData(const BYTE* data, DWORD dataSize);
void SetDialogWnd();
bool CheckPassword();
bool ComparePassword(const std::wstring& inputPassword, const std::wstring& filename, size_t index);
void ViewStorage(HDC hdc);
void SetAddFileWnd();
void AddNewFileInStorage();
void AddFilesInFileList();
void AddItemInFileTable(std::wstring newFile);
int GetFileItemClickPos(int x, int y);
bool DeleteFileFromStorage(int fileId);
bool ExportFileFromStorage(int fileId);
void StorageDntExist(int id);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT clearRect = { 0,0,clientWidth, clientHeight};
    HDC hdc;
    PAINTSTRUCT ps;
    switch (uMsg) {
    case WM_CREATE: 
        break;
    case WM_DESTROY:        
        PostQuitMessage(0);
        break;
    case WM_SIZE:        
        UpdateClientSize();
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);        
        break;   
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 700; 
        mmi->ptMinTrackSize.y = 600;
        mmi->ptMaxTrackSize.x = 700;
        mmi->ptMaxTrackSize.y = 600;
        return 0;
    }    
    case WM_PAINT: 
        hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &clearRect, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
        break;   
    
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK stListWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT clearRect = {0,0, stListWndWidth, stListWndHeight};
    RECT mainRect = { stListWndWidth, 0, clientWidth - stListWndWidth, stListWndHeight };
    HDC hdc;
    PAINTSTRUCT ps;
    switch (uMsg) {
    case WM_CREATE:       
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE: 
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == NEW_STORAGE)
        {                        
            ShowWindow(viewStorageWnd, SW_HIDE);
            ShowWindow(addStorageWnd, SW_SHOW);
            InvalidateRect(mainWnd, &mainRect, TRUE);
            UpdateWindow(mainWnd);
            UpdateWindow(addStorageWnd);
        }
        break;
    case WM_PAINT: 
        hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &clearRect, (HBRUSH)(COLOR_WINDOW + 1));
        DrawList(hwnd, hdc);
        EndPaint(hwnd, &ps);
        break;
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int clickedIndex = y / listItmHeight;

        if (clickedIndex >= 0 && clickedIndex < storages.size()) {
            storageID = clickedIndex;
            if (storages[clickedIndex].locked) {
                SetDialogWnd();
            }
            else 
            {
                ShowWindow(viewStorageWnd, SW_SHOW);
                ShowWindow(addStorageWnd, SW_HIDE);
                InvalidateRect(mainWnd, &mainRect, TRUE);
                UpdateWindow(mainWnd);
            }
        }
        break;
    }
    case WM_VSCROLL: {
        int oldPos = scrollPos;
        switch (LOWORD(wParam)) {
        case SB_LINEUP:
            if (--scrollPos < 0) scrollPos = 0;
            break;
        case SB_LINEDOWN:
            if (++scrollPos > leftStorages) scrollPos = leftStorages;
            else leftStorages--;
            break;
        }
        if (oldPos != scrollPos)
        {            
            SetScrollPos(stListWnd, SB_VERT, scrollPos, true);
            InvalidateRect(stListWnd, NULL, true);
        }
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK dialogWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT clearRect = { stListWndWidth, 0, clientWidth - stListWndWidth, stListWndHeight };
    HDC hdc;
    PAINTSTRUCT ps;
    switch (uMsg) {
    case WM_CREATE:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        break;
    case WM_PAINT:   
        hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &clearRect, (HBRUSH)(COLOR_WINDOW + 1));
        TextOut(hdc, 20, 20, L"Enter storage password:", 23);
        EndPaint(hwnd, &ps);
        break;
    case WM_COMMAND:   
        if (LOWORD(wParam) == PASS_CANCEL) {
            SetWindowText(passCheckEdit, L"");
            CloseWindow(dialogWnd);
        }
        else if (LOWORD(wParam) == PASS_CHECK) {
            if (CheckPassword()) 
            {
                storages[storageID].locked = false;
                filesInStorage.clear();
                DeleteFilesFromFileTable();
                ShowWindow(viewStorageWnd, SW_SHOW);
                ShowWindow(addStorageWnd, SW_HIDE);
                InvalidateRect(mainWnd, &clearRect, TRUE);
                UpdateWindow(mainWnd);
                SetWindowText(passCheckEdit, L"");
                CloseWindow(dialogWnd);
                GetFilesInStorage();
                AddFilesInFileList();
            }            
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd); 
        return 0;
        break;    
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK addFileWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT clearRect = { stListWndWidth, 0, clientWidth - stListWndWidth, stListWndHeight };
    HDC hdc;
    PAINTSTRUCT ps;
    switch (uMsg) {
    case WM_CREATE:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &clearRect, (HBRUSH)(COLOR_WINDOW + 1));
        TextOut(hdc, 20, 10, L"Enter path to file:", 19);
        TextOut(hdc, 20, 65, L"Enter name:", 10);
        EndPaint(hwnd, &ps);
        break;
    case WM_COMMAND:   
        if (LOWORD(wParam) == ADD_FILE_CANCEL) {
            CloseWindow(hwnd);
        }
        if (LOWORD(wParam) == ADD_FILE_OK) {
            CloseWindow(hwnd);
            AddNewFileInStorage();
            InvalidateRect(fileInStorageListWnd, NULL, true);
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK addStorageWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    RECT clearRect = { stListWndWidth, 0, clientWidth - stListWndWidth, stListWndHeight };
    RECT listRect = {0, 0, stListWndWidth, stListWndHeight};
    HDC hdc;
    PAINTSTRUCT ps;
    switch (uMsg) {
    case WM_CREATE:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &clearRect, (HBRUSH)(COLOR_WINDOW + 1));
        NewStorageText(hdc);
        EndPaint(hwnd, &ps);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == BACK)
        {
            SetWindowText(newDirInput, L"");
            SetWindowText(newNameInput, L"");
            SetWindowText(newPassInput, L"");
            SetWindowText(newPassRepInput, L"");
            ShowWindow(hwnd, SW_HIDE);
            InvalidateRect(mainWnd, &clearRect, TRUE);
            UpdateWindow(mainWnd);
        }
        else if (LOWORD(wParam) == ADD_STORAGE)
        {
            AddNewStorage();
            UpdateScroll();
            InvalidateRect(stListWnd, &listRect, TRUE);
            UpdateWindow(stListWnd);
        }
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK viewStorageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    int clickPosX, clickPosY, commandId, itemIndex;
    RECT clearRect = { stListWndWidth, 0, clientWidth - stListWndWidth, stListWndHeight };
    HDC hdc;
    PAINTSTRUCT ps;
    switch (uMsg) {
    case WM_CREATE:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &clearRect, (HBRUSH)(COLOR_WINDOW + 1));
        ViewStorage(hdc);
        EndPaint(hwnd, &ps);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == LOCK_STORAGE)
        {
            storages[storageID].locked = true;
            ShowWindow(hwnd, SW_HIDE);
            InvalidateRect(mainWnd, &clearRect, TRUE);
            UpdateWindow(mainWnd);
        }
        else if (LOWORD(wParam) == ADD_IN_STORAGE) {
            SetAddFileWnd();
        }  
        else if (LOWORD(wParam) == 20) {
            bool res = DeleteFileFromStorage(fileID);
        }
        else if (LOWORD(wParam) == 21) {
            bool res = ExportFileFromStorage(fileID);
        }
        break;   
    case WM_NOTIFY: {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        if (nmhdr->hwndFrom == fileInStorageListWnd && nmhdr->code == NM_CLICK) {
            LVHITTESTINFO hit;
            POINT pt;
            DWORD pos = GetMessagePos();
            pt.x = LOWORD(pos);
            pt.y = HIWORD(pos);
            ScreenToClient(fileInStorageListWnd, &pt);
            fileID = GetFileItemClickPos(pt.x, pt.y);
            if (fileID != -1) {
                POINT screenPt = { pt.x, pt.y };
                ClientToScreen(fileInStorageListWnd, &screenPt);
                TrackPopupMenu(fileInStorageFunc, TPM_RIGHTBUTTON, screenPt.x, screenPt.y, 0, hwnd, NULL);
            }
        }
        break;
    }   
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd) {
   
    const wchar_t CLASS_NAME[] = L"CryptoPanda";
    ReadEntriesFromFile(L"D:\\БГУИР\\Курсовые\\CryptoPanda\\CryptoPanda\\CPStorages.txt");

    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    clientWidth = workArea.right - workArea.left;
    clientHeight = workArea.bottom - workArea.top;

    //основное окно
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);
    mainWnd = CreateWindowEx(
        0, CLASS_NAME, L"CryptoPanda",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 600,
        NULL, NULL, hInstance, NULL
    );
    if (mainWnd == NULL) {
        return 0;
    }
    UpdateClientSize();   
    
    encoder = new Encoder();

    CreateChildWnd();
    ShowWindow(stListWnd, SW_SHOW);
    ShowWindow(addStorageWnd, SW_HIDE);
    ShowWindow(viewStorageWnd, SW_HIDE);

    InvalidateRect(mainWnd, NULL, TRUE);
    UpdateWindow(mainWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

void CreateChildWnd() {
    //список доступных хранилищ
    int rows = storages.size() - 8;
    if (rows < 0) rows = 0;
    WNDCLASS wc = {};
    wc.lpfnWndProc = stListWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"StorageList";
    RegisterClass(&wc);
    stListWnd = CreateWindowEx(0, L"StorageList", L"Storage List", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL, 0, 0, stListWndWidth, stListWndHeight, mainWnd, NULL, hInst, NULL);
    CreateWindow(L"Button", L"New Storage", WS_CHILD | WS_VISIBLE, 0, stListWndHeight - 41, stListWndWidth, 41, stListWnd, (HMENU)NEW_STORAGE, hInst, nullptr);
    UpdateScroll();

    //создание хранилища
    WNDCLASS wc1 = {};
    wc1.lpfnWndProc = addStorageWndProc;
    wc1.hInstance = hInst;
    wc1.lpszClassName = L"AddStorage";
    RegisterClass(&wc1);
    addStorageWnd = CreateWindowEx(0, L"AddStorage", L"AddStorage", WS_CHILD | WS_VISIBLE /*| WS_BORDER*/, stListWndWidth, 0, clientWidth - stListWndWidth, stListWndHeight, mainWnd, NULL, hInst, NULL);
    newDirInput = CreateWindow(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 25, 60, clientWidth - stListWndWidth - 50, controllHeight, addStorageWnd, (HMENU)ADD_STORAGE_PATH, NULL, NULL);
    newNameInput = CreateWindow(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 25, 140, clientWidth - stListWndWidth - 50, controllHeight, addStorageWnd, (HMENU)ADD_STORAGE_NAME, NULL, NULL);
    newPassInput = CreateWindow(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 25, 220, clientWidth - stListWndWidth - 50, controllHeight, addStorageWnd, (HMENU)ADD_STORAGE_PASS, NULL, NULL);
    newPassRepInput = CreateWindow(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 25, 300, clientWidth - stListWndWidth - 50, controllHeight, addStorageWnd, (HMENU)ADD_STORAGE_REP, NULL, NULL);
    CreateWindow(L"Button", L"Add Storage", WS_CHILD | WS_VISIBLE, 25, 360, clientWidth - stListWndWidth - 50, 25, addStorageWnd, (HMENU)ADD_STORAGE, hInst, nullptr);
    CreateWindow(L"Button", L"Back", WS_CHILD | WS_VISIBLE, 25, 500, 100, 25, addStorageWnd, (HMENU)BACK, hInst, nullptr);

    //просмотр содержимого хранилища
    wc = {};
    wc.lpfnWndProc = viewStorageProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"ViewStorage";
    RegisterClass(&wc);
    viewStorageWnd = CreateWindowEx(0, L"ViewStorage", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, stListWndWidth, 0, clientWidth - stListWndWidth, stListWndHeight, mainWnd, NULL, hInst, NULL);
    CreateWindow(L"Button", L"Lock", WS_CHILD | WS_VISIBLE, 200, 30, 100, 25, viewStorageWnd, (HMENU)LOCK_STORAGE, hInst, nullptr);
    CreateWindow(L"Button", L"Add File", WS_CHILD | WS_VISIBLE, 320, 30, 100, 25, viewStorageWnd, (HMENU)ADD_IN_STORAGE, hInst, nullptr);
    //список файлов хранилища
    fileInStorageListWnd = CreateWindowEx(0, WC_LISTVIEW, NULL, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_NOCOLUMNHEADER | WS_VSCROLL | WS_BORDER,
        38, 100, clientWidth - stListWndWidth - 38*2, clientHeight - 100 - 30,
        viewStorageWnd, NULL, hInst, NULL);
    //создание таблицы для файлов
    LVCOLUMN col;
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    wchar_t columnName[] = L"Files";
    col.pszText = columnName;
    col.cx = clientWidth - stListWndWidth - 38*2;
    ListView_InsertColumn(fileInStorageListWnd, 0, &col);

    //меню для работы с файлами
    fileInStorageFunc = CreatePopupMenu();
    AppendMenu(fileInStorageFunc, MF_STRING, 20, L"Delete");
    AppendMenu(fileInStorageFunc, MF_STRING, 21, L"Export");    

    HFONT hFont = CreateFont(17, 0,0,0, FW_NORMAL,FALSE, FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_QUALITY, L"Arial");
    SendMessage(fileInStorageListWnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void UpdateClientSize() {
    GetClientRect(mainWnd, &clientRect);
    clientWidth = clientRect.right - clientRect.left;
    clientHeight = clientRect.bottom - clientRect.top;
    stListWndHeight = clientHeight;
    stListWndWidth = (int)(0.35 * clientWidth);
}

void ReadEntriesFromFile(const std::wstring& pathsFilename) {
    std::wifstream pathsFile(pathsFilename);
    bool folderExists;
    pathsFile.imbue(std::locale("ru_RU.UTF-8"));
    if (!pathsFile.is_open()) {        
        return;
    }
    if (pathsFile.peek() == std::wifstream::traits_type::eof()) {        
        return;
    }
    std::wstring line;
    while (std::getline(pathsFile, line)) {
        if (line.empty()) {
            continue; 
        }
        std::vector<std::wstring> pathParts;
        std::wistringstream pathStream(line);
        std::wstring part;
        while (std::getline(pathStream, part, L'\\')) {
            pathParts.push_back(part);
        }
        if (pathParts.size() >= 2) {
            std::wstring name = pathParts.back();
            std::wstring folders = L"...\\" + pathParts[pathParts.size() - 2] + L"\\" + name;           
            folderExists = std::filesystem::exists(line) && std::filesystem::is_directory(line);
            if (folderExists == false) 
                StorageDntExist(storages.size());
            else  
                storages.push_back({ line, name, folders, true });
        }
    }
    pathsFile.close();
    std::wofstream outFile(pathsFilename, std::ios::trunc);
    if (!outFile.is_open()) {
        std::wcerr << L"Ошибка открытия файла для записи!" << std::endl;
        return;
    }
    
    outFile.imbue(std::locale(".utf-8"));

    for (const auto& storagePath : storages) {       
        outFile << storagePath.path << std::endl;
    }
    outFile.close();
}

void DrawList(HWND hwnd, HDC hdc) {
    // отрисовка списка доступных хранилищ
    SIZE textSize1, textSize2;    
    int spaceX = 15, spaceY = 13, fontHeight = 16;
    SetTextColor(hdc, RGB(0, 0, 0));
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
    SelectObject(hdc, hPen);
    HBRUSH hBrush = CreateSolidBrush(RGB(128, 128, 128));
    RECT fillRect;

    MoveToEx(hdc, stListWndWidth - 1, 0, nullptr);
    LineTo(hdc, stListWndWidth - 1, stListWndHeight);

    stItmsToShow = storages.size() - scrollPos;
    if (stItmsToShow < 0) stItmsToShow = storages.size();
    if (stItmsToShow > 8) stItmsToShow = 8;

    for (int i = scrollPos; i < stItmsToShow + scrollPos; i++)
    {
        GetTextExtentPoint32(hdc, storages[i].name.c_str(), storages[i].name.size(), &textSize1);
        GetTextExtentPoint32(hdc, storages[i].folders.c_str(), storages[i].folders.size(), &textSize2);
        TextOut(hdc, spaceX, spaceY + (i- scrollPos) *listItmHeight, storages[i].name.c_str(), storages[i].name.size());
        TextOut(hdc, spaceX, spaceY + 21 + (i - scrollPos) * listItmHeight, storages[i].folders.c_str(), storages[i].folders.size());

        MoveToEx(hdc, 0, (i - scrollPos +1)*listItmHeight, nullptr);
        LineTo(hdc, stListWndWidth, (i - scrollPos + 1) * listItmHeight);
    }    
}

void StorageDntExist(int id) {
    std::vector<std::vector<BYTE>> passwords;
    size_t index = 0;
    while (true) {
        std::vector<BYTE> password = LoadPasswordByIndex(L"CPKeys.bin", index);
        if (password.empty()) {
            break;
        }
        passwords.push_back(std::move(password));
        index++;
    }
    passwords.erase(passwords.begin() + id);
    std::ofstream ofs(L"CPKeys.bin", std::ios::binary | std::ios::trunc);
    for (int i = 0;i < passwords.size();i++) {
        SaveToFile(L"CPKeys.bin", passwords[i]);
    }
    ofs.close();
    passwords.clear();
}

void NewStorageText(HDC hdc) {
    TextOut(hdc, 25, 40, L"Path:", 5);
    TextOut(hdc, 25, 120, L"Name:", 5);
    TextOut(hdc, 25, 200, L"Password:", 9);
    TextOut(hdc, 25, 280, L"Repeat password:", 16);
    TextOut(hdc, 25, 420, L"IMPORTANT:If you lose your password,", 36);
    TextOut(hdc, 25, 440, L"you will lose your storage data", 32);
}

void AddNewStorage() {
    //RECT clearRect = { stListWndWidth, 0, clientWidth - stListWndWidth, stListWndHeight };

    int x = GetWindowTextLength(newDirInput);
    if (x == 0) {
        MessageBox(mainWnd, L"Enter path", L"failed", MB_OK);
        return;
    }
    std::wstring directory(x, L'\0');
    GetWindowText(newDirInput, &directory[0], x + 1);

    x = GetWindowTextLength(newNameInput);
    if (x == 0) {
        MessageBox(mainWnd, L"Enter name", L"failed", MB_OK);
        return;
    }    

    if (x > 256 - directory.size()) {
        MessageBox(mainWnd, L"Name is too long", L"failed", MB_OK);
        return;
    }
    std::wstring name(x, L'\0');
    GetWindowText(newNameInput, &name[0], x + 1);
    if (СontainsInvalidChars(name)) {
        MessageBox(mainWnd, L"Name contains invalid chars", L"failed", MB_OK);
        return;
    }

    for (int i = 0;i < storages.size();i++)
    {
        if (storages[i].name == name)
        {
            MessageBox(mainWnd, L"Storage with this name already exists", L"failed", MB_OK);
            return;
        }
    }

    x = GetWindowTextLength(newPassInput);
    if (x > 32) {
        MessageBox(mainWnd, L"Password is too long", L"failed", MB_OK);
        return;
    }
    if (x < 8) {
        MessageBox(mainWnd, L"Password is too short", L"failed", MB_OK);
        return;
    }
    if (x == 0) {
        MessageBox(mainWnd, L"Enter Password", L"failed", MB_OK);
        return;
    }
    std::wstring password(x, L'\0');
    GetWindowText(newPassInput, &password[0], x + 1);

    x = GetWindowTextLength(newPassRepInput);
    if (x <= 0 || x > 22) {
        MessageBox(mainWnd, L"Mismatch", L"failed", MB_OK);
        return;
    }
    std::wstring passwordRep(x, L'\0');
    GetWindowText(newPassRepInput, &passwordRep[0], x + 1);

    if (password.compare(passwordRep) != 0) {
        MessageBox(mainWnd, L"Mismatch", L"failed", MB_OK);
        return;
    }
    std::wstring fullPath = directory + L"\\" + name;
    if (!CreateDirectory(fullPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
        MessageBox(mainWnd, L"Failed to create directory", L"failed", MB_OK);
        return;
    }

    SaveToFile(L"CPKeys.bin", ProtectData(reinterpret_cast<const BYTE*>(password.c_str()), password.size() * sizeof(wchar_t)));

    std::wofstream outFile("CPStorages.txt", std::ios::app);
    if (!outFile.is_open()) {
        MessageBox(mainWnd, L"Failed to open CPStorages.txt for writing", L"failed", MB_OK);
        return;
    }
    outFile.imbue(std::locale(".utf-8"));
    outFile << fullPath << std::endl;
    outFile.close();

    storages.clear();
    ReadEntriesFromFile(L"D:\\БГУИР\\Курсовые\\CryptoPanda\\CryptoPanda\\CPStorages.txt");
}

bool СontainsInvalidChars(const std::wstring& input) {
    static const std::wstring invalidChars = L".,;<>:\"/\\|?*\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";

    for (wchar_t c : input) {
        if (invalidChars.find(c) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

bool CreateFolder(const std::wstring& path, const std::wstring& folderName) {
    std::wstring fullPath = path + L"\\" + folderName;
    DWORD dwAttrib = GetFileAttributes(fullPath.c_str());
    if (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }
    if (!CreateDirectory(fullPath.c_str(), NULL)) {
        return false;
    }
    return true;
}

void SaveToFile(const std::wstring& filename, const std::vector<BYTE>& data) {
    std::ofstream ofs(filename, std::ios::binary | std::ios::app);
    if (ofs.is_open()) {
        DWORD size = static_cast<DWORD>(data.size());
        ofs.write(reinterpret_cast<const char*>(&size), sizeof(size));
        ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
        ofs.close();
    }
}

std::vector<BYTE> ProtectData(const BYTE* data, DWORD dataSize) {
    DATA_BLOB inputBlob = { dataSize, const_cast<BYTE*>(data) };
    DATA_BLOB outputBlob;

    if (CryptProtectData(&inputBlob, NULL, NULL, NULL, NULL, 0, &outputBlob)) {
        std::vector<BYTE> result(outputBlob.pbData, outputBlob.pbData + outputBlob.cbData);
        LocalFree(outputBlob.pbData);
        return result;
    }
    return {};
}

std::vector<BYTE> UnprotectData(const BYTE* data, DWORD dataSize) {
    DATA_BLOB inputBlob = { dataSize, const_cast<BYTE*>(data) };
    DATA_BLOB outputBlob;

    if (CryptUnprotectData(&inputBlob, NULL, NULL, NULL, NULL, 0, &outputBlob)) {
        std::vector<BYTE> result(outputBlob.pbData, outputBlob.pbData + outputBlob.cbData);
        LocalFree(outputBlob.pbData);
        return result;
    }
    return {};
}

std::vector<BYTE> LoadPasswordByIndex(const std::wstring& filename, size_t index) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::wcerr << L"Не удалось открыть файл для чтения." << std::endl;
        return {};
    }

    while (true) {
        DWORD size;
        ifs.read(reinterpret_cast<char*>(&size), sizeof(size));

        if (ifs.eof()) break;

        if (index == 0) {
            std::vector<BYTE> data(size);
            ifs.read(reinterpret_cast<char*>(data.data()), size);
            return data; 
        }
        ifs.ignore(size); 
        index--;
    }
    return {};
}

void SetDialogWnd() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = dialogWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PasswordCheck";
    RegisterClass(&wc);
    dialogWnd = CreateWindowEx(0, L"PasswordCheck", storages[storageID].name.c_str(), WS_BORDER | WS_VISIBLE | WS_OVERLAPPED, clientWidth / 2 - 200, clientHeight / 2 - 100, 400, 150, mainWnd, NULL, hInst, NULL);
    CreateWindow(L"Button", L"OK", WS_CHILD | WS_VISIBLE | WS_BORDER, 20, 80, 150, 25, dialogWnd, (HMENU)PASS_CHECK, hInst, nullptr);
    CreateWindow(L"Button", L"Cancel", WS_CHILD | WS_VISIBLE | WS_BORDER, 210, 80, 150, 25, dialogWnd, (HMENU)PASS_CANCEL, hInst, nullptr);
    passCheckEdit = CreateWindow(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_PASSWORD, 20, 40, 340, 25, dialogWnd, NULL, NULL, NULL);
    ShowWindow(dialogWnd, SW_SHOW);
}

bool ComparePassword(const std::wstring& inputPassword, const std::wstring& filename, size_t index) {
    std::vector<BYTE> loadedPassword = LoadPasswordByIndex(filename, index);
    if (!loadedPassword.empty()) {
        std::vector<BYTE> decryptedPassword = UnprotectData(loadedPassword.data(), loadedPassword.size());
        if (!decryptedPassword.empty()) {
            std::wstring decryptedWPassword = std::wstring(reinterpret_cast<wchar_t*>(decryptedPassword.data()), decryptedPassword.size() / sizeof(wchar_t));
            return (inputPassword == decryptedWPassword);
        }
    }
    return false; 
}

bool CheckPassword() {
    int x = GetWindowTextLength(passCheckEdit);
    if (x > 22) {
        MessageBox(mainWnd, L"Password is to long", L"failed", MB_OK);
        return false;
    }
    if (x == 0) {
        MessageBox(mainWnd, L"Enter Password", L"failed", MB_OK);
        return false;
    }
    std::wstring password(x, L'\0');
    GetWindowText(passCheckEdit, &password[0], x + 1);
    if (ComparePassword(password, L"CPKeys.bin", storageID)) return true;
    else MessageBox(mainWnd, L"Wrong password", L"failed", MB_OK);
    return false;
}

void ViewStorage(HDC hdc) {
    TextOut(hdc, 40, 35, storages[storageID].name.c_str(), storages[storageID].name.size());
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
    SelectObject(hdc, hPen);
    MoveToEx(hdc, 0, 65, nullptr);
    LineTo(hdc, clientWidth,65);
    int height = clientHeight;   
}

//поулчение всех имён файлов из хранилища в расшифрованном виде
void GetFilesInStorage() {
    std::wstring key = GetPassword();
    std::wstring dirPath = storages[storageID].path;
    try {
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(dirPath)) {
            if (std::filesystem::is_regular_file(entry.status())) {
                FileNames filename;
                std::filesystem::path filePath = entry.path(); // Получаем путь к файлу
                std::wstring filenameWithoutExtension = filePath.stem().wstring();
                encoder->SetKey(key);
                filename.encodedName = filenameWithoutExtension;
                std::wstring name = encoder->EncryptLine(filenameWithoutExtension);
                filename.name = name;
                filesInStorage.push_back(filename);
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        MessageBox(viewStorageWnd, L"Can not reade storage", L"Failed", NULL);
    }
}

void SetAddFileWnd()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = addFileWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"AddFileInStorage";
    RegisterClass(&wc);
    addFileInStorageWnd = CreateWindowEx(0, L"AddFileInStorage", storages[storageID].name.c_str(), WS_OVERLAPPED | WS_BORDER | WS_VISIBLE & ~WS_SIZEBOX, clientWidth / 2 - 200, clientHeight / 2 - 100, 600, 200, mainWnd, NULL, hInst, NULL);
    CreateWindow(L"Button", L"OK", WS_CHILD | WS_VISIBLE | WS_BORDER, 20, 125, 235, 25, addFileInStorageWnd, (HMENU)ADD_FILE_OK, hInst, nullptr);
    CreateWindow(L"Button", L"Cancel", WS_CHILD | WS_VISIBLE | WS_BORDER, 325, 125, 235, 25, addFileInStorageWnd, (HMENU)ADD_FILE_CANCEL, hInst, nullptr);
    newFilePathEdit = CreateWindow(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 30, 540, 25, addFileInStorageWnd, NULL, NULL, NULL);
    newFileNameEdit = CreateWindow(L"Edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 85, 540, 25, addFileInStorageWnd, NULL, NULL, NULL);
    ShowWindow(addFileInStorageWnd, SW_SHOW);
}

void AddNewFileInStorage()
{
    int x = GetWindowTextLength(newFilePathEdit);
    if (x == 0)
    {
        MessageBox(mainWnd, L"Enter path", L"failed", MB_OK);
        return;
    }
    std::wstring direcory(x, L'\0');
    GetWindowText(newFilePathEdit, &direcory[0], x + 1);

    x = GetWindowTextLength(newFileNameEdit);
    if (x == 0)
    {
        MessageBox(mainWnd, L"Enter name", L"failed", MB_OK);
        return;
    }
    std::wstring name(x, L'\0');
    GetWindowText(newFileNameEdit, &name[0], x + 1);
    for (int i = 0;i < filesInStorage.size();i++) {
        if (filesInStorage[i].name == name) {
            MessageBox(viewStorageWnd, L"This file name is already in storage.Choose another one", L"Failed", MB_OK);
            return;
        }
    }
    std::wstring key = GetPassword();
    encoder->SetKey(key);    
    std::wstring encodedName = L" ";
    encodedName = encoder->EncryptLine(name);    
    encoder->SetKey(key);
    std::wstring newFilePath = storages[storageID].path + L"\\" + encodedName + L".txt";   
    if (std::filesystem::exists(direcory) && encoder->EncryptDecryptFile(direcory, newFilePath))
    {
        AddItemInFileTable(name);
        filesInStorage.push_back({name, encodedName});
    }
    else MessageBox(viewStorageWnd, L"FilePath doesn't exist", L"Faild", MB_OK);    
}

//обновление параметров скролла
void UpdateScroll() {
    int rowsLeft = storages.size() - 8;
    if (rowsLeft < 0) rowsLeft = 0;
    leftStorages = rowsLeft;
    SetScrollRange(stListWnd, SB_VERT, 0, rowsLeft, true);    
}

// получение мастер-ключа из DPAPI
std::wstring GetPassword() {
    std::vector<BYTE> loadedPassword = LoadPasswordByIndex(L"CPKeys.bin", storageID);
    if (!loadedPassword.empty()) {
        std::vector<BYTE> decryptedPassword = UnprotectData(loadedPassword.data(), loadedPassword.size());
        if (!decryptedPassword.empty()) {
            std::wstring decryptedWPassword = std::wstring(reinterpret_cast<wchar_t*>(decryptedPassword.data()), decryptedPassword.size() / sizeof(wchar_t));
            return decryptedWPassword;
        }
    }
    return L"";
}

void AddItemInFileTable(std::wstring newFile) {
    int i = filesInStorage.size();
    LVITEM item;
    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.iSubItem = 0;
    item.pszText = const_cast<LPWSTR>(newFile.c_str());
    ListView_InsertItem(fileInStorageListWnd, &item);
}

void AddFilesInFileList() {
    for (int i = 0;i < filesInStorage.size();i++) {
        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = i;
        item.iSubItem = 0;        
        item.pszText = const_cast<LPWSTR>(filesInStorage[i].name.c_str());
        ListView_InsertItem(fileInStorageListWnd, &item);
    }
}

void DeleteFilesFromFileTable() {
    ListView_DeleteAllItems(fileInStorageListWnd);
}

//получение координат клика по файлу из хранилища
int GetFileItemClickPos(int x, int y) {         
    for (int i = 0;i < filesInStorage.size();i++) {
        if (y > i * 20 && y < (i + 1) * 20) 
            return i;
    }
    return -1;
}

bool DeleteFileFromStorage(int fileId) {
    std::wstring filePath = storages[storageID].path + L"\\" + filesInStorage[fileId].encodedName + L".txt";

    DWORD attributes = GetFileAttributes(filePath.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributes(filePath.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
    }

    if (DeleteFile(filePath.c_str())) {
        MessageBox(viewStorageWnd, L"File deleted successfully.", L"Info", MB_OK);
        ListView_DeleteItem(fileInStorageListWnd, fileId);
        return true;
    }
    MessageBox(viewStorageWnd, L"Failed to delete the file.", L"Error", MB_OK | MB_ICONERROR);
    SetFileAttributes(filePath.c_str(), attributes);
    return false;
}

bool ExportFileFromStorage(int fileId) {
    std::wstring output = L"D:\\БГУИР\\Курсовые\\CryptoPanda\\Export\\" + filesInStorage[fileId].name;
    std::wstring input = storages[storageID].path + L"\\" + filesInStorage[fileId].encodedName + L".txt";
    std::wstring key = GetPassword();
    encoder->SetKey(key);
    if (encoder->EncryptDecryptFile(input, output)) {
        MessageBox(viewStorageWnd, L"File exported in Export Folder", L"Done", MB_OK);
        return true;
    }
    else MessageBox(viewStorageWnd, L"File wasn't exported", L"Failed", MB_OK);
    return false;
}