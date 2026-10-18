; AutoHotkey 1.1 — run at Windows startup after Floorp/Firefox windows are loaded.
; Mod setting hotkeyEnabled is 0; this script does not use the mod's RegisterHotKey.
; Relies on PostMessage to ahk_class WindhawkThumbnailReorderTrigger.

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

TriggerReorder() {
    triggerMsg := DllCall("RegisterWindowMessage", "Str", "Windhawk.TaskbarThumbnailReorder.Trigger")
    PostMessage, %triggerMsg%, 0, 0,, ahk_class WindhawkThumbnailReorderTrigger
}

; Startup path: caller waits until windows are loaded, then invokes TriggerReorder().
TriggerReorder()
