import os
import sys
import time
import subprocess
import ctypes
import traceback
from ctypes import wintypes

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

WM_COMMAND = 0x0111
WM_CLOSE = 0x0010
ID_BTN_SETTINGS = 2018
BN_CLICKED = 0
IDCANCEL = 2

proc = subprocess.Popen(["AWSCleaner.exe"])
time.sleep(2)

def enum_windows_callback(hwnd, lParam):
    try:
        title_buffer = ctypes.create_unicode_buffer(512)
        user32.GetWindowTextW(hwnd, title_buffer, 512)
        if title_buffer.value == "AWS Cleaner":
            ctypes.cast(lParam, ctypes.POINTER(wintypes.HWND))[0] = hwnd
            return False
    except Exception as e:
        pass
    return True

hwndMain = wintypes.HWND(0)
EnumWindowsProc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
user32.EnumWindows(EnumWindowsProc(enum_windows_callback), ctypes.byref(hwndMain))

if hwndMain.value:
    wparam = (BN_CLICKED << 16) | ID_BTN_SETTINGS
    user32.SendMessageW(hwndMain.value, WM_COMMAND, wparam, 0)
    time.sleep(1)
    
    def enum_child_callback(hwnd, lParam):
        owner = user32.GetWindow(hwnd, 4) # GW_OWNER
        if owner == hwndMain.value:
            ctypes.cast(lParam, ctypes.POINTER(wintypes.HWND))[0] = hwnd
            return False
        return True
    
    hwndDlg = wintypes.HWND(0)
    user32.EnumWindows(EnumWindowsProc(enum_child_callback), ctypes.byref(hwndDlg))
    
    if hwndDlg.value:
        print("Sending IDCANCEL to Settings dialog...")
        wparam = (BN_CLICKED << 16) | IDCANCEL
        user32.SendMessageW(hwndDlg.value, WM_COMMAND, wparam, 0)
        time.sleep(2)
        
        proc.poll()
        if proc.returncode is not None:
            print("CRASH DETECTED! Exit code:", hex(proc.returncode & 0xFFFFFFFF))
        else:
            print("No crash. Process is still running.")
            proc.terminate()
    else:
        proc.terminate()
else:
    proc.terminate()
