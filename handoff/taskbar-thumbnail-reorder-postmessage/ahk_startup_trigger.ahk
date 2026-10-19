; AutoHotkey 1.1 — run at Windows startup after Floorp/Firefox windows are loaded.
; Mod setting hotkeyEnabled is 0; this script does not use the mod's RegisterHotKey.
; Relies on SendMessage + ack to ahk_class WindhawkThumbnailReorderTrigger.

#NoEnv
#SingleInstance Force
SendMode Input
SetWorkingDir %A_ScriptDir%

; Run as Admin
If not A_IsAdmin
{
    Run *RunAs "%A_ScriptFullPath%"
    ExitApp
}

DetectHiddenWindows, On

TriggerReorder() {
    m := DllCall("RegisterWindowMessage", "Str", "Windhawk.TaskbarThumbnailReorder.Trigger")
    Loop, 20 {
        WinWait, ahk_class WindhawkThumbnailReorderTrigger,, 3
        if (!ErrorLevel) {
            SendMessage, %m%, 0, 0,, ahk_class WindhawkThumbnailReorderTrigger,,,, 3000
            if (ErrorLevel != "FAIL" && ErrorLevel+0 = 0x574841)
                return true
        }
        Sleep, 2000
    }
    MsgBox, 48, Windhawk, Not acked (mod loaded?)
}

TriggerReorder()
