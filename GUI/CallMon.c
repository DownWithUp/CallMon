#include <Windows.h>
#include <stdio.h>
#include <shlwapi.h>
#include <winres.h>
#include <Uxtheme.h>
#include "Utils.h"
#include "resource.h" // This includes everything from the resource view (forms, elements, configs...)

#pragma comment(linker,"\"/manifestdependency:type='win32' \
                            name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
                            processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(linker, "/ENTRY:WinMain")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Shlwapi.lib")

VOID GUIInit(HWND hwnd)
{
    SetThemeAppProperties(STAP_ALLOW_NONCLIENT | STAP_ALLOW_CONTROLS);
    SetWindowTheme(hwnd, L"Explorer", NULL);
    ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_LIST_MAIN), LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_ONECLICKACTIVATE);
    CListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MAIN), 0, 0, 0, L"PID", 64);
    CListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MAIN), 1, 1, 1, L"Syscall Number", 100);
    CListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MAIN), 2, 2, 2, L"Arg #1", 170);
    CListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MAIN), 3, 3, 3, L"Arg #2", 170);
    CListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MAIN), 4, 4, 4, L"Arg #3", 170);
    CListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST_MAIN), 5, 5, 5, L"Arg #4", 170);
    SetDlgItemTextA(hwnd, IDC_MEMO_STACK, "(Nothing selected)");
}

BOOL WINAPI EventHandler(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    LPNMITEMACTIVATE    pActivatedItem;
    PSTACK_CHUNK        pStackChunk;
    LPNMHDR             pNm;
    DWORD               dwIndex;
    PVOID               pData;
    HWND                hList;
    HWND                hBox;
    HWND                hMemo;
    RECT                rWindow;
    RECT                rList;
    RECT                rBox;
    RECT                rMemo;
    INT                 nWidth; 
    INT                 nHeight;

    switch (Msg) 
    {
    case WM_NOTIFY:
        pNm = (LPNMHDR)lParam;
        if (pNm->code == LVN_ITEMACTIVATE)
        {
            pActivatedItem = (LPNMITEMACTIVATE)lParam;
            if (pGlobalStackMem)
            {
                dwIndex = pActivatedItem->iItem;
                pStackChunk = (PSTACK_CHUNK)((BYTE*)pGlobalStackMem + (dwIndex * sizeof(STACK_CHUNK)));
                pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x1000);
                if (pData)
                {
                    wnsprintfW(pData, 0x1000, L"0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n"
                                            L"0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n0x%I64X\r\n",
                                            pStackChunk->StackData[0], pStackChunk->StackData[1], pStackChunk->StackData[2],
                                            pStackChunk->StackData[3], pStackChunk->StackData[4], pStackChunk->StackData[5],
                                            pStackChunk->StackData[6], pStackChunk->StackData[7], pStackChunk->StackData[8],
                                            pStackChunk->StackData[9], pStackChunk->StackData[10], pStackChunk->StackData[11],
                                            pStackChunk->StackData[12], pStackChunk->StackData[13], pStackChunk->StackData[14],
                                            pStackChunk->StackData[15]);
                    SetDlgItemText(hwnd, IDC_MEMO_STACK, pData);
                    HeapFree(GetProcessHeap(), 0, pData);
                }
            }
            break;
        }  
        return(FALSE);
    case WM_INITDIALOG:
        GUIInit(hwnd);
        return(FALSE);
    case WM_CLOSE:
        DestroyWindow(hwnd);
        PostQuitMessage(0);
        return(FALSE);
    case WM_SIZING:
        rWindow = *((RECT*)lParam);
        if (rWindow.right - rWindow.left <= MIN_WIDTH)
            ((RECT*)lParam)->right = rWindow.left + MIN_WIDTH;

        if (rWindow.bottom - rWindow.top <= MIN_HEIGHT)
            ((RECT*)lParam)->bottom = rWindow.top + MIN_HEIGHT;
        return(FALSE);

    case WM_SYSCOMMAND:
        if (wParam != SC_MAXIMIZE)
        {
            break;
        }
    case WM_SIZE:
        nWidth = (INT)LOWORD(lParam);
        nHeight = (INT)HIWORD(lParam);
        GetClientRect(hwnd, &rWindow);
        if (!bGlobalSizes)
        {
            bGlobalSizes = TRUE;
            RtlSecureZeroMemory(&rList, sizeof(rList));
            GetWindowRect(GetDlgItem(hwnd, IDC_LIST_MAIN), &rList);
            RtlSecureZeroMemory(&rBox, sizeof(rBox));
            GetWindowRect(GetDlgItem(hwnd, IDC_STATIC_BOX), &rBox);
            RtlSecureZeroMemory(&rMemo, sizeof(rMemo));
            GetWindowRect(GetDlgItem(hwnd, IDC_MEMO_STACK), &rMemo);
            dx1 = rWindow.right - (rList.right - rList.left);
            dx2 = rList.bottom - rList.top;
            dx3 = rWindow.right - (rBox.right - rBox.left);
            dx4 = rBox.bottom - rBox.top;
            dx5 = rMemo.right - rMemo.left;
        }
        hList = GetDlgItem(hwnd, IDC_LIST_MAIN);
        if (hList) 
        {
            SetWindowPos(hList, 0, 0, 0, rWindow.right - dx1, nHeight - 99, SWP_NOMOVE | SWP_NOZORDER);
        }
        hBox = GetDlgItem(hwnd, IDC_STATIC_BOX);
        if (hBox)
        {
            SetWindowPos(hBox, 0, 0, 0, rWindow.right - dx3, dx4, SWP_NOMOVE | SWP_NOZORDER);
        }
        hMemo = GetDlgItem(hwnd, IDC_MEMO_STACK);
        if (hMemo)
        {
            SetWindowPos(hMemo, 0, 0, 0, dx5, nHeight - 115, SWP_NOMOVE | SWP_NOZORDER);
        }
        return(FALSE);
    case WM_COMMAND:
        return (CommandHandler(hwnd, wParam, lParam));
    }
    return FALSE;
}

void ShowDialog(HWND Parent) {
    WNDCLASSA   wClass;
    HWND        hDialog;
    MSG         msg;

    RtlSecureZeroMemory(&wClass, sizeof(wClass));
    RtlSecureZeroMemory(&msg, sizeof(msg));
    wClass.lpszClassName = "CallMon_MainForm";
    RegisterClassA(&wClass);
    hDialog = CreateDialogA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(IDD_DIALOG1), Parent, &EventHandler); 
    ShowWindow(hDialog, SWP_NOSIZE | SW_SHOWNORMAL);
    while (GetMessageA(&msg, NULL, 0, 0)) 
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return;
}

DWORD ListenProc(LPVOID lpvParam)
{
    PTOTAL_PACKET   pTotalPacket;
    PSTACK_CHUNK    pStackChunk;
    LPVOID          lpBuff;
    DWORD           dwBytesRead;
    DWORD           dwCount;
    HWND            hwMainWindow;
    HWND            hwList;

    hwMainWindow = FindWindowA("CallMon_MainForm", NULL);
    lpBuff = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TOTAL_PACKET));
    dwCount = 0;
    ConnectNamedPipe((HANDLE)lpvParam, NULL);
    while (TRUE)
    {
        ReadFile((HANDLE)lpvParam, lpBuff, sizeof(TOTAL_PACKET), &dwBytesRead, NULL);
        if (hGlobalHWND)
        {
            pTotalPacket = (PTOTAL_PACKET)lpBuff;

            hwList = GetDlgItem(hGlobalHWND, IDC_LIST_MAIN);
            LV_ITEM lvItem = { 0 };

            lvItem.iItem = dwCount;
            ListView_InsertItem(hwList, &lvItem);

            PVOID pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x100);
            wnsprintfW(pData, 0x100, L"%d", pTotalPacket->CustomHeader.ProcessId);
            ListView_SetItemText(hwList, dwCount, 0, pData);
            HeapFree(GetProcessHeap(), 0, pData);

            pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x100);
            wnsprintfW(pData, 0x100, L"RAX: %I64X", pTotalPacket->Frame.Rax);
            ListView_SetItemText(hwList, dwCount, 1, pData);
            HeapFree(GetProcessHeap(), 0, pData);

            pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x100);
            wnsprintfW(pData, 0x100, L"RCX: %I64X", pTotalPacket->Frame.Rcx);
            ListView_SetItemText(hwList, dwCount, 2, pData);
            HeapFree(GetProcessHeap(), 0, pData);
        
            pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x100);
            wnsprintfW(pData, 0x100, L"RDX: %I64X", pTotalPacket->Frame.Rdx);
            ListView_SetItemText(hwList, dwCount, 3, pData);
            HeapFree(GetProcessHeap(), 0, pData);

            pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x100);
            wnsprintfW(pData, 0x100, L"R8: %I64X", pTotalPacket->Frame.R8);
            ListView_SetItemText(hwList, dwCount, 4, pData);
            HeapFree(GetProcessHeap(), 0, pData);

            pData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x100);
            wnsprintfW(pData, 0x100, L"R9: %I64X", pTotalPacket->Frame.R9);
            ListView_SetItemText(hwList, dwCount, 5, pData);
            HeapFree(GetProcessHeap(), 0, pData);

            pStackChunk = (PSTACK_CHUNK)((BYTE*)pGlobalStackMem + (dwCount * sizeof(STACK_CHUNK)));
            pStackChunk->Row = dwCount;
            memcpy(pStackChunk->StackData, pTotalPacket->CustomHeader.StackData, sizeof(pTotalPacket->CustomHeader.StackData));
            dwCount++;
        }
    }
}

BOOL CommandHandler(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    IsNTAdmin   pIsNTAdmin;
    HANDLE      hDriver;
    WCHAR       wzName[MAX_PATH];
    BOOL        bTranslated;
    INT         nPID;

    switch (LOWORD(wParam)) 
    {
    case IDC_BTN_INIT:
        hGlobalHWND = hwnd;
        pIsNTAdmin = *(IsNTAdmin)GetProcAddress(LoadLibraryA("advpack.dll"), "IsNTAdmin");
        if (pIsNTAdmin)
        {
            if (!pIsNTAdmin(0, NULL))
            {
                GetModuleFileNameW(0, (LPWSTR)&wzName, MAX_PATH * 2);
                if (MessageBoxA(hwnd, "You must be running as an administrator. Restart now?", "Warning", MB_ICONWARNING | MB_YESNO) == IDYES)
                {
                    ShellExecute(hwnd, L"runas", wzName, NULL, NULL, SW_SHOW);
                    ExitProcess(0);
                }
                else
                {
                    return(FALSE);
                }
            }
            else
            {
                hDriver = CreateFileA("AltCall.sys", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
                if (hDriver != INVALID_HANDLE_VALUE)
                {
                    if ((!GetDriverPrivilege()) || (!LoadDriver(hDriver)))
                    {
                        MessageBoxA(hwnd, "Unable to load driver! Ensure it is in the same directory as "
                            "this program and DSE is disabled.", "Error", MB_ICONERROR);
                        CloseHandle(hDriver);
                        return(FALSE);
                       
                    }
                    CloseHandle(hDriver);
                }
                if (!pGlobalStackMem)
                {
                    pGlobalStackMem = VirtualAlloc(0, GLOBAL_STACK_MEM_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                    if (!pGlobalStackMem)
                    {
                        MessageBoxA(hwnd, "Unable to allocate memory for stack capture!", "Fatal Error", MB_ICONERROR);
                        ExitProcess(-1);
                    }
                }
                ObtainDevice();
                CreatePipe();
                if ((hGlobalPipe != INVALID_HANDLE_VALUE) && (hGlobalDriver != INVALID_HANDLE_VALUE))
                {
                    hGlobalListenThread = CreateThread(NULL, 0, &ListenProc, hGlobalPipe, 0, NULL);
                    DeviceIoControl(hGlobalDriver, IOCTL_INIT, NULL, 0, NULL, 0, NULL, NULL);
                }
            }
        }
        return(FALSE);
    case IDC_BTN_ADD:
        nPID = GetDlgItemInt(hwnd, IDC_EDIT_ADD, &bTranslated, FALSE);
        if (bTranslated)
        {
            if (hGlobalDriver != INVALID_HANDLE_VALUE)
            {
                DeviceIoControl(hGlobalDriver, IOCTL_ADD_PROCESS, &nPID, sizeof(nPID), &nPID, sizeof(nPID), NULL, NULL);
                return(FALSE);
            }
        }
        else
        {
            MessageBoxA(hwnd, "Value must be a valid number!", "Error", MB_ICONERROR);
        }
        return(FALSE);
    case IDC_BTN_REMOVE:
        nPID = GetDlgItemInt(hwnd, IDC_EDIT_ADD, &bTranslated, FALSE);
        if (bTranslated)
        {
            if (hGlobalDriver != INVALID_HANDLE_VALUE)
            {
                DeviceIoControl(hGlobalDriver, IOCTL_REMOVE_PROCESS, &nPID, sizeof(nPID), &nPID, sizeof(nPID), NULL, NULL);
                return(FALSE);
            }
        }
        else
        {
            MessageBoxA(hwnd, "Value must be a valid number!", "Error", MB_ICONERROR);
        }
        return(FALSE);
    case IDC_BUTTON_CLEAR:
        if (MessageBoxA(hwnd, "Are you sure you want to clear all entries?", "Confirmation", MB_ICONWARNING | MB_YESNO) == IDYES)
        {
            if (pGlobalStackMem)
            {
                if (hGlobalListenThread != INVALID_HANDLE_VALUE)
                {
                    TerminateThread(hGlobalListenThread, 0);
                    ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_LIST_MAIN));
                    RtlSecureZeroMemory(pGlobalStackMem, GLOBAL_STACK_MEM_SIZE);
                    hGlobalListenThread = CreateThread(NULL, 0, &ListenProc, hGlobalPipe, 0, NULL);
                }
            }
        }
        return(FALSE);
    default:
        return(FALSE);
    }
}

INT CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ShowDialog(0);
    ExitProcess(0);
    return(0);
}
