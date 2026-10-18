; AutoHotkey 1.1 — used at Windows startup (after Floorp/Firefox windows load)
; and rebound to F1 for manual retry.
;
; F1 here is AHK's hotkey, NOT the mod's built-in hotkey
; (mod setting hotkeyEnabled is 0).

#NoEnv
#SingleInstance Force
SendMode Input
SetWorkingDir %A_ScriptDir%

TriggerReorder() {
    triggerMsg := DllCall("RegisterWindowMessage", "Str", "Windhawk.TaskbarThumbnailReorder.Trigger")
    PostMessage, %triggerMsg%, 0, 0,, ahk_class WindhawkThumbnailReorderTrigger
}

; Startup path: caller waits until windows are loaded, then invokes TriggerReorder().
; Manual retry:
F1::TriggerReorder()
