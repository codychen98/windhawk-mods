// ==WindhawkMod==
// @id              taskbar-thumbnail-reorder-fork
// @name            Taskbar Thumbnail Reorder - Fork
// @description     Reorder taskbar thumbnails manually; optional one-shot auto-reorder from JSON rules on mod load
// @version         1.3.16
// @author          m417z
// @github          https://github.com/m417z
// @twitter         https://twitter.com/m417z
// @homepage        https://m417z.com/
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lcomctl32 -lole32 -loleaut32 -lruntimeobject -lversion -lUIAutomationCore
// ==/WindhawkMod==

// WINDHAWK: Paste this ENTIRE file into the mod editor (mod.wh.cpp).
// Do not use taskbar-thumbnail-reorder.wh.cpp or auto-reorder-*.cpp separately;
// Windhawk only supports a single source file.









// Source code is published under The GNU General Public License v3.0.
//
// For bug reports and feature requests, please open an issue here:
// https://github.com/ramensoftware/windhawk-mods/issues
//
// For pull requests, development takes place here:
// https://github.com/m417z/my-windhawk-mods

// ==WindhawkModReadme==
/*
# Taskbar Thumbnail Reorder

Reorder taskbar thumbnails with the left mouse button. You can also configure a
**one-shot automatic reorder** when the mod loads in Explorer (once per Explorer
session).

Only Windows 10 64-bit and Windows 11 are supported. For older Windows versions
check out [7+ Taskbar Tweaker](https://tweaker.ramensoftware.com/).

![demonstration](https://i.imgur.com/wGGe2RS.gif)

## Manual reorder

Drag thumbnails in the taskbar flyout with the left mouse button. This works
before and after an automatic reorder pass.

## Automatic reorder (optional)

Enable **Auto-reorder when mod loads**, set **Delay before reorder (seconds)**
(e.g. `5` to wait five seconds after the mod initializes), and paste JSON rules
into **Reorder rules (JSON)**.

On each Explorer session:

1. Windhawk injects the mod into `explorer.exe`.
2. After the delay, the mod runs **one** reorder pass (not on every flyout open).
3. The pass scans **all virtual desktops**, finds target app windows on each
   desktop, reads tab titles (UI Automation), applies your rules, and reorders
   taskbar thumbnails **on that desktop only** (windows are not moved between
   desktops).

Changing settings later in the same Explorer session **reloads** the JSON but
does **not** run auto-reorder again until Explorer or the mod is restarted.

### Example JSON (schema version 1)

Copy this into **Reorder rules (JSON)** and adjust strings for your tabs/folders:

```json
{
  "version": 1,
  "autoReorderOnModLoad": true,
  "delayAfterModReadySeconds": 5,
  "apps": [
    {
      "app": "firefox.exe",
      "matches": [
        { "strings": ["gmail", "onenote"], "position": 1 },
        { "strings": ["adidas"], "position": 2 }
      ]
    },
    {
      "app": "explorer.exe",
      "matches": [
        { "strings": ["Projects"], "position": 1 },
        { "strings": ["Downloads"], "position": 2 }
      ]
    }
  ]
}
```

`explorer.exe` here means **File Explorer folder windows**, not the Windows
shell process hosting the taskbar.

### Rule matching

| Topic | Behavior |
|-------|----------|
| `strings` in one rule | **All** strings must appear on **some tab** in the **same** window (partial, case-insensitive). `["gmail","onenote"]` does not match a window with only a gmail tab. |
| Rule order | Rules are sorted by `position` (priority), then tried in that order. |
| Each matched rule | Takes the **next** flyout slot (1, then 2, then 3, …) on that desktop. |
| Unmatched rule | **Skipped** — no empty slot; the next matched rule uses the next number. |
| One window matches several rules | **First matching rule wins** only. |
| Two windows match the same rule | The **first unassigned** window gets that slot; the other goes to the **tail**. |
| Unmatched windows | Keep **relative order** after all matched windows. |

Thumbnails are **one per top-level window**; Firefox "tabs" in rules refer to tabs **inside** that window (enumerated via UI Automation), not separate thumbnails.

### Virtual desktops

Order is computed **per virtual desktop**. The pass includes windows on desktops you
are not currently viewing. Thumbnail positions are relative to the flyout on each
desktop, not one global order across the PC.

### Import / export

Save your JSON to a file for backup or another PC. Paste the same document into
**Reorder rules (JSON)** on another machine; enable auto-reorder and use a similar
delay so restored windows have time to open.

Invalid JSON disables auto-reorder for that session and logs an error (DebugView /
Windhawk log); Explorer is not affected.

### Limitations

- Auto-reorder runs **once** after mod load + delay. Apps that open **much later**
  (e.g. Firefox long after login) are not reordered until Explorer/mod reload.
- Increase the delay if session restore needs more than a few seconds (try 10–15).
- Tab detection depends on UI Automation (Firefox / File Explorer UI); selectors
  may need updates after browser or Windows updates.
- Rules use **executable names** (`firefox.exe`), not window titles alone.
- UI Automation is capped at about **2 seconds per window**; slow targets log a
  timeout and fall back to the window title.
- With **ExplorerPatcher** or the old Windows 11 taskbar option enabled, use the
  same **Customize the old taskbar on Windows 11** setting as for manual reorder.

### Test without restart

Enable **Hotkey trigger** and press **F1** (default) to run reorder immediately.
Enable **Enable logging** and **Show log output** to see tab titles and apply
results. F1 does nothing when the hotkey trigger is disabled.

### External trigger (AutoHotkey, scripts)

When **PostMessage trigger** is enabled, the mod creates a hidden window
(class/title `WindhawkThumbnailReorderTrigger`). Post
`RegisterWindowMessage("Windhawk.TaskbarThumbnailReorder.Trigger")` to that HWND
to run the same pass as F1 (hotkey can be off). See setting description for an
AutoHotkey 1.1 example.

Use **one search string per rule** when each site is in its own Floorp/Firefox
window (e.g. `"YouTube"` alone, not `"YouTube"` + `"Gmail"` in one rule).
Multiple strings in one rule mean **all** must appear on tabs in the **same**
window.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- oldTaskbarOnWin11: false
  $name: Customize the old taskbar on Windows 11
  $description: >-
    Enable this option to customize the old taskbar on Windows 11 (if using
    ExplorerPatcher or a similar tool).
- autoReorderOnModLoad: false
  $name: Auto-reorder when mod loads
  $description: >-
    Run one automatic thumbnail reorder pass per Explorer session after the
    delay below. Does not run again if you change settings until Explorer or
    the mod restarts (rules are re-parsed, but the pass is not repeated).
- delayAfterModReadySeconds: 5
  $name: Delay before reorder (seconds)
  $description: >-
    Wait this many seconds after the mod loads in Explorer before reordering
    (0-120). Gives Firefox, File Explorer, and session restore time to open
    windows. Also used when JSON omits delayAfterModReadySeconds.
- rulesJson: ""
  $name: Reorder rules (JSON)
  $description: >-
    Schema version 1 JSON for target apps and match rules. In Visual mode, paste
    JSON in this field only (multiline is OK). In Textual mode, put the whole
    JSON on one quoted line. Do not use YAML pipe blocks. Invalid JSON disables
    auto-reorder and logs an error. Example (copy and edit):

    {"version":1,"autoReorderOnModLoad":true,"delayAfterModReadySeconds":5,"apps":[{"app":"firefox.exe","matches":[{"strings":["gmail","onenote"],"position":1},{"strings":["adidas"],"position":2}]},{"app":"explorer.exe","matches":[{"strings":["Projects"],"position":1},{"strings":["Downloads"],"position":2}]}]}

    See the mod readme for matching semantics (AND strings per rule, first rule
    wins, per virtual desktop, File Explorer vs shell explorer.exe).
- hotkeyEnabled: true
  $name: Hotkey trigger enabled
  $description: >-
    When enabled, press F1 (or the virtual-key below) to run reorder with the
    current JSON. When disabled, the hotkey does nothing.
- hotkeyVirtualKey: 112
  $name: Hotkey virtual-key code
  $description: >-
    Windows virtual-key code (VK). 112 = F1, 113 = F2, 114 = F3, 115 = F4.
    Change only if F1 conflicts with another app.
- externalPostMessageTriggerEnabled: true
  $name: PostMessage trigger enabled
  $description: >-
    Listen for RegisterWindowMessage("Windhawk.TaskbarThumbnailReorder.Trigger")
    on a hidden window (class WindhawkThumbnailReorderTrigger). Works when the
    hotkey is disabled. AutoHotkey 1.1: triggerMsg := DllCall(
    "RegisterWindowMessage", "Str", "Windhawk.TaskbarThumbnailReorder.Trigger");
    PostMessage, %triggerMsg%, 0, 0,, ahk_class WindhawkThumbnailReorderTrigger
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>


#include <commctrl.h>
#include <psapi.h>

#include <atomic>
#include <cmath>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#undef GetCurrentTime


// ----- auto-reorder-common.h -----
#include <cwctype>
#include <string>
#include <string_view>

namespace AutoReorderCommon {

inline bool EqualsIgnoreCase(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (towlower(a[i]) != towlower(b[i])) {
            return false;
        }
    }
    return true;
}

inline std::wstring ToLowerCopy(std::wstring s) {
    for (wchar_t& c : s) {
        c = static_cast<wchar_t>(towlower(c));
    }
    return s;
}

}  // namespace AutoReorderCommon

// ----- auto-reorder-rules.h -----
#include <string>
#include <vector>

struct AutoReorderMatchRule {
    std::vector<std::wstring> strings;
    int position = 0;
};

struct AutoReorderAppRules {
    std::wstring app;
    std::vector<AutoReorderMatchRule> matches;
};

struct AutoReorderRules {
    int version = 0;
    bool autoReorderOnModLoad = false;
    int delayAfterModReadySeconds = 5;
    std::vector<AutoReorderAppRules> apps;
};

// Parses rulesJson when non-empty; merges UI scalars when JSON omits them.
// Sets validOut false on parse/validation failure (fail-safe: no auto-reorder).
void AutoReorderRules_LoadFromSettings(bool uiAutoReorderOnModLoad,
                                       int uiDelaySeconds,
                                       PCWSTR rulesJsonText,
                                       AutoReorderRules& out,
                                       bool& validOut,
                                       std::wstring& errorOut);

// ----- auto-reorder-enumeration.h -----

#include <guiddef.h>
#include <vector>

// Collects distinct virtual desktop IDs from top-level windows.
std::vector<GUID> EnumerateVirtualDesktops();

// Top-level HWNDs for the given executable on the given virtual desktop.
std::vector<HWND> GetWindowsForAppOnDesktop(const std::wstring& appExe,
                                            const GUID& desktopId);

// Logs per-desktop window counts for each configured app rule.
void AutoReorder_LogEnumeratedTargets(
    const std::vector<AutoReorderAppRules>& appRules);

bool GetWindowVirtualDesktopIdForHwnd(HWND hwnd, GUID& desktopIdOut);

void AutoReorderEnumeration_Uninit();

// ----- auto-reorder-uia.h -----

#include <string>
#include <vector>

// Tab titles via UI Automation; window title as single-element fallback.
std::vector<std::wstring> GetTabTitlesForWindow(HWND hwnd);

bool AutoReorder_ContainsSubstringIgnoreCase(std::wstring_view haystack,
                                             std::wstring_view needle);

// Every required string must appear as a case-insensitive substring of some tab
// title in the same window.
bool AutoReorder_TabTitlesContainAllStrings(
    const std::vector<std::wstring>& tabTitles,
    const std::vector<std::wstring>& requiredStrings);

void AutoReorder_LogTabTitlesForTargets(
    const std::vector<AutoReorderAppRules>& appRules);

void AutoReorderUia_Uninit();

// ----- auto-reorder-engine.h -----

#include <windows.h>

#include <vector>

struct WindowWithTabTitles {
    HWND hwnd = nullptr;
    std::vector<std::wstring> tabTitles;
};

// Greedy rule assignment: rules sorted by `position` (priority). Each matched
// rule takes the next flyout slot (1, 2, 3, ...); unmatched rules are skipped
// with no gap. One window per rule; first-rule-wins; tail keeps stable order.
std::vector<HWND> ComputeOrder(
    const std::vector<WindowWithTabTitles>& windows,
    const std::vector<AutoReorderMatchRule>& matches);

void AutoReorder_ComputeAndLogOrders(
    const std::vector<AutoReorderAppRules>& appRules);

// Logs the manual test matrix from the implementation plan (synthetic tab data).
void AutoReorder_RunEngineSelfTestLog();

// ----- auto-reorder-trace.h / auto-reorder-trace.cpp -----

void AutoReorder_TraceStep(const wchar_t* stepIdAndMessage) {
    Wh_Log(L"Auto-reorder TRACE [tid=%u] %s", GetCurrentThreadId(),
           stepIdAndMessage);
}

void AutoReorder_TraceStepU(const wchar_t* stepIdAndMessage, ULONG_PTR val) {
    Wh_Log(L"Auto-reorder TRACE [tid=%u] %s val=0x%zx",
           GetCurrentThreadId(), stepIdAndMessage, val);
}

void AutoReorder_TraceStep2U(const wchar_t* stepIdAndMessage, ULONG_PTR valA,
                             size_t valB) {
    Wh_Log(L"Auto-reorder TRACE [tid=%u] %s a=0x%zx b=%zu",
           GetCurrentThreadId(), stepIdAndMessage, valA, valB);
}

// ----- auto-reorder-apply.h -----

void* FindTaskGroupViaThumbnailMapping(const std::vector<HWND>& targetHwnds);

bool AutoReorder_ApplyReorders(
    const std::vector<AutoReorderAppRules>& appRules,
    bool flyoutOnlyApply = true);

bool AutoReorder_ApplyFlyoutOnlyFromCache(
    const std::vector<AutoReorderAppRules>& appRules);

bool RunAutoReorderApplyOnTaskbarThread(
    const std::vector<AutoReorderAppRules>& appRules,
    bool fromDeferredTimer = false);

void AutoReorder_UninstallTraySubclass();

extern std::atomic<bool> g_pendingSessionApply;

#include <mutex>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.Xaml.Automation.h>
#include <winrt/Windows.UI.Xaml.Input.h>
#include <winrt/Windows.UI.Xaml.Media.h>

// The taskbar thumbnails use a WinUI 2 (MUX) ItemsRepeater built on top of
// system XAML. Pull in its projection to enumerate the realized items
// (ItemsSourceView / TryGetElement).
#define WH_WINRT_WINUI2
#include <winrt/Microsoft.UI.Xaml.Controls.h>

using namespace winrt::Windows::UI::Xaml;

struct {
    bool oldTaskbarOnWin11;
    bool autoReorderOnModLoad;
    int delayAfterModReadySeconds;
    bool hotkeyEnabled;
    int hotkeyVirtualKey;
    bool externalPostMessageTriggerEnabled;
} g_settings;

AutoReorderRules g_rules;
bool g_rulesValid;

std::atomic<bool> g_autoReorderDone{false};
HANDLE g_autoReorderTimer = nullptr;
HANDLE g_deferredApplyTimer = nullptr;
std::atomic<bool> g_deferredApplyTimerQueued{false};

enum class WinVersion {
    Unsupported,
    Win10,
    Win11,
    Win11_24H2,
};

WinVersion g_winVersion;

std::atomic<bool> g_taskbarViewDllLoaded;
std::atomic<bool> g_initialized;
std::atomic<bool> g_explorerPatcherInitialized;

bool g_noClassicThumbnails;
bool g_hasThumbnailAnimations;

int g_thumbDraggedIndex = -1;
bool g_thumbDragDone;
bool g_taskItemFilterDisallowAll;
DWORD g_taskInclusionChangeLastTickCount;

std::atomic<DWORD> g_getPtr_captureForThreadId;
HDPA g_getPtr_lastHdpa;

// https://www.geoffchappell.com/studies/windows/shell/comctl32/api/da/dpa/dpa.htm
typedef struct _DPA {
    int cpItems;
    PVOID* pArray;
    HANDLE hHeap;
    int cpCapacity;
    int cpGrow;
} DPA, *HDPA;

// XAML thumbnails implementation variables.

struct ThumbnailTaskItemMapping {
    winrt::weak_ref<winrt::Windows::Foundation::IInspectable> thumbnail;
    void* taskGroup;
    void* taskItem;
};

std::vector<ThumbnailTaskItemMapping> g_thumbnailTaskItemMapping;

bool g_inHoverFlyoutModel_TargetItemKey;

winrt::weak_ref<winrt::Windows::Foundation::IInspectable>
    g_TaskGroup_Thumbnails;

void* g_cachedTaskGroupFromFlyout = nullptr;

bool g_reorderingXamlThumbnails;

std::mutex g_autoReorderPassMutex;
std::atomic<bool> g_autoReorderPassRunning{false};

void RefreshCachedTaskGroupFromThumbnailMapping(bool notifyApply = true);
void CancelDeferredAutoReorderApply();
void AutoReorder_QueuePendingFromEngineCache();
bool AutoReorder_FlyoutThumbnailsReady();
bool TryApplyXamlFlyoutOrder(void* taskGroup,
                             const std::vector<HWND>& targetOrder);
void QueuePendingXamlFlyoutOrder(void* taskGroup,
                                 const std::vector<HWND>& targetOrder);
bool AutoReorder_TryApplyPendingXamlFlyoutOrder(void* taskGroup);
void AutoReorder_OnFlyoutMappingRefreshed();
bool AutoReorder_ShouldDeferApply();

FrameworkElement EnumChildElements(
    FrameworkElement element,
    std::function<bool(FrameworkElement)> enumCallback) {
    int childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);

    for (int i = 0; i < childrenCount; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            Wh_Log(L"Failed to get child %d of %d", i + 1, childrenCount);
            continue;
        }

        if (enumCallback(child)) {
            return child;
        }
    }

    return nullptr;
}

FrameworkElement FindChildByName(FrameworkElement element, PCWSTR name) {
    return EnumChildElements(element, [name](FrameworkElement child) {
        return child.Name() == name;
    });
}

FrameworkElement FindChildByClassName(FrameworkElement element,
                                      PCWSTR className) {
    return EnumChildElements(element, [className](FrameworkElement child) {
        return winrt::get_class_name(child) == className;
    });
}

void* QueryViaVtable(void* object, void* vtable) {
    void* ptr = object;
    while (*(void**)ptr != vtable) {
        ptr = (void**)ptr + 1;
    }
    return ptr;
}

void* QueryViaVtableBackwards(void* object, void* vtable) {
    void* ptr = object;
    while (*(void**)ptr != vtable) {
        ptr = (void**)ptr - 1;
    }
    return ptr;
}

void* Query_CTaskListThumbnailWnd_IExtendedUISwitcher(
    void* taskListThumbnailWnd) {
    // This has been correct at least since the first Win10 versions, and is
    // unlikely to ever change.
    return (void**)taskListThumbnailWnd + 2;
}

void* CTaskListWnd_vftable_ITaskListUI;

using CTaskListThumbnailWnd_GetHoverIndex_t = int(WINAPI*)(void* pThis);
CTaskListThumbnailWnd_GetHoverIndex_t CTaskListThumbnailWnd_GetHoverIndex;

using CTaskListThumbnailWnd__GetTaskItem_t = void*(WINAPI*)(void* pThis,
                                                            int index);
CTaskListThumbnailWnd__GetTaskItem_t CTaskListThumbnailWnd__GetTaskItem;

using CTaskListThumbnailWnd_GetTaskGroup_t = void*(WINAPI*)(void* pThis);
CTaskListThumbnailWnd_GetTaskGroup_t CTaskListThumbnailWnd_GetTaskGroup;

using CTaskListThumbnailWnd_TaskReordered_t = void(WINAPI*)(void* pThis,
                                                            void* taskItem);
CTaskListThumbnailWnd_TaskReordered_t CTaskListThumbnailWnd_TaskReordered;

using CTaskGroup_GetNumItems_t = int(WINAPI*)(PVOID pThis);
CTaskGroup_GetNumItems_t CTaskGroup_GetNumItems;

using CTaskListWnd__GetTBGroupFromGroup_t =
    void*(WINAPI*)(void* pThis, void* pTaskGroup, int* pTaskBtnGroupIndex);
CTaskListWnd__GetTBGroupFromGroup_t CTaskListWnd__GetTBGroupFromGroup;

using CTaskBtnGroup_GetGroupType_t = int(WINAPI*)(void* pThis);
CTaskBtnGroup_GetGroupType_t CTaskBtnGroup_GetGroupType;

using CTaskBtnGroup_IndexOfTaskItem_t = int(WINAPI*)(void* pThis,
                                                     void* taskItem);
CTaskBtnGroup_IndexOfTaskItem_t CTaskBtnGroup_IndexOfTaskItem;

using CTaskBtnGroup_GetGroup_t = void*(WINAPI*)(void* pThis);
CTaskBtnGroup_GetGroup_t CTaskBtnGroup_GetGroup;

using CWindowTaskItem_GetWindow_t = HWND(WINAPI*)(void* pThis);
CWindowTaskItem_GetWindow_t CWindowTaskItem_GetWindow;

using CImmersiveTaskItem_GetWindow_t = HWND(WINAPI*)(void* pThis);
CImmersiveTaskItem_GetWindow_t CImmersiveTaskItem_GetWindow;

void* CImmersiveTaskItem_vftable;

using CTaskListWnd_TaskInclusionChanged_t = HRESULT(WINAPI*)(void* pThis,
                                                             void* pTaskGroup,
                                                             void* pTaskItem);
CTaskListWnd_TaskInclusionChanged_t CTaskListWnd_TaskInclusionChanged;

using TaskItemFilter_IsTaskAllowed_t = bool(WINAPI*)(void* pThis,
                                                     void* pTaskItem);
TaskItemFilter_IsTaskAllowed_t TaskItemFilter_IsTaskAllowed_Original;
bool WINAPI TaskItemFilter_IsTaskAllowed_Hook(void* pThis, void* pTaskItem) {
    Wh_Log(L">");

    if (g_taskItemFilterDisallowAll) {
        return false;
    }

    return TaskItemFilter_IsTaskAllowed_Original(pThis, pTaskItem);
}

bool MoveTaskInTaskList(HWND hMMTaskListWnd,
                        void* lpMMTaskListLongPtr,
                        void* taskGroup,
                        void* taskItemFrom,
                        void* taskItemTo) {
    void* taskBtnGroup = CTaskListWnd__GetTBGroupFromGroup(lpMMTaskListLongPtr,
                                                           taskGroup, nullptr);
    if (!taskBtnGroup) {
        return false;
    }

    int taskBtnGroupType = CTaskBtnGroup_GetGroupType(taskBtnGroup);
    if (taskBtnGroupType != 1 && taskBtnGroupType != 3) {
        return false;
    }

    g_getPtr_lastHdpa = nullptr;

    g_getPtr_captureForThreadId = GetCurrentThreadId();
    int indexFrom = CTaskBtnGroup_IndexOfTaskItem(taskBtnGroup, taskItemFrom);
    g_getPtr_captureForThreadId = 0;

    if (indexFrom == -1) {
        return false;
    }

    HDPA buttonsArray = g_getPtr_lastHdpa;
    if (!buttonsArray) {
        return false;
    }

    g_getPtr_lastHdpa = nullptr;

    g_getPtr_captureForThreadId = GetCurrentThreadId();
    int indexTo = CTaskBtnGroup_IndexOfTaskItem(taskBtnGroup, taskItemTo);
    g_getPtr_captureForThreadId = 0;

    if (indexTo == -1) {
        return false;
    }

    if (g_getPtr_lastHdpa != buttonsArray) {
        return false;
    }

    void* button = (void*)DPA_DeletePtr(buttonsArray, indexFrom);
    if (!button) {
        return false;
    }

    DPA_InsertPtr(buttonsArray, indexTo, button);

    if (g_winVersion <= WinVersion::Win10) {
        InvalidateRect(hMMTaskListWnd, nullptr, FALSE);
    } else {
        g_taskInclusionChangeLastTickCount = GetTickCount();

        HWND hThumbnailWnd = GetCapture();
        if (hThumbnailWnd) {
            WCHAR szClassName[32];
            if (GetClassName(hThumbnailWnd, szClassName,
                             ARRAYSIZE(szClassName)) == 0 ||
                _wcsicmp(szClassName, L"TaskListThumbnailWnd") != 0) {
                hThumbnailWnd = nullptr;
            }
        }

        if (hThumbnailWnd) {
            SendMessage(hThumbnailWnd, WM_SETREDRAW, FALSE, 0);
        }

        g_taskItemFilterDisallowAll = true;

        void* pThis_ITaskListUI = QueryViaVtable(
            lpMMTaskListLongPtr, CTaskListWnd_vftable_ITaskListUI);

        CTaskListWnd_TaskInclusionChanged(pThis_ITaskListUI, taskGroup,
                                          taskItemTo);

        g_taskItemFilterDisallowAll = false;

        CTaskListWnd_TaskInclusionChanged(pThis_ITaskListUI, taskGroup,
                                          taskItemTo);

        if (hThumbnailWnd) {
            SendMessage(hThumbnailWnd, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(
                hThumbnailWnd, nullptr, nullptr,
                RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
        }
    }

    return true;
}

HDPA GetTaskItemsArray(void* taskGroup) {
    // This is a horrible hack, but it's the best way I found to get the array
    // of task items from a task group. It relies on the implementation of
    // CTaskGroup::GetNumItems being just this:
    //
    // return DPA_GetPtrCount(this->taskItemsArray);
    //
    // Or in other words:
    //
    // return *(int*)this[taskItemsArrayOffset];
    //
    // Instead of calling it with a real taskGroup object, we call it with an
    // array of pointers to ints. The returned int value is actually the offset
    // to the array member.

    static size_t offset = []() {
        constexpr int kIntArraySize = 256;
        int arrayOfInts[kIntArraySize];
        int* arrayOfIntPtrs[kIntArraySize];
        for (int i = 0; i < kIntArraySize; i++) {
            arrayOfInts[i] = i;
            arrayOfIntPtrs[i] = &arrayOfInts[i];
        }

        return CTaskGroup_GetNumItems(arrayOfIntPtrs);
    }();

    return (HDPA)((void**)taskGroup)[offset];
}

bool MoveTaskInGroup(void* taskGroup, void* taskItemFrom, void* taskItemTo) {
    HDPA taskItemsArray = GetTaskItemsArray(taskGroup);
    if (!taskItemsArray) {
        return false;
    }

    int taskItemsCount = taskItemsArray->cpItems;
    void** taskItems = (void**)taskItemsArray->pArray;

    int indexFrom = -1;
    for (int i = 0; i < taskItemsCount; i++) {
        if (taskItems[i] == taskItemFrom) {
            indexFrom = i;
            break;
        }
    }

    if (indexFrom == -1) {
        return false;
    }

    int indexTo = -1;
    for (int i = 0; i < taskItemsCount; i++) {
        if (taskItems[i] == taskItemTo) {
            indexTo = i;
            break;
        }
    }

    if (indexTo == -1) {
        return false;
    }

    void* taskItemTemp = taskItems[indexFrom];
    if (indexFrom < indexTo) {
        memmove(&taskItems[indexFrom], &taskItems[indexFrom + 1],
                (indexTo - indexFrom) * sizeof(void*));
    } else {
        memmove(&taskItems[indexTo + 1], &taskItems[indexTo],
                (indexFrom - indexTo) * sizeof(void*));
    }
    taskItems[indexTo] = taskItemTemp;

    auto taskbarEnumProc = [taskGroup, taskItemFrom, taskItemTo](
                               HWND hMMTaskbarWnd, bool secondary) {
        HWND hMMTaskSwWnd;
        if (!secondary) {
            hMMTaskSwWnd = (HWND)GetProp(hMMTaskbarWnd, L"TaskbandHWND");
        } else {
            hMMTaskSwWnd =
                FindWindowEx(hMMTaskbarWnd, nullptr, L"WorkerW", nullptr);
        }

        if (!hMMTaskSwWnd) {
            return;
        }

        HWND hMMTaskListWnd =
            FindWindowEx(hMMTaskSwWnd, nullptr, L"MSTaskListWClass", nullptr);
        if (!hMMTaskListWnd) {
            return;
        }

        void* lpMMTaskListLongPtr = (void*)GetWindowLongPtr(hMMTaskListWnd, 0);

        if (!MoveTaskInTaskList(hMMTaskListWnd, lpMMTaskListLongPtr, taskGroup,
                                taskItemFrom, taskItemTo)) {
            Wh_Log(L"Failed to move task item in taskbar %08X",
                   (DWORD)(DWORD_PTR)hMMTaskListWnd);
        }
    };

    EnumThreadWindows(
        GetCurrentThreadId(),
        [](HWND hWnd, LPARAM lParam) -> BOOL {
            WCHAR szClassName[32];
            if (GetClassName(hWnd, szClassName, ARRAYSIZE(szClassName)) == 0) {
                return TRUE;
            }

            bool secondary;
            if (_wcsicmp(szClassName, L"Shell_TrayWnd") == 0) {
                secondary = false;
            } else if (_wcsicmp(szClassName, L"Shell_SecondaryTrayWnd") == 0) {
                secondary = true;
            } else {
                return TRUE;
            }

            auto& proc = *reinterpret_cast<decltype(taskbarEnumProc)*>(lParam);
            proc(hWnd, secondary);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&taskbarEnumProc));

    return true;
}

bool MoveItemsFromThumbnail(void* lpMMThumbnailLongPtr,
                            int indexFrom,
                            int indexTo) {
    Wh_Log(L"> %d->%d", indexFrom, indexTo);

    void* taskItemFrom =
        CTaskListThumbnailWnd__GetTaskItem(lpMMThumbnailLongPtr, indexFrom);
    if (!taskItemFrom) {
        Wh_Log(L"Failed to get task item");
        return false;
    }

    void* taskItemTo =
        CTaskListThumbnailWnd__GetTaskItem(lpMMThumbnailLongPtr, indexTo);
    if (!taskItemTo) {
        Wh_Log(L"Failed to get task item");
        return false;
    }

    if (!MoveTaskInGroup(
            CTaskListThumbnailWnd_GetTaskGroup(lpMMThumbnailLongPtr),
            taskItemFrom, taskItemTo)) {
        Wh_Log(L"Failed to move task item");
        return false;
    }

    CTaskListThumbnailWnd_TaskReordered(lpMMThumbnailLongPtr, taskItemFrom);

    return true;
}

using CTaskListThumbnailWnd_v_WndProc_t = LRESULT(
    WINAPI*)(void* pThis, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
CTaskListThumbnailWnd_v_WndProc_t CTaskListThumbnailWnd_v_WndProc_Original;
LRESULT WINAPI CTaskListThumbnailWnd_v_WndProc_Hook(void* pThis,
                                                    HWND hWnd,
                                                    UINT Msg,
                                                    WPARAM wParam,
                                                    LPARAM lParam) {
    auto OriginalProc = [pThis](HWND hWnd, UINT Msg, WPARAM wParam,
                                LPARAM lParam) {
        return CTaskListThumbnailWnd_v_WndProc_Original(pThis, hWnd, Msg,
                                                        wParam, lParam);
    };

    if (g_noClassicThumbnails) {
        return OriginalProc(hWnd, Msg, wParam, lParam);
    }

    LRESULT result = 0;
    bool processed = false;

    switch (Msg) {
        case WM_LBUTTONDOWN:
            result = OriginalProc(hWnd, Msg, wParam, lParam);

            {
                void* lpMMThumbnailLongPtr = (void*)GetWindowLongPtr(hWnd, 0);
                void* lpMMThumbnailLongPtr_IExtendedUISwitcher =
                    Query_CTaskListThumbnailWnd_IExtendedUISwitcher(
                        lpMMThumbnailLongPtr);
                int trackedIndex = CTaskListThumbnailWnd_GetHoverIndex(
                    lpMMThumbnailLongPtr_IExtendedUISwitcher);
                if (trackedIndex >= 0) {
                    g_thumbDragDone = false;
                    g_thumbDraggedIndex = trackedIndex;
                    SetCapture(hWnd);
                }
            }
            break;

        case WM_MOUSEMOVE:
            result = OriginalProc(hWnd, Msg, wParam, lParam);

            if (GetCapture() == hWnd &&
                GetTickCount() - g_taskInclusionChangeLastTickCount > 60) {
                void* lpMMThumbnailLongPtr = (void*)GetWindowLongPtr(hWnd, 0);
                void* lpMMThumbnailLongPtr_IExtendedUISwitcher =
                    Query_CTaskListThumbnailWnd_IExtendedUISwitcher(
                        lpMMThumbnailLongPtr);
                int trackedIndex = CTaskListThumbnailWnd_GetHoverIndex(
                    lpMMThumbnailLongPtr_IExtendedUISwitcher);
                if (trackedIndex >= 0 && trackedIndex != g_thumbDraggedIndex &&
                    MoveItemsFromThumbnail(lpMMThumbnailLongPtr,
                                           g_thumbDraggedIndex, trackedIndex)) {
                    g_thumbDraggedIndex = trackedIndex;
                    g_thumbDragDone = true;
                }
            }
            break;

        case WM_LBUTTONUP:
            if (GetCapture() == hWnd) {
                ReleaseCapture();
                if (g_thumbDragDone) {
                    result = DefWindowProc(hWnd, Msg, wParam, lParam);
                    processed = true;
                }
            }

            if (!processed) {
                result = OriginalProc(hWnd, Msg, wParam, lParam);
            }
            break;

        case WM_TIMER:
            // Aero peek window of hovered thumbnail.
            if (wParam == 2006) {
                if (GetCapture() == hWnd) {
                    KillTimer(hWnd, wParam);
                    result = DefWindowProc(hWnd, Msg, wParam, lParam);
                    processed = true;
                }
            }

            if (!processed) {
                result = OriginalProc(hWnd, Msg, wParam, lParam);
            }
            break;

        default:
            result = OriginalProc(hWnd, Msg, wParam, lParam);
            break;
    }

    return result;
}

void AddTaskItemThumbnailMapping(
    winrt::Windows::Foundation::IInspectable thumbnail,
    void* taskGroup,
    void* taskItem) {
    // Delete stale mapping items.
    std::erase_if(g_thumbnailTaskItemMapping, [&](const auto& item) {
        auto thumbnailIter = item.thumbnail.get();
        if (!thumbnailIter || thumbnailIter == thumbnail) {
            return true;
        }

        if (item.taskGroup == taskGroup && item.taskItem == taskItem) {
            return true;
        }

        return false;
    });

    g_thumbnailTaskItemMapping.push_back({thumbnail, taskGroup, taskItem});
}

using TaskItemThumbnail_TaskItemThumbnail_t = void*(WINAPI*)(void* param1,
                                                             void* param2,
                                                             void* taskGroup,
                                                             void* taskItem,
                                                             void* taskListUi,
                                                             void* param6,
                                                             void* param7,
                                                             bool param8);
TaskItemThumbnail_TaskItemThumbnail_t
    TaskItemThumbnail_TaskItemThumbnail_Original;
void* WINAPI TaskItemThumbnail_TaskItemThumbnail_Hook(void* param1,
                                                      void* param2,
                                                      void* taskGroup,
                                                      void* taskItem,
                                                      void* taskListUi,
                                                      void* param6,
                                                      void* param7,
                                                      bool param8) {
    Wh_Log(L">");

    void* result = TaskItemThumbnail_TaskItemThumbnail_Original(
        param1, param2, taskGroup, taskItem, taskListUi, param6, param7,
        param8);

    winrt::Windows::Foundation::IInspectable obj = nullptr;
    ((IUnknown*)result + 2)
        ->QueryInterface(
            winrt::guid_of<winrt::Windows::Foundation::IInspectable>(),
            winrt::put_abi(obj));

    AddTaskItemThumbnailMapping(obj, taskGroup, taskItem);

    return result;
}

using TaskItemThumbnail_TaskItemThumbnail_2_t = void*(WINAPI*)(void* param1,
                                                               void* param2,
                                                               void* taskGroup,
                                                               void* taskItem,
                                                               void* taskListUi,
                                                               void* param6,
                                                               bool param7);
TaskItemThumbnail_TaskItemThumbnail_2_t
    TaskItemThumbnail_TaskItemThumbnail_2_Original;
void* WINAPI TaskItemThumbnail_TaskItemThumbnail_2_Hook(void* param1,
                                                        void* param2,
                                                        void* taskGroup,
                                                        void* taskItem,
                                                        void* taskListUi,
                                                        void* param6,
                                                        bool param7) {
    Wh_Log(L">");

    void* result = TaskItemThumbnail_TaskItemThumbnail_2_Original(
        param1, param2, taskGroup, taskItem, taskListUi, param6, param7);

    winrt::Windows::Foundation::IInspectable obj = nullptr;
    ((IUnknown*)result + 2)
        ->QueryInterface(
            winrt::guid_of<winrt::Windows::Foundation::IInspectable>(),
            winrt::put_abi(obj));

    AddTaskItemThumbnailMapping(obj, taskGroup, taskItem);

    return result;
}

using TaskGroup_Thumbnails_t = void*(WINAPI*)(void* pThis, void* param1);
TaskGroup_Thumbnails_t TaskGroup_Thumbnails_Original;
void* WINAPI TaskGroup_Thumbnails_Hook(void* pThis, void* param1) {
    void* result = TaskGroup_Thumbnails_Original(pThis, param1);

    if (g_inHoverFlyoutModel_TargetItemKey) {
        Wh_Log(L">");

        winrt::Windows::Foundation::IInspectable obj = nullptr;
        (*(IUnknown**)result)
            ->QueryInterface(
                winrt::guid_of<winrt::Windows::Foundation::IInspectable>(),
                winrt::put_abi(obj));

        bool assign = false;

        auto currentObj = g_TaskGroup_Thumbnails.get();
        if (!currentObj) {
            Wh_Log(L"No previous thumbnails object");
            assign = true;
        } else if (currentObj != obj) {
            Wh_Log(L"Different previous thumbnails object");
            assign = true;
        } else {
            Wh_Log(L"Same previous thumbnails object");
        }

        if (assign) {
            g_TaskGroup_Thumbnails = obj;

            // Remove invalid weak pointers.
            std::erase_if(g_thumbnailTaskItemMapping, [](const auto& item) {
                return !item.thumbnail.get();
            });

            RefreshCachedTaskGroupFromThumbnailMapping(false);
            bool appliedPending = false;
            if (g_cachedTaskGroupFromFlyout) {
                if (g_pendingSessionApply.load()) {
                    AutoReorder_QueuePendingFromEngineCache();
                }
                appliedPending = AutoReorder_TryApplyPendingXamlFlyoutOrder(
                    g_cachedTaskGroupFromFlyout);
                if (appliedPending) {
                    CancelDeferredAutoReorderApply();
                    g_pendingSessionApply = false;
                }
            }
            if (!appliedPending) {
                AutoReorder_OnFlyoutMappingRefreshed();
            }
        }
    }

    return result;
}

using TaskItemThumbnail_Size_t = int(WINAPI*)(void* pThis);
TaskItemThumbnail_Size_t TaskItemThumbnail_Size_Original;

using TaskItemThumbnail_GetAt_t = void*(WINAPI*)(void* pThis,
                                                 void** result,
                                                 int index);
TaskItemThumbnail_GetAt_t TaskItemThumbnail_GetAt_Original;

using CTaskListWnd_HandleExtendedUIClick_t =
    HRESULT(WINAPI*)(void* pThis,
                     void* taskGroup,
                     void* taskItem,
                     void* launcherOptions);
CTaskListWnd_HandleExtendedUIClick_t
    CTaskListWnd_HandleExtendedUIClick_Original;
HRESULT WINAPI CTaskListWnd_HandleExtendedUIClick_Hook(void* pThis,
                                                       void* taskGroup,
                                                       void* taskItem,
                                                       void* launcherOptions) {
    Wh_Log(L">");

    if (g_reorderingXamlThumbnails) {
        g_reorderingXamlThumbnails = false;
        return S_OK;
    }

    return CTaskListWnd_HandleExtendedUIClick_Original(
        pThis, taskGroup, taskItem, launcherOptions);
}

using HoverFlyoutModel_TargetItemKey_t = void(WINAPI*)(void* pThis,
                                                       void* param1);
HoverFlyoutModel_TargetItemKey_t HoverFlyoutModel_TargetItemKey_Original;
void WINAPI HoverFlyoutModel_TargetItemKey_Hook(void* pThis, void* param1) {
    Wh_Log(L">");

    g_inHoverFlyoutModel_TargetItemKey = true;

    HoverFlyoutModel_TargetItemKey_Original(pThis, param1);

    g_inHoverFlyoutModel_TargetItemKey = false;
}

bool IsPointerInsideElement(const UIElement& element,
                            const Input::PointerRoutedEventArgs& args) {
    auto pointerPos = args.GetCurrentPoint(element).Position();

    float width = element.RenderSize().Width;
    float height = element.RenderSize().Height;

    return pointerPos.X >= 0 && pointerPos.X < width && pointerPos.Y >= 0 &&
           pointerPos.Y < height;
}

void MoveItemsFromXAMLThumbnail(int indexFrom, int indexTo) {
    // New builds (around 10.0.26100.8544) added animations to thumbnails. One
    // side effect is that the previous method of calling
    // CTaskListWnd_TaskInclusionChanged(taskItemFrom) now clears the "Pressed"
    // visual state of the source thumbnail, which breaks the dragging after a
    // single reorder.
    //
    // As a workaround, we now use CTaskListWnd_TaskInclusionChanged(taskItemTo)
    // instead, which repositions the target thumbnail instead of the source
    // thumbnail. This only works if the target thumbnail is adjacent to the
    // source thumbnail, because otherwise all the items between the source and
    // target have to be repositioned, and it's probably not worth the effort.
    int indexDistance = std::abs(indexFrom - indexTo);
    if (indexDistance != 1) {
        Wh_Log(L"Only adjacent thumbnail moves are supported (distance %d)",
               indexDistance);
        return;
    }

    Wh_Log(L"Moving from %d to %d", indexFrom, indexTo);
    AutoReorder_TraceStep2U(L"T77 xaml_move:enter", static_cast<ULONG_PTR>(indexFrom),
                            static_cast<size_t>(indexTo));

    auto thumbnails = g_TaskGroup_Thumbnails.get();
    if (!thumbnails) {
        Wh_Log(L"Thumbnails object isn't valid");
        return;
    }

    auto* thumbnailsPtr = winrt::get_abi(thumbnails);

    int thumbnailsSize = TaskItemThumbnail_Size_Original(&thumbnailsPtr);

    if (indexFrom < 0 || indexFrom >= thumbnailsSize) {
        Wh_Log(L"Invalid indexFrom value");
        return;
    }

    if (indexTo < 0 || indexTo >= thumbnailsSize) {
        Wh_Log(L"Invalid indexTo value");
        return;
    }

    winrt::com_ptr<IUnknown> from;
    TaskItemThumbnail_GetAt_Original(&thumbnailsPtr, from.put_void(),
                                     indexFrom);

    winrt::com_ptr<IUnknown> to;
    TaskItemThumbnail_GetAt_Original(&thumbnailsPtr, to.put_void(), indexTo);

    void* taskItemFrom = nullptr;
    void* taskGroupFrom = nullptr;
    void* taskItemTo = nullptr;
    void* taskGroupTo = nullptr;

    for (const auto& iter : g_thumbnailTaskItemMapping) {
        auto thumbnail = iter.thumbnail.get();
        if (!thumbnail) {
            continue;
        }

        void* thumbnailPtr = winrt::get_abi(thumbnail);

        if (thumbnailPtr == from.get()) {
            taskItemFrom = iter.taskItem;
            taskGroupFrom = iter.taskGroup;
        }

        if (thumbnailPtr == to.get()) {
            taskItemTo = iter.taskItem;
            taskGroupTo = iter.taskGroup;
        }
    }

    if (!taskItemFrom || !taskGroupFrom || !taskItemTo || !taskGroupTo) {
        Wh_Log(L"Task item/group not found");
        return;
    }

    if (taskGroupFrom != taskGroupTo) {
        Wh_Log(L"Task group differs");
        return;
    }

    if (!MoveTaskInGroup(taskGroupFrom, taskItemFrom, taskItemTo)) {
        Wh_Log(L"Failed to move task item");
        return;
    }
}

std::unordered_map<void*, std::vector<HWND>> g_pendingXamlFlyoutOrder;

HWND GetHwndFromTaskItemForXaml(void* taskItem) {
    if (!taskItem) {
        return nullptr;
    }

    if (CImmersiveTaskItem_vftable &&
        *(void**)taskItem == CImmersiveTaskItem_vftable &&
        CImmersiveTaskItem_GetWindow) {
        return CImmersiveTaskItem_GetWindow(taskItem);
    }

    if (CWindowTaskItem_GetWindow) {
        return CWindowTaskItem_GetWindow(taskItem);
    }

    return nullptr;
}

std::vector<HWND> GetFlyoutMappedHwndsForTaskGroup(void* taskGroup) {
    std::vector<HWND> hwnds;
    if (!taskGroup) {
        return hwnds;
    }

    std::unordered_set<HWND> seen;
    for (const ThumbnailTaskItemMapping& mapping : g_thumbnailTaskItemMapping) {
        if (mapping.taskGroup != taskGroup || !mapping.taskItem) {
            continue;
        }

        HWND hwnd = GetHwndFromTaskItemForXaml(mapping.taskItem);
        if (hwnd && seen.insert(hwnd).second) {
            hwnds.push_back(hwnd);
        }
    }

    return hwnds;
}

bool BuildDesiredFlyoutHwndOrder(const std::vector<HWND>& currentHwnds,
                                 const std::vector<HWND>& targetOrder,
                                 std::vector<HWND>& desiredOut) {
    desiredOut.clear();

    for (HWND hwnd : targetOrder) {
        bool inFlyout = false;
        for (HWND current : currentHwnds) {
            if (current == hwnd) {
                inFlyout = true;
                break;
            }
        }
        if (inFlyout) {
            desiredOut.push_back(hwnd);
        }
    }

    for (HWND hwnd : currentHwnds) {
        bool alreadyListed = false;
        for (HWND listed : desiredOut) {
            if (listed == hwnd) {
                alreadyListed = true;
                break;
            }
        }
        if (!alreadyListed) {
            desiredOut.push_back(hwnd);
        }
    }

    return desiredOut.size() == currentHwnds.size();
}

bool TryApplyXamlFlyoutOrder(void* taskGroup,
                             const std::vector<HWND>& targetOrder) {
    AutoReorder_TraceStep2U(L"T70 xaml_apply:enter",
                            reinterpret_cast<ULONG_PTR>(taskGroup),
                            targetOrder.size());
    auto thumbnails = g_TaskGroup_Thumbnails.get();
    if (!thumbnails) {
        AutoReorder_TraceStep(L"T70b xaml_apply:exit no_thumbnails");
        return false;
    }

    auto* thumbnailsPtr = winrt::get_abi(thumbnails);
    int thumbnailsSize = TaskItemThumbnail_Size_Original(&thumbnailsPtr);
    AutoReorder_TraceStep2U(L"T71 xaml_apply:thumbnails_size", thumbnailsSize, 0);
    if (thumbnailsSize <= 1) {
        AutoReorder_TraceStep(L"T71b xaml_apply:exit thumbnails_not_ready");
        return false;
    }

    std::vector<HWND> currentHwnds;
    currentHwnds.reserve(static_cast<size_t>(thumbnailsSize));

    for (int i = 0; i < thumbnailsSize; ++i) {
        winrt::com_ptr<IUnknown> thumb;
        TaskItemThumbnail_GetAt_Original(&thumbnailsPtr, thumb.put_void(), i);

        void* taskItem = nullptr;
        bool found = false;
        for (const ThumbnailTaskItemMapping& mapping : g_thumbnailTaskItemMapping) {
            auto thumbnail = mapping.thumbnail.get();
            if (!thumbnail || mapping.taskGroup != taskGroup) {
                continue;
            }

            if (winrt::get_abi(thumbnail) == thumb.get()) {
                taskItem = mapping.taskItem;
                found = true;
                break;
            }
        }

        if (!found) {
            Wh_Log(L"Auto-reorder apply: flyout preview item %d not mapped", i);
            return false;
        }

        HWND hwnd = GetHwndFromTaskItemForXaml(taskItem);
        if (!hwnd) {
            Wh_Log(L"Auto-reorder apply: no HWND for flyout preview item %d", i);
            return false;
        }

        currentHwnds.push_back(hwnd);
    }

    AutoReorder_TraceStep2U(L"T72 xaml_apply:mapped_current", currentHwnds.size(), 0);

    std::vector<HWND> desiredHwnds;
    if (!BuildDesiredFlyoutHwndOrder(currentHwnds, targetOrder, desiredHwnds)) {
        AutoReorder_TraceStep(L"T72b xaml_apply:exit build_desired_failed");
        return false;
    }

    AutoReorder_TraceStep(L"T73 xaml_apply:built_desired");

    bool alreadyCorrect = true;
    for (size_t i = 0; i < desiredHwnds.size(); ++i) {
        if (desiredHwnds[i] != currentHwnds[i]) {
            alreadyCorrect = false;
            break;
        }
    }

    if (alreadyCorrect) {
        Wh_Log(L"Auto-reorder apply: flyout preview order already correct (%d "
               L"item(s))",
               thumbnailsSize);
        return true;
    }

    AutoReorder_TraceStep(L"T74 xaml_apply:before_move_loop");
    g_reorderingXamlThumbnails = true;
    for (size_t targetPos = 0; targetPos < desiredHwnds.size(); ++targetPos) {
        const HWND want = desiredHwnds[targetPos];
        int currentIdx = -1;
        for (size_t i = 0; i < currentHwnds.size(); ++i) {
            if (currentHwnds[i] == want) {
                currentIdx = static_cast<int>(i);
                break;
            }
        }

        if (currentIdx < 0) {
            g_reorderingXamlThumbnails = false;
            return false;
        }

        while (currentIdx != static_cast<int>(targetPos)) {
            const int step = currentIdx < static_cast<int>(targetPos)
                                 ? currentIdx + 1
                                 : currentIdx - 1;
            MoveItemsFromXAMLThumbnail(currentIdx, step);
            std::swap(currentHwnds[static_cast<size_t>(currentIdx)],
                      currentHwnds[static_cast<size_t>(step)]);
            currentIdx = step;
        }
    }
    g_reorderingXamlThumbnails = false;

    Wh_Log(L"Auto-reorder apply: updated Win11 flyout preview (%d item(s))",
           thumbnailsSize);
    AutoReorder_TraceStep(L"T76 xaml_apply:exit ok");
    return true;
}

void QueuePendingXamlFlyoutOrder(void* taskGroup,
                                 const std::vector<HWND>& targetOrder) {
    if (!taskGroup || targetOrder.size() <= 1) {
        return;
    }

    g_pendingXamlFlyoutOrder[taskGroup] = targetOrder;
    Wh_Log(L"Auto-reorder apply: flyout preview was closed; reorder queued for "
           L"task group %p (%zu window(s); hover the taskbar button to apply)",
           taskGroup, targetOrder.size());
}

bool AutoReorder_TryApplyPendingXamlFlyoutOrder(void* taskGroup) {
    AutoReorder_TraceStepU(L"T78 try_pending:enter",
                           reinterpret_cast<ULONG_PTR>(taskGroup));
    auto it = g_pendingXamlFlyoutOrder.find(taskGroup);
    if (it == g_pendingXamlFlyoutOrder.end()) {
        AutoReorder_TraceStep(L"T78b try_pending:exit not_queued");
        return false;
    }

    const std::vector<HWND> order = it->second;

    AutoReorder_TraceStep(L"T79 try_pending:before_try_xaml");
    if (TryApplyXamlFlyoutOrder(taskGroup, order)) {
        g_pendingXamlFlyoutOrder.erase(it);
        Wh_Log(L"Auto-reorder apply: applied queued flyout preview reorder "
               L"(%zu window(s))",
               order.size());
        return true;
    }

    Wh_Log(L"Auto-reorder apply: queued flyout reorder not ready yet (%zu "
           L"window(s); will retry on next hover)",
           order.size());
    return false;
}

using TaskItemThumbnailList_OnPointerMoved_t = int(WINAPI*)(void* pThis,
                                                            void* pArgs);
TaskItemThumbnailList_OnPointerMoved_t
    TaskItemThumbnailList_OnPointerMoved_Original;
int WINAPI TaskItemThumbnailList_OnPointerMoved_Hook(void* pThis, void* pArgs) {
    auto original = [=]() {
        return TaskItemThumbnailList_OnPointerMoved_Original(pThis, pArgs);
    };

    if (!GetCapture()) {
        g_reorderingXamlThumbnails = false;
        return original();
    }

    if (GetTickCount() - g_taskInclusionChangeLastTickCount <= 60) {
        return original();
    }

    Wh_Log(L">");

    FrameworkElement element = nullptr;
    ((IUnknown*)pThis)
        ->QueryInterface(winrt::guid_of<FrameworkElement>(),
                         winrt::put_abi(element));

    if (!element) {
        return original();
    }

    auto className = winrt::get_class_name(element);
    Wh_Log(L"%s", className.c_str());

    FrameworkElement taskItemThumbnailListRepeater = nullptr;

    if (className == L"Taskbar.TaskItemThumbnailList") {
        taskItemThumbnailListRepeater =
            FindChildByName(element, L"TaskItemThumbnailListRepeater");
    } else if (className == L"Taskbar.TaskItemThumbnailScrollableList") {
        FrameworkElement child = element;
        if ((child = FindChildByName(
                 child, L"TaskItemThumbnailScrollableListScrollViewer")) &&
            (child = FindChildByName(child, L"Root")) &&
            (child = FindChildByClassName(child,
                                          L"Windows.UI.Xaml.Controls.Grid")) &&
            (child = FindChildByName(child, L"ScrollContentPresenter")) &&
            (child =
                 FindChildByName(child, L"TaskItemThumbnailListRepeater"))) {
            taskItemThumbnailListRepeater = child;
        }
    } else {
        return original();
    }

    if (!taskItemThumbnailListRepeater) {
        Wh_Log(L"TaskItemThumbnailListRepeater not found");
        return original();
    }

    auto repeater =
        taskItemThumbnailListRepeater
            .try_as<winrt::Microsoft::UI::Xaml::Controls::ItemsRepeater>();
    if (!repeater) {
        Wh_Log(L"Not an ItemsRepeater");
        return original();
    }

    Input::PointerRoutedEventArgs args = nullptr;
    ((IUnknown*)pArgs)
        ->QueryInterface(winrt::guid_of<Input::PointerRoutedEventArgs>(),
                         winrt::put_abi(args));
    if (!args) {
        return original();
    }

    // Enumerate the repeater's realized items by index instead of walking the
    // visual tree. TryGetElement only returns live items, so ghost elements
    // (e.g. an item being animated out after a reorder) are excluded, and the
    // index is authoritative - unlike AutomationProperties.PositionInSet, which
    // is maintained by the automation peer and isn't refreshed on reorder.
    int indexPressed = -1;
    int indexHovered = -1;

    auto itemsSourceView = repeater.ItemsSourceView();
    int count = itemsSourceView ? itemsSourceView.Count() : 0;

    for (int index = 0; index < count; index++) {
        auto element = repeater.TryGetElement(index);
        if (!element) {
            // Not realized (virtualized away).
            continue;
        }

        auto child = element.try_as<FrameworkElement>();
        if (!child) {
            continue;
        }

        auto className = winrt::get_class_name(child);
        if (className != L"Taskbar.TaskItemThumbnailView") {
            Wh_Log(L"Unexpected element of class %s", className.c_str());
            continue;
        }

        auto grid =
            FindChildByClassName(child, L"Windows.UI.Xaml.Controls.Grid");
        if (!grid) {
            Wh_Log(L"Element has no grid child");
            continue;
        }

        if (indexPressed == -1) {
            VisualState currentState = nullptr;

            for (const auto& v :
                 VisualStateManager::GetVisualStateGroups(grid)) {
                if (v.Name() == L"CommonStates") {
                    currentState = v.CurrentState();
                    break;
                }
            }

            if (currentState) {
                auto currentStateName = currentState.Name();
                if (currentStateName == L"Pressed" ||
                    currentStateName == L"RequestingAttentionPressed") {
                    indexPressed = index;
                    Wh_Log(L"Pressed thumbnail at index %d: %s", indexPressed,
                           Automation::AutomationProperties::GetName(child)
                               .c_str());
                }
            }
        }

        if (indexHovered == -1 && IsPointerInsideElement(child, args)) {
            indexHovered = index;
            Wh_Log(L"Hovered thumbnail at index %d: %s", indexHovered,
                   Automation::AutomationProperties::GetName(child).c_str());
        }

        if (indexPressed != -1 && indexHovered != -1) {
            break;
        }
    }

    if (indexPressed != -1 && indexHovered != -1 &&
        indexPressed != indexHovered) {
        g_reorderingXamlThumbnails = true;
        MoveItemsFromXAMLThumbnail(indexPressed, indexHovered);
    }

    return original();
}

using FlyoutFrame_UpdateFlyoutPosition_t = void(WINAPI*)(void* pThis);
FlyoutFrame_UpdateFlyoutPosition_t FlyoutFrame_UpdateFlyoutPosition_Original;
void WINAPI FlyoutFrame_UpdateFlyoutPosition_Hook(void* pThis) {
    // Newer builds where thumbnail animations are enabled don't need this
    // workaround.
    if (g_hasThumbnailAnimations) {
        FlyoutFrame_UpdateFlyoutPosition_Original(pThis);
        return;
    }

    // Skip the call entirely until the reorder completes. Otherwise, if labels
    // are shown, the thumbnail flyout moves off-screen and disappears. That
    // seems to be a Windows bug, happens with middle-click too.
    if (g_reorderingXamlThumbnails) {
        if (GetCapture()) {
            return;
        }

        g_reorderingXamlThumbnails = false;
    }

    FlyoutFrame_UpdateFlyoutPosition_Original(pThis);
}

using DPA_GetPtr_t = decltype(&DPA_GetPtr);
DPA_GetPtr_t DPA_GetPtr_Original;
PVOID WINAPI DPA_GetPtr_Hook(HDPA hdpa, INT_PTR i) {
    if (g_getPtr_captureForThreadId == GetCurrentThreadId()) {
        g_getPtr_lastHdpa = hdpa;
    }

    return DPA_GetPtr_Original(hdpa, i);
}

bool HookTaskbarSymbols() {
    HMODULE module;
    if (g_winVersion <= WinVersion::Win10) {
        module = GetModuleHandle(nullptr);
    } else {
        module = LoadLibraryEx(L"taskbar.dll", nullptr,
                               LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) {
            Wh_Log(L"Couldn't load taskbar.dll");
            return false;
        }
    }

    // Taskbar.dll, explorer.exe
    WindhawkUtils::SYMBOL_HOOK symbolHooks[] =  //
        {
            {
                {LR"(const CTaskListWnd::`vftable'{for `ITaskListUI'})"},
                &CTaskListWnd_vftable_ITaskListUI,
            },
            {
                {LR"(public: virtual int __cdecl CTaskListThumbnailWnd::GetHoverIndex(void)const )"},
                &CTaskListThumbnailWnd_GetHoverIndex,
                nullptr,
                true,  // Classic thumbs, removed in or near 10.0.26100.8544.
            },
            {
                {
                    // Windows 11.
                    LR"(private: struct ITaskItem * __cdecl CTaskListThumbnailWnd::_GetTaskItem(int)const )",

                    // Windows 10.
                    LR"(private: struct ITaskItem * __cdecl CTaskListThumbnailWnd::_GetTaskItem(int))",
                },
                &CTaskListThumbnailWnd__GetTaskItem,
                nullptr,
                true,  // Classic thumbs, removed in or near 10.0.26100.8544.
            },
            {
                {LR"(public: virtual struct ITaskGroup * __cdecl CTaskListThumbnailWnd::GetTaskGroup(void)const )"},
                &CTaskListThumbnailWnd_GetTaskGroup,
                nullptr,
                true,  // Classic thumbs, removed in or near 10.0.26100.8544.
            },
            {
                {LR"(public: virtual void __cdecl CTaskListThumbnailWnd::TaskReordered(struct ITaskItem *))"},
                &CTaskListThumbnailWnd_TaskReordered,
                nullptr,
                true,  // Classic thumbs, removed in or near 10.0.26100.8544.
            },
            {
                {LR"(public: virtual int __cdecl CTaskGroup::GetNumItems(void))"},
                &CTaskGroup_GetNumItems,
            },
            {
                {LR"(protected: struct ITaskBtnGroup * __cdecl CTaskListWnd::_GetTBGroupFromGroup(struct ITaskGroup *,int *))"},
                &CTaskListWnd__GetTBGroupFromGroup,
            },
            {
                {LR"(public: virtual enum eTBGROUPTYPE __cdecl CTaskBtnGroup::GetGroupType(void))"},
                &CTaskBtnGroup_GetGroupType,
            },
            {
                {LR"(public: virtual int __cdecl CTaskBtnGroup::IndexOfTaskItem(struct ITaskItem *))"},
                &CTaskBtnGroup_IndexOfTaskItem,
            },
            {
                {LR"(public: virtual struct ITaskGroup * __cdecl CTaskBtnGroup::GetGroup(void))"},
                &CTaskBtnGroup_GetGroup,
            },
            {
                {LR"(public: virtual struct HWND__ * __cdecl CWindowTaskItem::GetWindow(void))"},
                &CWindowTaskItem_GetWindow,
            },
            {
                {LR"(public: virtual struct HWND__ * __cdecl CImmersiveTaskItem::GetWindow(void))"},
                &CImmersiveTaskItem_GetWindow,
            },
            {
                {LR"(const CImmersiveTaskItem::`vftable')"},
                &CImmersiveTaskItem_vftable,
            },
            {
                {LR"(public: virtual long __cdecl CTaskListWnd::TaskInclusionChanged(struct ITaskGroup *,struct ITaskItem *))"},
                &CTaskListWnd_TaskInclusionChanged,
            },
            {
                {LR"(public: virtual bool __cdecl TaskItemFilter::IsTaskAllowed(struct ITaskItem *))"},
                &TaskItemFilter_IsTaskAllowed_Original,
                TaskItemFilter_IsTaskAllowed_Hook,
            },
            {
                {LR"(private: virtual __int64 __cdecl CTaskListThumbnailWnd::v_WndProc(struct HWND__ *,unsigned int,unsigned __int64,__int64))"},
                &CTaskListThumbnailWnd_v_WndProc_Original,
                CTaskListThumbnailWnd_v_WndProc_Hook,
                true,  // Classic thumbs, removed in or near 10.0.26100.8544.
            },
            {
                // An older variant, see the newer variant below.
                {LR"(public: __cdecl winrt::WindowsUdk::UI::Shell::implementation::TaskItemThumbnail::TaskItemThumbnail(struct winrt::WindowsUdk::UI::Shell::TaskItem const &,struct ITaskGroup *,struct ITaskItem *,struct ITaskListUI *,struct IWICImagingFactory *,struct ITaskListAcc *,bool))"},
                &TaskItemThumbnail_TaskItemThumbnail_Original,
                TaskItemThumbnail_TaskItemThumbnail_Hook,
                true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
            },
            {
                // A newer variant seen in Windows 11 version 10.0.26100.8328.
                {LR"(public: __cdecl winrt::WindowsUdk::UI::Shell::implementation::TaskItemThumbnail::TaskItemThumbnail(struct winrt::WindowsUdk::UI::Shell::TaskItem const &,struct ITaskGroup *,struct ITaskItem *,struct ITaskListUI *,struct IWICImagingFactory *,bool))"},
                &TaskItemThumbnail_TaskItemThumbnail_2_Original,
                TaskItemThumbnail_TaskItemThumbnail_2_Hook,
                true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
            },
            {
                {LR"(public: struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::WindowsUdk::UI::Shell::TaskItemThumbnail> __cdecl winrt::WindowsUdk::UI::Shell::implementation::TaskGroup::Thumbnails(void))"},
                &TaskGroup_Thumbnails_Original,
                TaskGroup_Thumbnails_Hook,
                true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
            },
            {
                {LR"(public: __cdecl winrt::impl::consume_Windows_Foundation_Collections_IVector<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::WindowsUdk::UI::Shell::TaskItemThumbnail>,struct winrt::WindowsUdk::UI::Shell::TaskItemThumbnail>::Size(void)const )"},
                &TaskItemThumbnail_Size_Original,
                nullptr,
                true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
            },
            {
                {LR"(public: __cdecl winrt::impl::consume_Windows_Foundation_Collections_IVector<struct winrt::Windows::Foundation::Collections::IObservableVector<struct winrt::WindowsUdk::UI::Shell::TaskItemThumbnail>,struct winrt::WindowsUdk::UI::Shell::TaskItemThumbnail>::GetAt(unsigned int)const )"},
                &TaskItemThumbnail_GetAt_Original,
                nullptr,
                true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
            },
            {
                {LR"(public: virtual long __cdecl CTaskListWnd::HandleExtendedUIClick(struct ITaskGroup *,struct ITaskItem *,struct winrt::Windows::System::LauncherOptions const &))"},
                &CTaskListWnd_HandleExtendedUIClick_Original,
                CTaskListWnd_HandleExtendedUIClick_Hook,
                true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
            },
        };

    if (!HookSymbols(module, symbolHooks, ARRAYSIZE(symbolHooks))) {
        Wh_Log(L"HookSymbols failed");
        return false;
    }

    if (!CTaskListThumbnailWnd_GetHoverIndex ||
        !CTaskListThumbnailWnd__GetTaskItem ||
        !CTaskListThumbnailWnd_GetTaskGroup ||
        !CTaskListThumbnailWnd_TaskReordered ||
        !CTaskListThumbnailWnd_v_WndProc_Original) {
        g_noClassicThumbnails = true;
    }

    return true;
}

bool HookTaskbarViewDllSymbols(HMODULE module) {
    // Taskbar.View.dll
    WindhawkUtils::SYMBOL_HOOK symbolHooks[] = {
        {
            {LR"(private: void __cdecl winrt::Taskbar::implementation::HoverFlyoutModel::TargetItemKey(struct winrt::hstring const &))"},
            &HoverFlyoutModel_TargetItemKey_Original,
            HoverFlyoutModel_TargetItemKey_Hook,
            true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
        },
        {
            {LR"(public: virtual int __cdecl winrt::impl::produce<struct winrt::Taskbar::implementation::TaskItemThumbnailList,struct winrt::Windows::UI::Xaml::Controls::IControlOverrides>::OnPointerMoved(void *))"},
            &TaskItemThumbnailList_OnPointerMoved_Original,
            TaskItemThumbnailList_OnPointerMoved_Hook,
            true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
        },
        {
            {LR"(private: void __cdecl winrt::Taskbar::implementation::FlyoutFrame::UpdateFlyoutPosition(void))"},
            &FlyoutFrame_UpdateFlyoutPosition_Original,
            FlyoutFrame_UpdateFlyoutPosition_Hook,
            true,  // New XAML thumbnails, enabled in late Windows 11 24H2.
        },
    };

    if (!HookSymbols(module, symbolHooks, ARRAYSIZE(symbolHooks))) {
        Wh_Log(L"HookSymbols failed");
        return false;
    }

    return true;
}

HMODULE GetTaskbarViewModuleHandle() {
    HMODULE module = GetModuleHandle(L"Taskbar.View.dll");
    if (!module) {
        module = GetModuleHandle(L"ExplorerExtensions.dll");
    }

    return module;
}

void HandleLoadedModuleIfTaskbarView(HMODULE module, LPCWSTR lpLibFileName) {
    if (g_winVersion >= WinVersion::Win11 && !g_taskbarViewDllLoaded &&
        GetTaskbarViewModuleHandle() == module &&
        !g_taskbarViewDllLoaded.exchange(true)) {
        Wh_Log(L"Loaded %s", lpLibFileName);

        if (HookTaskbarViewDllSymbols(module)) {
            Wh_ApplyHookOperations();
        }
    }
}

VS_FIXEDFILEINFO* GetModuleVersionInfo(HMODULE hModule, UINT* puPtrLen) {
    void* pFixedFileInfo = nullptr;
    UINT uPtrLen = 0;

    HRSRC hResource =
        FindResource(hModule, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    if (hResource) {
        HGLOBAL hGlobal = LoadResource(hModule, hResource);
        if (hGlobal) {
            void* pData = LockResource(hGlobal);
            if (pData) {
                if (!VerQueryValue(pData, L"\\", &pFixedFileInfo, &uPtrLen) ||
                    uPtrLen == 0) {
                    pFixedFileInfo = nullptr;
                    uPtrLen = 0;
                }
            }
        }
    }

    if (puPtrLen) {
        *puPtrLen = uPtrLen;
    }

    return (VS_FIXEDFILEINFO*)pFixedFileInfo;
}

WinVersion GetExplorerVersion() {
    VS_FIXEDFILEINFO* fixedFileInfo = GetModuleVersionInfo(nullptr, nullptr);
    if (!fixedFileInfo) {
        return WinVersion::Unsupported;
    }

    WORD major = HIWORD(fixedFileInfo->dwFileVersionMS);
    WORD minor = LOWORD(fixedFileInfo->dwFileVersionMS);
    WORD build = HIWORD(fixedFileInfo->dwFileVersionLS);
    WORD qfe = LOWORD(fixedFileInfo->dwFileVersionLS);

    Wh_Log(L"Version: %u.%u.%u.%u", major, minor, build, qfe);

    switch (major) {
        case 10:
            if (build < 22000) {
                return WinVersion::Win10;
            } else if (build < 26100) {
                return WinVersion::Win11;
            } else {
                return WinVersion::Win11_24H2;
            }
            break;
    }

    return WinVersion::Unsupported;
}

struct EXPLORER_PATCHER_HOOK {
    PCSTR symbol;
    void** pOriginalFunction;
    void* hookFunction = nullptr;
    bool optional = false;

    template <typename Prototype>
    EXPLORER_PATCHER_HOOK(
        PCSTR symbol,
        Prototype** originalFunction,
        std::type_identity_t<Prototype*> hookFunction = nullptr,
        bool optional = false)
        : symbol(symbol),
          pOriginalFunction(reinterpret_cast<void**>(originalFunction)),
          hookFunction(reinterpret_cast<void*>(hookFunction)),
          optional(optional) {}
};

bool HookExplorerPatcherSymbols(HMODULE explorerPatcherModule) {
    if (g_explorerPatcherInitialized.exchange(true)) {
        return true;
    }

    if (g_winVersion >= WinVersion::Win11) {
        g_winVersion = WinVersion::Win10;
    }

    EXPLORER_PATCHER_HOOK hooks[] = {
        {R"(??_7CTaskListWnd@@6BITaskListUI@@@)",
         &CTaskListWnd_vftable_ITaskListUI},
        {R"(?GetHoverIndex@CTaskListThumbnailWnd@@UEBAHXZ)",
         &CTaskListThumbnailWnd_GetHoverIndex},
        {R"(?_GetTaskItem@CTaskListThumbnailWnd@@AEAAPEAUITaskItem@@H@Z)",
         &CTaskListThumbnailWnd__GetTaskItem},
        {R"(?GetTaskGroup@CTaskListThumbnailWnd@@UEBAPEAUITaskGroup@@XZ)",
         &CTaskListThumbnailWnd_GetTaskGroup},
        {R"(?TaskReordered@CTaskListThumbnailWnd@@UEAAXPEAUITaskItem@@@Z)",
         &CTaskListThumbnailWnd_TaskReordered},
        {R"(?GetNumItems@CTaskGroup@@UEAAHXZ)", &CTaskGroup_GetNumItems},
        {R"(?_GetTBGroupFromGroup@CTaskListWnd@@IEAAPEAUITaskBtnGroup@@PEAUITaskGroup@@PEAH@Z)",
         &CTaskListWnd__GetTBGroupFromGroup},
        {R"(?GetGroupType@CTaskBtnGroup@@UEAA?AW4eTBGROUPTYPE@@XZ)",
         &CTaskBtnGroup_GetGroupType},
        {R"(?IndexOfTaskItem@CTaskBtnGroup@@UEAAHPEAUITaskItem@@@Z)",
         &CTaskBtnGroup_IndexOfTaskItem},
        {R"(?GetGroup@CTaskBtnGroup@@UEAAPEAUITaskGroup@@XZ)",
         &CTaskBtnGroup_GetGroup},
        {R"(?GetWindow@CWindowTaskItem@@UEAAPEAUHWND__@@XZ)",
         &CWindowTaskItem_GetWindow},
        {R"(?GetWindow@CImmersiveTaskItem@@UEAAPEAUHWND__@@XZ)",
         &CImmersiveTaskItem_GetWindow},
        {R"(??_7CImmersiveTaskItem@@6B@)", &CImmersiveTaskItem_vftable},
        {R"(?TaskInclusionChanged@CTaskListWnd@@UEAAJPEAUITaskGroup@@PEAUITaskItem@@@Z)",
         &CTaskListWnd_TaskInclusionChanged},
        {R"(?IsTaskAllowed@TaskItemFilter@@UEAA_NPEAUITaskItem@@@Z)",
         &TaskItemFilter_IsTaskAllowed_Original,
         TaskItemFilter_IsTaskAllowed_Hook},
        {R"(?v_WndProc@CTaskListThumbnailWnd@@EEAA_JPEAUHWND__@@I_K_J@Z)",
         &CTaskListThumbnailWnd_v_WndProc_Original,
         CTaskListThumbnailWnd_v_WndProc_Hook},
    };

    bool succeeded = true;

    for (const auto& hook : hooks) {
        void* ptr = (void*)GetProcAddress(explorerPatcherModule, hook.symbol);
        if (!ptr) {
            Wh_Log(L"ExplorerPatcher symbol%s doesn't exist: %S",
                   hook.optional ? L" (optional)" : L"", hook.symbol);
            if (!hook.optional) {
                succeeded = false;
            }
            continue;
        }

        if (hook.hookFunction) {
            Wh_SetFunctionHook(ptr, hook.hookFunction, hook.pOriginalFunction);
        } else {
            *hook.pOriginalFunction = ptr;
        }
    }

    if (!succeeded) {
        Wh_Log(L"HookExplorerPatcherSymbols failed");
    } else if (g_initialized) {
        Wh_ApplyHookOperations();
    }

    return succeeded;
}

bool IsExplorerPatcherModule(HMODULE module) {
    WCHAR moduleFilePath[MAX_PATH];
    switch (
        GetModuleFileName(module, moduleFilePath, ARRAYSIZE(moduleFilePath))) {
        case 0:
        case ARRAYSIZE(moduleFilePath):
            return false;
    }

    PCWSTR moduleFileName = wcsrchr(moduleFilePath, L'\\');
    if (!moduleFileName) {
        return false;
    }

    moduleFileName++;

    if (_wcsnicmp(L"ep_taskbar.", moduleFileName, sizeof("ep_taskbar.") - 1) ==
        0) {
        Wh_Log(L"ExplorerPatcher taskbar module: %s", moduleFileName);
        return true;
    }

    return false;
}

bool HandleLoadedExplorerPatcher() {
    HMODULE hMods[1024];
    DWORD cbNeeded;
    if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods),
                           &cbNeeded)) {
        for (size_t i = 0; i < cbNeeded / sizeof(HMODULE); i++) {
            if (IsExplorerPatcherModule(hMods[i])) {
                return HookExplorerPatcherSymbols(hMods[i]);
            }
        }
    }

    return true;
}

void HandleLoadedModuleIfExplorerPatcher(HMODULE module) {
    if (module && !((ULONG_PTR)module & 3) && !g_explorerPatcherInitialized) {
        if (IsExplorerPatcherModule(module)) {
            HookExplorerPatcherSymbols(module);
        }
    }
}

using LoadLibraryExW_t = decltype(&LoadLibraryExW);
LoadLibraryExW_t LoadLibraryExW_Original;
HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR lpLibFileName,
                                   HANDLE hFile,
                                   DWORD dwFlags) {
    HMODULE module = LoadLibraryExW_Original(lpLibFileName, hFile, dwFlags);
    if (module) {
        HandleLoadedModuleIfExplorerPatcher(module);
        HandleLoadedModuleIfTaskbarView(module, lpLibFileName);
    }

    return module;
}

void LoadSettings() {
    g_settings.oldTaskbarOnWin11 = Wh_GetIntSetting(L"oldTaskbarOnWin11");
    g_settings.autoReorderOnModLoad =
        Wh_GetIntSetting(L"autoReorderOnModLoad") != 0;
    g_settings.delayAfterModReadySeconds =
        Wh_GetIntSetting(L"delayAfterModReadySeconds");
    g_settings.hotkeyEnabled = Wh_GetIntSetting(L"hotkeyEnabled") != 0;
    g_settings.externalPostMessageTriggerEnabled =
        Wh_GetIntSetting(L"externalPostMessageTriggerEnabled") != 0;
    g_settings.hotkeyVirtualKey = Wh_GetIntSetting(L"hotkeyVirtualKey");
    if (g_settings.hotkeyVirtualKey < 1) {
        g_settings.hotkeyVirtualKey = VK_F1;
    }

    PCWSTR rulesJson = Wh_GetStringSetting(L"rulesJson");
    std::wstring parseError;
    AutoReorderRules_LoadFromSettings(
        g_settings.autoReorderOnModLoad,
        g_settings.delayAfterModReadySeconds, rulesJson, g_rules, g_rulesValid,
        parseError);
    Wh_FreeStringSetting(rulesJson);

    if (!g_rulesValid) {
        Wh_Log(L"Auto-reorder rules invalid, auto-reorder disabled: %s",
               parseError.c_str());
        g_settings.autoReorderOnModLoad = false;
        return;
    }

    g_settings.autoReorderOnModLoad = g_rules.autoReorderOnModLoad;
    g_settings.delayAfterModReadySeconds = g_rules.delayAfterModReadySeconds;

    Wh_Log(L"Auto-reorder rules loaded: %zu apps, enabled=%d, delay=%d s",
           g_rules.apps.size(), g_settings.autoReorderOnModLoad ? 1 : 0,
           g_settings.delayAfterModReadySeconds);
}

void RunAutoReorder() {
    Wh_Log(L"RunAutoReorder: %zu app rule(s), oldTaskbarOnWin11=%d, "
           L"explorerPatcher=%d",
           g_rules.apps.size(), g_settings.oldTaskbarOnWin11 ? 1 : 0,
           g_explorerPatcherInitialized.load() ? 1 : 0);
    AutoReorder_RunEngineSelfTestLog();
    AutoReorder_ComputeAndLogOrders(g_rules.apps);
    RunAutoReorderApplyOnTaskbarThread(g_rules.apps);
}

void RunAutoReorderTestPass() {
    if (!g_rulesValid) {
        Wh_Log(L"Auto-reorder test: rules invalid or JSON parse failed");
        return;
    }

    if (g_rules.apps.empty()) {
        Wh_Log(L"Auto-reorder test: no apps in rules (empty apps array)");
        return;
    }

    if (!g_initialized.load()) {
        Wh_Log(L"Auto-reorder test: mod not initialized yet");
        return;
    }

    Wh_Log(L"Auto-reorder: manual pass (hotkey)");
    AutoReorder_TraceStep(L"T05 hotkey:before_engine");
    AutoReorder_ComputeAndLogOrders(g_rules.apps);
    AutoReorder_TraceStep(L"T06 hotkey:after_engine");
    AutoReorder_TraceStep(L"T07 hotkey:before_run_apply");
    RunAutoReorderApplyOnTaskbarThread(g_rules.apps);
    AutoReorder_TraceStep(L"T08 hotkey:after_run_apply");
}

constexpr int kReorderHotkeyId = 1;
constexpr wchar_t kReorderTriggerWindowClass[] =
    L"WindhawkThumbnailReorderTrigger";
constexpr wchar_t kReorderTriggerRegisteredMessageName[] =
    L"Windhawk.TaskbarThumbnailReorder.Trigger";

std::atomic<bool> g_reorderListenerThreadStop{false};
HANDLE g_reorderListenerThread = nullptr;
DWORD g_reorderListenerThreadId = 0;
UINT g_reorderTriggerRegisteredMessage = 0;
HWND g_reorderTriggerHwnd = nullptr;

void HandleManualReorderTrigger(const wchar_t* source) {
    Wh_Log(L"Auto-reorder: trigger from %s", source);
    RunAutoReorderTestPass();
}

LRESULT CALLBACK ReorderTriggerWndProc(HWND hwnd,
                                     UINT msg,
                                     WPARAM wParam,
                                     LPARAM lParam) {
    if (g_reorderTriggerRegisteredMessage != 0 &&
        msg == g_reorderTriggerRegisteredMessage) {
        HandleManualReorderTrigger(L"PostMessage");
        return 0;
    }

    if (msg == WM_HOTKEY && wParam == kReorderHotkeyId) {
        HandleManualReorderTrigger(L"hotkey");
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool EnsureReorderTriggerWindowClassRegistered() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = ReorderTriggerWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = kReorderTriggerWindowClass;

    if (!RegisterClassExW(&wc)) {
        const DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            Wh_Log(L"Auto-reorder listener: RegisterClassExW failed (%u)", err);
            return false;
        }
    }

    return true;
}

DWORD WINAPI ReorderListenerThreadProc(LPVOID param) {
    const UINT virtualKey = static_cast<UINT>(
        reinterpret_cast<ULONG_PTR>(param));

    MSG msg;
    PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);

    if (!EnsureReorderTriggerWindowClassRegistered()) {
        return 0;
    }

    g_reorderTriggerRegisteredMessage = RegisterWindowMessageW(
        kReorderTriggerRegisteredMessageName);
    if (g_reorderTriggerRegisteredMessage == 0) {
        Wh_Log(L"Auto-reorder listener: RegisterWindowMessage failed (%u)",
               GetLastError());
        return 0;
    }

    // Hidden popup (not HWND_MESSAGE) so AutoHotkey can target ahk_class.
    g_reorderTriggerHwnd = CreateWindowExW(
        WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW, kReorderTriggerWindowClass,
        kReorderTriggerWindowClass, WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
        GetModuleHandle(nullptr), nullptr);
    if (!g_reorderTriggerHwnd) {
        Wh_Log(L"Auto-reorder listener: CreateWindowExW failed (%u)",
               GetLastError());
        g_reorderTriggerRegisteredMessage = 0;
        return 0;
    }

    ShowWindow(g_reorderTriggerHwnd, SW_HIDE);

    Wh_Log(L"Auto-reorder listener: PostMessage window ready (class=%s, msg=%s, "
           L"id=0x%X)",
           kReorderTriggerWindowClass, kReorderTriggerRegisteredMessageName,
           g_reorderTriggerRegisteredMessage);

    bool hotkeyRegistered = false;
    if (virtualKey != 0) {
        hotkeyRegistered =
            RegisterHotKey(g_reorderTriggerHwnd, kReorderHotkeyId, MOD_NOREPEAT,
                           virtualKey) != FALSE;
        if (!hotkeyRegistered) {
            Wh_Log(L"Auto-reorder hotkey: RegisterHotKey failed (%u) for VK=%u",
                   GetLastError(), virtualKey);
        } else {
            Wh_Log(L"Auto-reorder hotkey: registered VK=0x%X (%u)", virtualKey,
                   virtualKey);
        }
    }

    while (!g_reorderListenerThreadStop.load()) {
        const BOOL result = GetMessageW(&msg, nullptr, 0, 0);
        if (result <= 0) {
            break;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hotkeyRegistered) {
        UnregisterHotKey(g_reorderTriggerHwnd, kReorderHotkeyId);
    }

    if (g_reorderTriggerHwnd) {
        DestroyWindow(g_reorderTriggerHwnd);
        g_reorderTriggerHwnd = nullptr;
    }

    g_reorderTriggerRegisteredMessage = 0;
    Wh_Log(L"Auto-reorder listener: thread exiting");
    return 0;
}

void StopReorderListenerThread() {
    HANDLE thread = g_reorderListenerThread;
    if (!thread) {
        return;
    }

    g_reorderListenerThreadStop = true;

    const DWORD threadId = g_reorderListenerThreadId;
    if (threadId != 0) {
        PostThreadMessage(threadId, WM_QUIT, 0, 0);
    }

    WaitForSingleObject(thread, 5000);
    CloseHandle(thread);

    g_reorderListenerThread = nullptr;
    g_reorderListenerThreadId = 0;
    g_reorderListenerThreadStop = false;
}

void UpdateReorderListener() {
    StopReorderListenerThread();

    if (!g_initialized.load()) {
        return;
    }

    const bool wantHotkey = g_settings.hotkeyEnabled;
    const bool wantPostMessage = g_settings.externalPostMessageTriggerEnabled;
    if (!wantHotkey && !wantPostMessage) {
        return;
    }

    UINT virtualKey = 0;
    if (wantHotkey) {
        virtualKey = static_cast<UINT>(g_settings.hotkeyVirtualKey);
        if (virtualKey == 0) {
            Wh_Log(L"Auto-reorder hotkey: virtual key 0 is invalid");
            if (!wantPostMessage) {
                return;
            }
            virtualKey = 0;
        }
    }

    g_reorderListenerThreadStop = false;
    g_reorderListenerThread =
        CreateThread(nullptr, 0, ReorderListenerThreadProc,
                     reinterpret_cast<LPVOID>(
                         static_cast<ULONG_PTR>(virtualKey)),
                     0, &g_reorderListenerThreadId);
    if (!g_reorderListenerThread) {
        Wh_Log(L"Auto-reorder listener: CreateThread failed (%u)",
               GetLastError());
        g_reorderListenerThreadId = 0;
        return;
    }

    Wh_Log(L"Auto-reorder listener: thread started (hotkey=%d VK=0x%X, "
           L"PostMessage=%d)",
           wantHotkey ? 1 : 0, virtualKey, wantPostMessage ? 1 : 0);
}

void CALLBACK AutoReorderTimerCallback(PVOID /*lpParameter*/,
                                       BOOLEAN /*timerOrWaitFired*/) {
    g_autoReorderTimer = nullptr;

    if (g_autoReorderDone.exchange(true)) {
        Wh_Log(L"Auto-reorder: timer fired but session pass already done");
        return;
    }

    Wh_Log(L"Auto-reorder: timer fired, starting session pass");
    RunAutoReorder();
}

void CancelAutoReorderTimer() {
    HANDLE timer = g_autoReorderTimer;
    if (!timer) {
        return;
    }

    g_autoReorderTimer = nullptr;
    if (!DeleteTimerQueueTimer(nullptr, timer, INVALID_HANDLE_VALUE)) {
        Wh_Log(L"Auto-reorder: DeleteTimerQueueTimer failed (%u)",
               GetLastError());
    }
}

void CALLBACK DeferredApplyTimerCallback(PVOID /*lpParameter*/,
                                           BOOLEAN /*timerOrWaitFired*/) {
    AutoReorder_TraceStep(L"T01 deferred_timer:enter");
    g_deferredApplyTimer = nullptr;
    g_deferredApplyTimerQueued = false;

    if (!g_rulesValid || g_rules.apps.empty()) {
        AutoReorder_TraceStep(L"T01b deferred_timer:exit rules_invalid");
        return;
    }

    Wh_Log(L"Auto-reorder apply: deferred timer fired (off flyout hook stack)");
    AutoReorder_TraceStep(L"T02 deferred_timer:before_run_apply");
    RunAutoReorderApplyOnTaskbarThread(g_rules.apps, true);
    AutoReorder_TraceStep(L"T03 deferred_timer:after_run_apply");
}

void CancelDeferredApplyTimer() {
    HANDLE timer = g_deferredApplyTimer;
    if (!timer) {
        g_deferredApplyTimerQueued = false;
        return;
    }

    g_deferredApplyTimer = nullptr;
    g_deferredApplyTimerQueued = false;
    if (!DeleteTimerQueueTimer(nullptr, timer, INVALID_HANDLE_VALUE)) {
        Wh_Log(L"Auto-reorder: CancelDeferredApplyTimer failed (%u)",
               GetLastError());
    }
}

void CancelDeferredAutoReorderApply() {
    CancelDeferredApplyTimer();
}

void ScheduleDeferredAutoReorderApply() {
    if (g_deferredApplyTimerQueued.exchange(true)) {
        return;
    }

    constexpr DWORD kDeferredApplyDelayMs = 200;

    if (!CreateTimerQueueTimer(&g_deferredApplyTimer, nullptr,
                               DeferredApplyTimerCallback, nullptr,
                               kDeferredApplyDelayMs, 0,
                               WT_EXECUTEONLYONCE)) {
        g_deferredApplyTimerQueued = false;
        Wh_Log(L"Auto-reorder apply: ScheduleDeferredApply failed (%u)",
               GetLastError());
    }
}

void StartAutoReorderTimerIfNeeded() {
    if (!g_settings.autoReorderOnModLoad || !g_rulesValid) {
        return;
    }

    if (g_autoReorderDone.load()) {
        return;
    }

    if (g_autoReorderTimer) {
        Wh_Log(L"Auto-reorder: one-shot timer already scheduled");
        return;
    }

    DWORD delayMs =
        static_cast<DWORD>(g_settings.delayAfterModReadySeconds) * 1000;

    Wh_Log(L"Auto-reorder: scheduling one-shot timer in %d s",
           g_settings.delayAfterModReadySeconds);

    if (!CreateTimerQueueTimer(
            &g_autoReorderTimer, nullptr, AutoReorderTimerCallback, nullptr,
            delayMs, 0, WT_EXECUTEONLYONCE)) {
        Wh_Log(L"Auto-reorder: CreateTimerQueueTimer failed (%u)",
               GetLastError());
        g_autoReorderTimer = nullptr;
    }
}



// ----- auto-reorder-rules.cpp -----


#include <cwctype>
#include <functional>
#include <unordered_set>

namespace {

constexpr int kDelayMinSeconds = 0;
constexpr int kDelayMaxSeconds = 120;
constexpr int kSchemaVersion = 1;

int ClampDelaySeconds(int seconds) {
    if (seconds < kDelayMinSeconds) {
        return kDelayMinSeconds;
    }
    if (seconds > kDelayMaxSeconds) {
        return kDelayMaxSeconds;
    }
    return seconds;
}

void TrimInPlace(std::wstring& s) {
    size_t start = 0;
    while (start < s.size() && iswspace(s[start])) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && iswspace(s[end - 1])) {
        --end;
    }
    s = s.substr(start, end - start);
}

using AutoReorderCommon::EqualsIgnoreCase;

class JsonReader {
   public:
    explicit JsonReader(std::wstring_view text) : text_(text) {}

    bool ParseObject(
        const std::function<bool(std::wstring_view key)>& onKey) {
        if (!ConsumeChar(L'{')) {
            return false;
        }
        SkipWhitespace();
        if (TryConsumeChar(L'}')) {
            return true;
        }
        while (true) {
            std::wstring key;
            if (!ParseString(key)) {
                return false;
            }
            SkipWhitespace();
            if (!ConsumeChar(L':')) {
                return false;
            }
            SkipWhitespace();
            if (!onKey(key)) {
                return false;
            }
            SkipWhitespace();
            if (TryConsumeChar(L'}')) {
                return true;
            }
            if (!ConsumeChar(L',')) {
                return false;
            }
            SkipWhitespace();
        }
    }

    bool ParseArray(const std::function<bool()>& onElement) {
        if (!ConsumeChar(L'[')) {
            return false;
        }
        SkipWhitespace();
        if (TryConsumeChar(L']')) {
            return true;
        }
        while (true) {
            if (!onElement()) {
                return false;
            }
            SkipWhitespace();
            if (TryConsumeChar(L']')) {
                return true;
            }
            if (!ConsumeChar(L',')) {
                return false;
            }
            SkipWhitespace();
        }
    }

    bool ParseString(std::wstring& out) {
        if (!ConsumeChar(L'"')) {
            SetError(L"expected string");
            return false;
        }
        out.clear();
        while (pos_ < text_.size()) {
            wchar_t c = text_[pos_++];
            if (c == L'"') {
                return true;
            }
            if (c == L'\\') {
                if (pos_ >= text_.size()) {
                    SetError(L"unterminated escape in string");
                    return false;
                }
                wchar_t e = text_[pos_++];
                switch (e) {
                    case L'"':
                    case L'\\':
                    case L'/':
                        out.push_back(e);
                        break;
                    case L'b':
                        out.push_back(L'\b');
                        break;
                    case L'f':
                        out.push_back(L'\f');
                        break;
                    case L'n':
                        out.push_back(L'\n');
                        break;
                    case L'r':
                        out.push_back(L'\r');
                        break;
                    case L't':
                        out.push_back(L'\t');
                        break;
                    case L'u': {
                        if (pos_ + 4 > text_.size()) {
                            SetError(L"invalid \\u escape");
                            return false;
                        }
                        unsigned int code = 0;
                        for (int i = 0; i < 4; ++i) {
                            wchar_t hex = text_[pos_++];
                            code <<= 4;
                            if (hex >= L'0' && hex <= L'9') {
                                code |= static_cast<unsigned>(hex - L'0');
                            } else if (hex >= L'a' && hex <= L'f') {
                                code |= static_cast<unsigned>(hex - L'a' + 10);
                            } else if (hex >= L'A' && hex <= L'F') {
                                code |= static_cast<unsigned>(hex - L'A' + 10);
                            } else {
                                SetError(L"invalid \\u escape");
                                return false;
                            }
                        }
                        if (code <= 0xFFFF) {
                            out.push_back(static_cast<wchar_t>(code));
                        } else {
                            SetError(L"unsupported Unicode code point");
                            return false;
                        }
                        break;
                    }
                    default:
                        SetError(L"invalid escape in string");
                        return false;
                }
                continue;
            }
            if (static_cast<unsigned char>(c) < 0x20) {
                SetError(L"control character in string");
                return false;
            }
            out.push_back(c);
        }
        SetError(L"unterminated string");
        return false;
    }

    bool ParseBool(bool& out) {
        if (TryConsumeLiteral(L"true")) {
            out = true;
            return true;
        }
        if (TryConsumeLiteral(L"false")) {
            out = false;
            return true;
        }
        SetError(L"expected boolean");
        return false;
    }

    bool ParseInt(int& out) {
        SkipWhitespace();
        size_t start = pos_;
        bool negative = false;
        if (pos_ < text_.size() && text_[pos_] == L'-') {
            negative = true;
            ++pos_;
        }
        if (pos_ >= text_.size() || !iswdigit(text_[pos_])) {
            SetError(L"expected number");
            return false;
        }
        long long value = 0;
        while (pos_ < text_.size() && iswdigit(text_[pos_])) {
            value = value * 10 + (text_[pos_] - L'0');
            ++pos_;
        }
        if (pos_ < text_.size()) {
            wchar_t next = text_[pos_];
            if (next == L'.' || next == L'e' || next == L'E') {
                SetError(L"expected integer");
                return false;
            }
        }
        if (negative) {
            value = -value;
        }
        out = static_cast<int>(value);
        return true;
    }

    bool AtEnd() {
        SkipWhitespace();
        return pos_ >= text_.size();
    }

    const std::wstring& error() const { return error_; }

   private:
    std::wstring_view text_;
    size_t pos_ = 0;
    std::wstring error_;

    void SkipWhitespace() {
        while (pos_ < text_.size() && iswspace(text_[pos_])) {
            ++pos_;
        }
    }

    bool ConsumeChar(wchar_t c) {
        SkipWhitespace();
        if (pos_ < text_.size() && text_[pos_] == c) {
            ++pos_;
            return true;
        }
        error_ = std::wstring(L"expected '") + c + L"'";
        return false;
    }

    bool TryConsumeChar(wchar_t c) {
        SkipWhitespace();
        if (pos_ < text_.size() && text_[pos_] == c) {
            ++pos_;
            return true;
        }
        return false;
    }

    bool TryConsumeLiteral(std::wstring_view literal) {
        SkipWhitespace();
        if (pos_ + literal.size() > text_.size()) {
            return false;
        }
        if (text_.compare(pos_, literal.size(), literal) != 0) {
            return false;
        }
        pos_ += literal.size();
        return true;
    }

    void SetError(const wchar_t* message) { error_ = message; }
};

bool ParseMatchObject(JsonReader& reader, AutoReorderMatchRule& match,
                      std::wstring& errorOut) {
    bool hasStrings = false;
    bool hasPosition = false;
    bool ok = reader.ParseObject([&](std::wstring_view key) {
        if (EqualsIgnoreCase(key, L"strings")) {
            hasStrings = true;
            return reader.ParseArray([&]() {
                std::wstring s;
                if (!reader.ParseString(s)) {
                    return false;
                }
                TrimInPlace(s);
                if (s.empty()) {
                    errorOut = L"match strings entry must not be empty";
                    return false;
                }
                match.strings.push_back(std::move(s));
                return true;
            });
        }
        if (EqualsIgnoreCase(key, L"position")) {
            hasPosition = true;
            return reader.ParseInt(match.position);
        }
        errorOut = std::wstring(L"unknown key in match: ") + std::wstring(key);
        return false;
    });
    if (!ok) {
        if (errorOut.empty()) {
            errorOut = reader.error();
        }
        return false;
    }
    if (!hasStrings || match.strings.empty()) {
        errorOut = L"match requires non-empty strings array";
        return false;
    }
    if (!hasPosition) {
        errorOut = L"match requires position";
        return false;
    }
    if (match.position < 1) {
        errorOut = L"match position must be >= 1";
        return false;
    }
    return true;
}

bool ParseAppObject(JsonReader& reader, AutoReorderAppRules& app,
                    std::wstring& errorOut) {
    bool hasApp = false;
    bool hasMatches = false;
    bool ok = reader.ParseObject([&](std::wstring_view key) {
        if (EqualsIgnoreCase(key, L"app")) {
            hasApp = true;
            return reader.ParseString(app.app);
        }
        if (EqualsIgnoreCase(key, L"matches")) {
            hasMatches = true;
            return reader.ParseArray([&]() {
                AutoReorderMatchRule match;
                if (!ParseMatchObject(reader, match, errorOut)) {
                    return false;
                }
                app.matches.push_back(std::move(match));
                return true;
            });
        }
        errorOut = std::wstring(L"unknown key in app: ") + std::wstring(key);
        return false;
    });
    if (!ok) {
        if (errorOut.empty()) {
            errorOut = reader.error();
        }
        return false;
    }
    if (!hasApp) {
        errorOut = L"app entry requires app";
        return false;
    }
    TrimInPlace(app.app);
    if (app.app.empty()) {
        errorOut = L"app name must not be empty";
        return false;
    }
    if (!hasMatches) {
        errorOut = L"app entry requires matches";
        return false;
    }
    return true;
}

void WarnDuplicatePositions(const AutoReorderAppRules& app) {
    std::unordered_set<int> seen;
    for (const auto& match : app.matches) {
        if (!seen.insert(match.position).second) {
            Wh_Log(L"Auto-reorder: duplicate position %d for app %s",
                   match.position, app.app.c_str());
        }
    }
}

bool ParseRulesJsonDocument(std::wstring_view jsonText, AutoReorderRules& out,
                            std::wstring& errorOut) {
    JsonReader reader(jsonText);
    bool hasVersion = false;
    bool hasAutoReorder = false;
    bool hasDelay = false;
    bool hasApps = false;

    bool ok = reader.ParseObject([&](std::wstring_view key) {
        if (EqualsIgnoreCase(key, L"version")) {
            hasVersion = true;
            return reader.ParseInt(out.version);
        }
        if (EqualsIgnoreCase(key, L"autoReorderOnModLoad")) {
            hasAutoReorder = true;
            return reader.ParseBool(out.autoReorderOnModLoad);
        }
        if (EqualsIgnoreCase(key, L"delayAfterModReadySeconds")) {
            hasDelay = true;
            if (!reader.ParseInt(out.delayAfterModReadySeconds)) {
                return false;
            }
            out.delayAfterModReadySeconds =
                ClampDelaySeconds(out.delayAfterModReadySeconds);
            return true;
        }
        if (EqualsIgnoreCase(key, L"apps")) {
            hasApps = true;
            return reader.ParseArray([&]() {
                AutoReorderAppRules app;
                if (!ParseAppObject(reader, app, errorOut)) {
                    return false;
                }
                WarnDuplicatePositions(app);
                out.apps.push_back(std::move(app));
                return true;
            });
        }
        errorOut = std::wstring(L"unknown top-level key: ") + std::wstring(key);
        return false;
    });

    if (!ok) {
        if (errorOut.empty()) {
            errorOut = reader.error();
        }
        return false;
    }
    if (!reader.AtEnd()) {
        errorOut = L"unexpected trailing content in JSON";
        return false;
    }
    if (!hasVersion) {
        errorOut = L"missing version";
        return false;
    }
    if (out.version != kSchemaVersion) {
        errorOut = L"unsupported version (expected 1)";
        return false;
    }
    if (!hasApps) {
        errorOut = L"missing apps";
        return false;
    }
    if (!hasAutoReorder) {
        // Keep value already set from UI merge.
    }
    if (!hasDelay) {
        out.delayAfterModReadySeconds =
            ClampDelaySeconds(out.delayAfterModReadySeconds);
    }
    return true;
}

}  // namespace

void AutoReorderRules_LoadFromSettings(bool uiAutoReorderOnModLoad,
                                       int uiDelaySeconds,
                                       PCWSTR rulesJsonText,
                                       AutoReorderRules& out,
                                       bool& validOut,
                                       std::wstring& errorOut) {
    errorOut.clear();
    validOut = true;

    out = AutoReorderRules{};
    out.autoReorderOnModLoad = uiAutoReorderOnModLoad;
    out.delayAfterModReadySeconds = ClampDelaySeconds(uiDelaySeconds);

    std::wstring json =
        rulesJsonText ? std::wstring(rulesJsonText) : std::wstring{};
    TrimInPlace(json);

    if (json.empty()) {
        out.version = kSchemaVersion;
        return;
    }

    if (!ParseRulesJsonDocument(json, out, errorOut)) {
        validOut = false;
        out.apps.clear();
        return;
    }
}

// ----- auto-reorder-enumeration.cpp -----


#include <objbase.h>

#include <cwctype>
#include <string>

// {AA509086-5CA9-4C25-BF23-AFAC86C30765}
constexpr CLSID kVirtualDesktopManagerClsid = {
    0xAA509086,
    0x5CA9,
    0x4C25,
    {0xBF, 0x23, 0xAF, 0xAC, 0x86, 0xC3, 0x07, 0x65}};

// {A5F59FC4-5DE8-496A-9B7E-0886E57E5106}
constexpr IID kVirtualDesktopManagerIID = {
    0xA5F59FC4,
    0x5DE8,
    0x496A,
    {0x9B, 0x7E, 0x08, 0x86, 0xE5, 0x7E, 0x51, 0x06}};

constexpr GUID kSingleDesktopFallbackId = {
    0xA1B2C3D4,
    0xE5F6,
    0x4789,
    {0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78, 0x90}};

enum class VirtualDesktopMode { Unknown, ComManager, SingleDesktopFallback };

MIDL_INTERFACE("A5F59FC4-5DE8-496A-9B7E-0886E57E5106")
IVirtualDesktopManager : public IUnknown {
   public:
    virtual HRESULT STDMETHODCALLTYPE IsWindowOnCurrentVirtualDesktop(
        HWND topLevelWindow,
        BOOL* onCurrentDesktopOut) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWindowDesktopId(
        HWND topLevelWindow,
        GUID* desktopIdOut) = 0;
};

IVirtualDesktopManager* g_virtualDesktopManager = nullptr;
VirtualDesktopMode g_virtualDesktopMode = VirtualDesktopMode::Unknown;

bool UsesSingleDesktopFallback() {
    return g_virtualDesktopMode == VirtualDesktopMode::SingleDesktopFallback;
}

void EnsureComInitializedOnThread() {
    const HRESULT hr =
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        Wh_Log(L"Auto-reorder: CoInitializeEx failed (0x%08X)", hr);
    }
}

bool EnsureVirtualDesktopManager() {
    if (g_virtualDesktopManager) {
        return true;
    }

    if (UsesSingleDesktopFallback()) {
        return false;
    }

    EnsureComInitializedOnThread();

    HRESULT hr = CoCreateInstance(kVirtualDesktopManagerClsid, nullptr,
                                  CLSCTX_INPROC_SERVER, kVirtualDesktopManagerIID,
                                  reinterpret_cast<void**>(
                                      &g_virtualDesktopManager));
    if (SUCCEEDED(hr)) {
        g_virtualDesktopMode = VirtualDesktopMode::ComManager;
        return true;
    }

    if (g_virtualDesktopMode == VirtualDesktopMode::Unknown) {
        Wh_Log(L"Auto-reorder: IVirtualDesktopManager unavailable (0x%08X); "
               L"using single-desktop mode",
               hr);
        g_virtualDesktopMode = VirtualDesktopMode::SingleDesktopFallback;
    }

    return false;
}

bool GetWindowVirtualDesktopIdForHwnd(HWND hwnd, GUID& desktopIdOut) {
    if (EnsureVirtualDesktopManager()) {
        return SUCCEEDED(
            g_virtualDesktopManager->GetWindowDesktopId(hwnd, &desktopIdOut));
    }

    if (UsesSingleDesktopFallback()) {
        desktopIdOut = kSingleDesktopFallbackId;
        return true;
    }

    return false;
}

namespace {

using AutoReorderCommon::EqualsIgnoreCase;
using AutoReorderCommon::ToLowerCopy;

std::wstring GuidToLogString(const GUID& guid) {
    wchar_t buffer[64]{};
    if (StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer))) <=
        0) {
        return L"{?}";
    }
    return buffer;
}

bool IsTopLevelWindow(HWND hwnd) {
    return GetAncestor(hwnd, GA_ROOT) == hwnd;
}

std::wstring GetWindowProcessExeName(HWND hwnd) {
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (!processId) {
        return {};
    }

    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!process) {
        return {};
    }

    wchar_t path[MAX_PATH]{};
    DWORD pathSize = ARRAYSIZE(path);
    std::wstring exeName;
    if (QueryFullProcessImageNameW(process, 0, path, &pathSize)) {
        const wchar_t* baseName = wcsrchr(path, L'\\');
        exeName = baseName ? baseName + 1 : path;
    }

    CloseHandle(process);
    return ToLowerCopy(std::move(exeName));
}

bool IsFileExplorerWindow(HWND hwnd) {
    wchar_t className[64]{};
    if (!GetClassNameW(hwnd, className, ARRAYSIZE(className))) {
        return false;
    }

    return wcscmp(className, L"CabinetWClass") == 0 ||
           wcscmp(className, L"ExploreWClass") == 0;
}

bool WindowMatchesAppExe(HWND hwnd, std::wstring_view appExeLower) {
    if (EqualsIgnoreCase(appExeLower, L"explorer.exe")) {
        return IsFileExplorerWindow(hwnd);
    }

    return EqualsIgnoreCase(GetWindowProcessExeName(hwnd), appExeLower);
}

bool WindowOnDesktop(HWND hwnd, const GUID& desktopId) {
    if (UsesSingleDesktopFallback() &&
        IsEqualGUID(desktopId, kSingleDesktopFallbackId)) {
        return true;
    }

    GUID windowDesktopId{};
    if (!GetWindowVirtualDesktopIdForHwnd(hwnd, windowDesktopId)) {
        return false;
    }
    return IsEqualGUID(windowDesktopId, desktopId) != FALSE;
}

struct CollectDesktopIdsContext {
    std::vector<GUID> desktopIds;
};

BOOL CALLBACK CollectDesktopIdsProc(HWND hwnd, LPARAM lParam) {
    auto* context = reinterpret_cast<CollectDesktopIdsContext*>(lParam);
    if (!IsTopLevelWindow(hwnd)) {
        return TRUE;
    }

    GUID desktopId{};
    if (!GetWindowVirtualDesktopIdForHwnd(hwnd, desktopId)) {
        return TRUE;
    }

    for (const GUID& existing : context->desktopIds) {
        if (IsEqualGUID(existing, desktopId)) {
            return TRUE;
        }
    }

    context->desktopIds.push_back(desktopId);
    return TRUE;
}

bool IsLikelyTaskbarWindow(HWND hwnd) {
    if (!IsWindowVisible(hwnd)) {
        return false;
    }

    const HWND owner = GetWindow(hwnd, GW_OWNER);
    if (owner && owner != hwnd) {
        return false;
    }

    const LONG exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }

    return true;
}

struct CollectAppWindowsContext {
    std::wstring appExeLower;
    GUID desktopId{};
    std::vector<HWND> windows;
};

BOOL CALLBACK CollectAppWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* context = reinterpret_cast<CollectAppWindowsContext*>(lParam);
    if (!IsTopLevelWindow(hwnd)) {
        return TRUE;
    }

    if (!IsLikelyTaskbarWindow(hwnd)) {
        return TRUE;
    }

    if (!WindowOnDesktop(hwnd, context->desktopId)) {
        return TRUE;
    }

    if (!WindowMatchesAppExe(hwnd, context->appExeLower)) {
        return TRUE;
    }

    context->windows.push_back(hwnd);
    return TRUE;
}

}  // namespace

std::vector<GUID> EnumerateVirtualDesktops() {
    if (UsesSingleDesktopFallback()) {
        return {kSingleDesktopFallbackId};
    }

    CollectDesktopIdsContext context;
    EnumWindows(CollectDesktopIdsProc, reinterpret_cast<LPARAM>(&context));

    if (!context.desktopIds.empty()) {
        return context.desktopIds;
    }

    if (!EnsureVirtualDesktopManager()) {
        return {kSingleDesktopFallbackId};
    }

    Wh_Log(L"Auto-reorder: no desktop IDs from windows; using single-desktop "
           L"mode");
    g_virtualDesktopMode = VirtualDesktopMode::SingleDesktopFallback;
    return {kSingleDesktopFallbackId};
}

std::vector<HWND> GetWindowsForAppOnDesktop(const std::wstring& appExe,
                                            const GUID& desktopId) {
    AutoReorder_TraceStepU(L"T80 enum_app_windows:enter",
                           reinterpret_cast<ULONG_PTR>(appExe.c_str()));
    CollectAppWindowsContext context;
    context.appExeLower = ToLowerCopy(appExe);
    context.desktopId = desktopId;
    EnumWindows(CollectAppWindowsProc, reinterpret_cast<LPARAM>(&context));
    AutoReorder_TraceStep2U(L"T81 enum_app_windows:exit", context.windows.size(),
                            0);
    return context.windows;
}

void AutoReorder_LogEnumeratedTargets(
    const std::vector<AutoReorderAppRules>& appRules) {
    if (appRules.empty()) {
        Wh_Log(L"Auto-reorder: no app rules configured");
        return;
    }

    std::vector<GUID> desktops = EnumerateVirtualDesktops();
    if (desktops.empty()) {
        Wh_Log(L"Auto-reorder: no virtual desktops discovered from windows");
        return;
    }

    if (UsesSingleDesktopFallback()) {
        Wh_Log(L"Auto-reorder: single-desktop mode (IVirtualDesktopManager "
               L"unavailable)");
    } else {
        Wh_Log(L"Auto-reorder: discovered %zu virtual desktop(s)",
               desktops.size());
    }

    for (size_t desktopIndex = 0; desktopIndex < desktops.size();
         ++desktopIndex) {
        const GUID& desktopId = desktops[desktopIndex];
        Wh_Log(L"Auto-reorder: desktop[%zu] %s", desktopIndex,
               GuidToLogString(desktopId).c_str());

        for (const AutoReorderAppRules& appRule : appRules) {
            std::vector<HWND> windows =
                GetWindowsForAppOnDesktop(appRule.app, desktopId);
            Wh_Log(L"Auto-reorder:   app %s: %zu window(s)",
                   appRule.app.c_str(), windows.size());
        }
    }
}

void AutoReorderEnumeration_Uninit() {
    if (g_virtualDesktopManager) {
        g_virtualDesktopManager->Release();
        g_virtualDesktopManager = nullptr;
    }
    g_virtualDesktopMode = VirtualDesktopMode::Unknown;
}

// ----- auto-reorder-uia.cpp -----


#include <UIAutomation.h>
#include <objbase.h>

#include <atomic>
#include <cwctype>
#include <string>

namespace {

constexpr int kMaxTabTitles = 64;
constexpr DWORD kUiaPerWindowTimeoutMs = 2000;

// {ff48dba4-60ef-4201-aa87-54103eef594e}
constexpr CLSID kCUIAutomationClsid = {
    0xff48dba4,
    0x60ef,
    0x4201,
    {0xaa, 0x87, 0x54, 0x10, 0x3e, 0xef, 0x59, 0x4e}};

void ReleaseVariant(VARIANT& variant) {
    VariantClear(&variant);
}

bool CreateControlTypeCondition(IUIAutomation* automation,
                                CONTROLTYPEID controlType,
                                IUIAutomationCondition** conditionOut) {
    VARIANT variant{};
    variant.vt = VT_I4;
    variant.lVal = controlType;
    HRESULT hr = automation->CreatePropertyCondition(UIA_ControlTypePropertyId,
                                                     variant, conditionOut);
    ReleaseVariant(variant);
    return SUCCEEDED(hr);
}

void AppendTabItemNames(IUIAutomationElement* scope,
                        IUIAutomation* automation,
                        std::vector<std::wstring>& titlesOut) {
    if (!scope || titlesOut.size() >= static_cast<size_t>(kMaxTabTitles)) {
        return;
    }

    IUIAutomationCondition* tabItemCondition = nullptr;
    if (!CreateControlTypeCondition(automation, UIA_TabItemControlTypeId,
                                    &tabItemCondition)) {
        return;
    }

    IUIAutomationElementArray* tabItems = nullptr;
    HRESULT hr = scope->FindAll(TreeScope_Descendants, tabItemCondition,
                                &tabItems);
    tabItemCondition->Release();

    if (FAILED(hr) || !tabItems) {
        return;
    }

    int length = 0;
    tabItems->get_Length(&length);
    for (int i = 0; i < length &&
                    titlesOut.size() < static_cast<size_t>(kMaxTabTitles);
         ++i) {
        IUIAutomationElement* tabItem = nullptr;
        if (FAILED(tabItems->GetElement(i, &tabItem)) || !tabItem) {
            continue;
        }

        BSTR name = nullptr;
        if (SUCCEEDED(tabItem->get_CurrentName(&name)) && name) {
            if (SysStringLen(name) > 0) {
                titlesOut.emplace_back(name);
            }
            SysFreeString(name);
        }

        tabItem->Release();
    }

    tabItems->Release();
}

bool CollectTabTitlesViaUia(HWND hwnd, std::vector<std::wstring>& titlesOut) {
    IUIAutomation* automation = nullptr;
    HRESULT hr = CoCreateInstance(kCUIAutomationClsid, nullptr,
                                  CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation),
                                  reinterpret_cast<void**>(&automation));
    if (FAILED(hr) || !automation) {
        Wh_Log(L"Auto-reorder UIA: CoCreateInstance(CUIAutomation) failed "
               L"(0x%08X)",
               hr);
        return false;
    }

    IUIAutomationElement* root = nullptr;
    hr = automation->ElementFromHandle(hwnd, &root);
    if (FAILED(hr) || !root) {
        automation->Release();
        return false;
    }

    IUIAutomationCondition* tabControlCondition = nullptr;
    if (CreateControlTypeCondition(automation, UIA_TabControlTypeId,
                                   &tabControlCondition)) {
        IUIAutomationElement* tabControl = nullptr;
        hr = root->FindFirst(TreeScope_Descendants, tabControlCondition,
                             &tabControl);
        tabControlCondition->Release();

        if (SUCCEEDED(hr) && tabControl) {
            AppendTabItemNames(tabControl, automation, titlesOut);
            tabControl->Release();
        }
    }

    if (titlesOut.empty()) {
        AppendTabItemNames(root, automation, titlesOut);
    }

    root->Release();
    automation->Release();
    return !titlesOut.empty();
}

struct UiaCollectWork {
    HWND hwnd = nullptr;
    std::atomic<bool> abandoned{false};
    std::vector<std::wstring> titles;
    bool uiaSuccess = false;
};

DWORD WINAPI UiaCollectThreadProc(LPVOID parameter) {
    HRESULT hr =
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        Wh_Log(L"Auto-reorder UIA: CoInitializeEx on worker failed (0x%08X)",
               hr);
        return 1;
    }

    auto* work = static_cast<UiaCollectWork*>(parameter);
    std::vector<std::wstring> localTitles;
    const bool success = CollectTabTitlesViaUia(work->hwnd, localTitles);
    if (!work->abandoned.load(std::memory_order_acquire)) {
        work->titles = std::move(localTitles);
        work->uiaSuccess = success;
    }

    CoUninitialize();
    return 0;
}

bool CollectTabTitlesViaUiaWithTimeout(HWND hwnd,
                                       std::vector<std::wstring>& titlesOut) {
    UiaCollectWork work{};
    work.hwnd = hwnd;

    HANDLE thread =
        CreateThread(nullptr, 0, UiaCollectThreadProc, &work, 0, nullptr);
    if (!thread) {
        Wh_Log(L"Auto-reorder UIA: CreateThread failed (%u)", GetLastError());
        return false;
    }

    const DWORD waitResult =
        WaitForSingleObject(thread, kUiaPerWindowTimeoutMs);
    if (waitResult == WAIT_TIMEOUT) {
        work.abandoned.store(true, std::memory_order_release);
        Wh_Log(L"Auto-reorder UIA: hwnd %p timed out after %u ms, using "
               L"fallback",
               hwnd, kUiaPerWindowTimeoutMs);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        return false;
    }

    if (waitResult != WAIT_OBJECT_0) {
        Wh_Log(L"Auto-reorder UIA: WaitForSingleObject failed (%u)",
               GetLastError());
        work.abandoned.store(true, std::memory_order_release);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
        return false;
    }

    CloseHandle(thread);

    if (work.uiaSuccess && !work.titles.empty()) {
        titlesOut = std::move(work.titles);
        return true;
    }

    return false;
}

std::wstring GetWindowTitleFallback(HWND hwnd) {
    wchar_t title[1024]{};
    const int length = GetWindowTextW(hwnd, title, ARRAYSIZE(title));
    if (length <= 0) {
        return {};
    }
    return title;
}

void LogTabTitlesForWindow(HWND hwnd, const std::vector<std::wstring>& titles) {
    if (titles.empty()) {
        Wh_Log(L"Auto-reorder:     hwnd %p: no tab titles", hwnd);
        return;
    }

    for (size_t i = 0; i < titles.size(); ++i) {
        Wh_Log(L"Auto-reorder:     hwnd %p tab[%zu]: \"%s\"", hwnd, i,
               titles[i].c_str());
    }
}

}  // namespace

bool AutoReorder_ContainsSubstringIgnoreCase(std::wstring_view haystack,
                                             std::wstring_view needle) {
    if (needle.empty()) {
        return true;
    }
    if (haystack.size() < needle.size()) {
        return false;
    }

    for (size_t i = 0; i <= haystack.size() - needle.size(); ++i) {
        bool matches = true;
        for (size_t j = 0; j < needle.size(); ++j) {
            if (towlower(haystack[i + j]) != towlower(needle[j])) {
                matches = false;
                break;
            }
        }
        if (matches) {
            return true;
        }
    }
    return false;
}

bool AutoReorder_TabTitlesContainAllStrings(
    const std::vector<std::wstring>& tabTitles,
    const std::vector<std::wstring>& requiredStrings) {
    for (const std::wstring& required : requiredStrings) {
        bool found = false;
        for (const std::wstring& tabTitle : tabTitles) {
            if (AutoReorder_ContainsSubstringIgnoreCase(tabTitle, required)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

std::vector<std::wstring> GetTabTitlesForWindow(HWND hwnd) {
    std::vector<std::wstring> titles;
    if (CollectTabTitlesViaUiaWithTimeout(hwnd, titles)) {
        return titles;
    }

    std::wstring windowTitle = GetWindowTitleFallback(hwnd);
    if (!windowTitle.empty()) {
        titles.push_back(std::move(windowTitle));
        Wh_Log(L"Auto-reorder UIA: hwnd %p using window title fallback", hwnd);
    }

    return titles;
}

void AutoReorder_LogTabTitlesForTargets(
    const std::vector<AutoReorderAppRules>& appRules) {
    if (appRules.empty()) {
        return;
    }

    std::vector<GUID> desktops = EnumerateVirtualDesktops();
    if (desktops.empty()) {
        Wh_Log(L"Auto-reorder UIA: skipping tab title log (no desktops)");
        return;
    }

    for (size_t desktopIndex = 0; desktopIndex < desktops.size();
         ++desktopIndex) {
        const GUID& desktopId = desktops[desktopIndex];

        for (const AutoReorderAppRules& appRule : appRules) {
            std::vector<HWND> windows =
                GetWindowsForAppOnDesktop(appRule.app, desktopId);
            if (windows.empty()) {
                continue;
            }

            Wh_Log(L"Auto-reorder UIA: desktop[%zu] app %s tab titles:",
                   desktopIndex, appRule.app.c_str());

            for (HWND hwnd : windows) {
                std::vector<std::wstring> titles = GetTabTitlesForWindow(hwnd);
                LogTabTitlesForWindow(hwnd, titles);
            }
        }
    }
}

void AutoReorderUia_Uninit() {}

// ----- auto-reorder-engine.cpp -----


#include <algorithm>
#include <mutex>

namespace {

using AutoReorderCommon::EqualsIgnoreCase;
using AutoReorderCommon::ToLowerCopy;

struct CachedDesktopOrder {
    std::wstring appExeLower;
    GUID desktopId{};
    std::vector<HWND> hwndOrder;
};

std::mutex g_cachedDesktopOrdersMutex;
std::vector<CachedDesktopOrder> g_cachedDesktopOrders;

int FindFirstMatchingUnassignedWindow(
    const std::vector<WindowWithTabTitles>& windows,
    const std::vector<bool>& assigned,
    const AutoReorderMatchRule& rule) {
    for (size_t i = 0; i < windows.size(); ++i) {
        if (assigned[i]) {
            continue;
        }
        if (AutoReorder_TabTitlesContainAllStrings(windows[i].tabTitles,
                                                   rule.strings)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void LogComputedOrder(HWND hwnd,
                      size_t rank,
                      const std::vector<WindowWithTabTitles>& windows) {
    size_t windowIndex = 0;
    for (; windowIndex < windows.size(); ++windowIndex) {
        if (windows[windowIndex].hwnd == hwnd) {
            break;
        }
    }

    if (windowIndex < windows.size()) {
        Wh_Log(L"Auto-reorder engine:   #%zu hwnd %p (%zu tab title(s))", rank,
               hwnd, windows[windowIndex].tabTitles.size());
    } else {
        Wh_Log(L"Auto-reorder engine:   #%zu hwnd %p", rank, hwnd);
    }
}

}  // namespace

void AutoReorder_ClearCachedDesktopOrders() {
    std::lock_guard<std::mutex> lock(g_cachedDesktopOrdersMutex);
    g_cachedDesktopOrders.clear();
}

void AutoReorder_StoreCachedDesktopOrder(const std::wstring& appExe,
                                         const GUID& desktopId,
                                         std::vector<HWND> hwndOrder) {
    const std::wstring appLower = ToLowerCopy(appExe);
    std::lock_guard<std::mutex> lock(g_cachedDesktopOrdersMutex);
    for (auto it = g_cachedDesktopOrders.begin(); it != g_cachedDesktopOrders.end();
         ++it) {
        if (EqualsIgnoreCase(it->appExeLower, appLower) &&
            IsEqualGUID(it->desktopId, desktopId)) {
            it->hwndOrder = std::move(hwndOrder);
            return;
        }
    }

    CachedDesktopOrder entry;
    entry.appExeLower = appLower;
    entry.desktopId = desktopId;
    entry.hwndOrder = std::move(hwndOrder);
    g_cachedDesktopOrders.push_back(std::move(entry));
}

bool AutoReorder_TryGetCachedDesktopOrder(const std::wstring& appExe,
                                          const GUID& desktopId,
                                          std::vector<HWND>& hwndOrderOut) {
    const std::wstring appLower = ToLowerCopy(appExe);
    std::lock_guard<std::mutex> lock(g_cachedDesktopOrdersMutex);
    for (const CachedDesktopOrder& entry : g_cachedDesktopOrders) {
        if (EqualsIgnoreCase(entry.appExeLower, appLower) &&
            IsEqualGUID(entry.desktopId, desktopId)) {
            hwndOrderOut = entry.hwndOrder;
            return !hwndOrderOut.empty();
        }
    }

    return false;
}

std::vector<HWND> ComputeOrder(
    const std::vector<WindowWithTabTitles>& windows,
    const std::vector<AutoReorderMatchRule>& matches) {
    const size_t windowCount = windows.size();
    if (windowCount == 0) {
        return {};
    }

    if (matches.empty()) {
        std::vector<HWND> stableOrder;
        stableOrder.reserve(windowCount);
        for (const WindowWithTabTitles& window : windows) {
            stableOrder.push_back(window.hwnd);
        }
        return stableOrder;
    }

    std::vector<AutoReorderMatchRule> sortedMatches = matches;
    std::stable_sort(sortedMatches.begin(), sortedMatches.end(),
                     [](const AutoReorderMatchRule& a,
                        const AutoReorderMatchRule& b) {
                         return a.position < b.position;
                     });

    std::vector<HWND> result;
    result.reserve(windowCount);
    std::vector<bool> assigned(windowCount, false);

    // Rules are tried in position order. Each successful match takes the next
    // flyout slot (1, 2, 3, ...). Unmatched rules are skipped with no gap.
    for (const AutoReorderMatchRule& rule : sortedMatches) {
        const int windowIndex =
            FindFirstMatchingUnassignedWindow(windows, assigned, rule);
        if (windowIndex < 0) {
            continue;
        }

        assigned[static_cast<size_t>(windowIndex)] = true;
        result.push_back(windows[static_cast<size_t>(windowIndex)].hwnd);
    }

    for (size_t i = 0; i < windowCount; ++i) {
        if (!assigned[i]) {
            result.push_back(windows[i].hwnd);
        }
    }

    return result;
}

void AutoReorder_ComputeAndLogOrders(
    const std::vector<AutoReorderAppRules>& appRules) {
    if (appRules.empty()) {
        return;
    }

    AutoReorder_ClearCachedDesktopOrders();

    std::vector<GUID> desktops = EnumerateVirtualDesktops();
    if (desktops.empty()) {
        return;
    }

    for (size_t desktopIndex = 0; desktopIndex < desktops.size();
         ++desktopIndex) {
        const GUID& desktopId = desktops[desktopIndex];

        for (const AutoReorderAppRules& appRule : appRules) {
            std::vector<HWND> windowHandles =
                GetWindowsForAppOnDesktop(appRule.app, desktopId);
            if (windowHandles.empty()) {
                continue;
            }

            std::vector<WindowWithTabTitles> windows;
            windows.reserve(windowHandles.size());
            for (HWND hwnd : windowHandles) {
                WindowWithTabTitles entry;
                entry.hwnd = hwnd;
                entry.tabTitles = GetTabTitlesForWindow(hwnd);
                windows.push_back(std::move(entry));
            }

            std::vector<HWND> order = ComputeOrder(windows, appRule.matches);
            Wh_Log(L"Auto-reorder engine: desktop[%zu] app %s computed order "
                   L"(%zu window(s)):",
                   desktopIndex, appRule.app.c_str(), order.size());

            for (size_t rank = 0; rank < order.size(); ++rank) {
                LogComputedOrder(order[rank], rank + 1, windows);
            }

            AutoReorder_StoreCachedDesktopOrder(appRule.app, desktopId,
                                                std::move(order));
        }
    }
}

void AutoReorder_RunEngineSelfTestLog() {
    const std::vector<AutoReorderMatchRule> rules = {
        {{L"gmail", L"onenote"}, 1},
        {{L"adidas"}, 2},
    };

    const std::vector<WindowWithTabTitles> fixtureWindows = {
        {nullptr, {L"reddit"}},
        {nullptr, {L"gmail", L"onenote", L"adidas"}},
        {nullptr, {L"gmail"}},
        {nullptr, {L"adidas"}},
    };

    const std::vector<HWND> expectedOrder = {
        fixtureWindows[1].hwnd,
        fixtureWindows[3].hwnd,
        fixtureWindows[0].hwnd,
        fixtureWindows[2].hwnd,
    };

    const std::vector<HWND> actualOrder =
        ComputeOrder(fixtureWindows, rules);

    bool passed = actualOrder.size() == expectedOrder.size();
    for (size_t i = 0; passed && i < expectedOrder.size(); ++i) {
        passed = actualOrder[i] == expectedOrder[i];
    }

    Wh_Log(L"Auto-reorder engine self-test (manual matrix): %s",
           passed ? L"PASS" : L"FAIL");

    // Skipped rule compresses later matches upward (AnimeKAI missing -> XDA at #2).
    const std::vector<AutoReorderMatchRule> compressRules = {
        {{L"rule-a"}, 1},
        {{L"missing"}, 2},
        {{L"rule-c"}, 3},
    };
    const std::vector<WindowWithTabTitles> compressWindows = {
        {nullptr, {L"other"}},
        {nullptr, {L"rule-a"}},
        {nullptr, {L"rule-c"}},
    };
    const std::vector<HWND> compressExpected = {
        compressWindows[1].hwnd,
        compressWindows[2].hwnd,
        compressWindows[0].hwnd,
    };
    const std::vector<HWND> compressActual =
        ComputeOrder(compressWindows, compressRules);
    bool compressPassed =
        compressActual.size() == compressExpected.size();
    for (size_t i = 0; compressPassed && i < compressExpected.size(); ++i) {
        compressPassed = compressActual[i] == compressExpected[i];
    }
    Wh_Log(L"Auto-reorder engine self-test (skipped rule compress): %s",
           compressPassed ? L"PASS" : L"FAIL");

    if (!passed) {
        for (size_t i = 0; i < actualOrder.size(); ++i) {
            int fixtureIndex = -1;
            for (size_t j = 0; j < fixtureWindows.size(); ++j) {
                if (actualOrder[i] == fixtureWindows[j].hwnd) {
                    fixtureIndex = static_cast<int>(j);
                    break;
                }
            }
            Wh_Log(
                L"Auto-reorder engine self-test: actual[%zu] expected fixture "
                L"W%d",
                i, fixtureIndex);
        }
    }
}

#include <unordered_map>

void* FindTaskGroupViaThumbnailMapping(const std::vector<HWND>& targetHwnds) {
    if (!CWindowTaskItem_GetWindow || targetHwnds.empty()) {
        return nullptr;
    }

    std::unordered_map<void*, size_t> groupMatchCounts;

    for (const ThumbnailTaskItemMapping& mapping : g_thumbnailTaskItemMapping) {
        if (!mapping.taskGroup || !mapping.taskItem) {
            continue;
        }

        HWND hwnd = nullptr;
        if (CImmersiveTaskItem_vftable &&
            *(void**)mapping.taskItem == CImmersiveTaskItem_vftable &&
            CImmersiveTaskItem_GetWindow) {
            hwnd = CImmersiveTaskItem_GetWindow(mapping.taskItem);
        } else {
            hwnd = CWindowTaskItem_GetWindow(mapping.taskItem);
        }

        if (!hwnd) {
            continue;
        }

        for (HWND target : targetHwnds) {
            if (hwnd == target) {
                groupMatchCounts[mapping.taskGroup]++;
                break;
            }
        }
    }

    void* bestGroup = nullptr;
    size_t bestCount = 0;
    for (const auto& entry : groupMatchCounts) {
        if (entry.second > bestCount) {
            bestCount = entry.second;
            bestGroup = entry.first;
        }
    }

    if (bestGroup) {
        Wh_Log(L"Auto-reorder apply: task group %p from XAML thumbnail mapping "
               L"(%zu match(es))",
               bestGroup, bestCount);
    }

    return bestCount > 0 ? bestGroup : nullptr;
}

void RefreshCachedTaskGroupFromThumbnailMapping(bool notifyApply) {
    if (g_thumbnailTaskItemMapping.empty()) {
        return;
    }

    std::unordered_map<void*, size_t> groupCounts;
    for (const ThumbnailTaskItemMapping& mapping : g_thumbnailTaskItemMapping) {
        if (mapping.taskGroup) {
            groupCounts[mapping.taskGroup]++;
        }
    }

    void* bestGroup = nullptr;
    size_t bestCount = 0;
    for (const auto& entry : groupCounts) {
        if (entry.second > bestCount) {
            bestCount = entry.second;
            bestGroup = entry.first;
        }
    }

    if (bestGroup && bestCount > 1) {
        g_cachedTaskGroupFromFlyout = bestGroup;
        Wh_Log(L"Auto-reorder: cached flyout task group %p (%zu thumbnail(s))",
               bestGroup, bestCount);
        if (notifyApply) {
            AutoReorder_OnFlyoutMappingRefreshed();
        }
    }
}

bool AutoReorder_FlyoutThumbnailsReady() {
    auto thumbnails = g_TaskGroup_Thumbnails.get();
    if (!thumbnails) {
        return false;
    }

    auto* thumbnailsPtr = winrt::get_abi(thumbnails);
    const int thumbnailsSize =
        TaskItemThumbnail_Size_Original(&thumbnailsPtr);
    return thumbnailsSize >= 2;
}

bool AutoReorder_HasFlyoutThumbnailMapping() {
    return !g_thumbnailTaskItemMapping.empty();
}

bool AutoReorder_IsXamlFlyoutPreviewActive() {
    return g_TaskGroup_Thumbnails.get() != nullptr;
}

void AutoReorder_OnFlyoutMappingRefreshed() {
    AutoReorder_TraceStep(L"T90 flyout_mapping_refreshed:enter");
    if (!g_rulesValid || g_rules.apps.empty()) {
        return;
    }

    if (g_reorderingXamlThumbnails) {
        AutoReorder_TraceStep(L"T90b flyout_mapping_refreshed:skip reordering");
        return;
    }

    if (g_pendingSessionApply.exchange(false) ||
        !g_pendingXamlFlyoutOrder.empty()) {
        AutoReorder_TraceStep(L"T91 flyout_mapping_refreshed:schedule_deferred");
        ScheduleDeferredAutoReorderApply();
        return;
    }

    if (AutoReorder_ShouldDeferApply()) {
        Wh_Log(L"Auto-reorder apply: flyout mapping ready, scheduling deferred "
               L"apply");
        ScheduleDeferredAutoReorderApply();
    }
}

// ----- auto-reorder-apply.cpp -----


#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// Resolved in taskbar-thumbnail-reorder.wh.cpp (same translation unit).
extern CTaskBtnGroup_GetGroupType_t CTaskBtnGroup_GetGroupType;
extern CTaskBtnGroup_GetGroup_t CTaskBtnGroup_GetGroup;
extern CWindowTaskItem_GetWindow_t CWindowTaskItem_GetWindow;
extern CImmersiveTaskItem_GetWindow_t CImmersiveTaskItem_GetWindow;
extern void* CImmersiveTaskItem_vftable;
extern bool MoveTaskInGroup(void* taskGroup, void* taskItemFrom, void* taskItemTo);
extern HDPA GetTaskItemsArray(void* taskGroup);

using CTaskListThumbnailWnd_GetTaskGroup_t = void*(WINAPI*)(void* pThis);
using CTaskListThumbnailWnd_TaskReordered_t = void(WINAPI*)(void* pThis,
                                                           void* taskItem);
extern CTaskListThumbnailWnd_GetTaskGroup_t CTaskListThumbnailWnd_GetTaskGroup;
extern CTaskListThumbnailWnd_TaskReordered_t CTaskListThumbnailWnd_TaskReordered;

namespace {

std::atomic<size_t> g_taskListButtonGroupsOffset{SIZE_MAX};

DWORD GetTaskbarUiThreadId() {
    HWND trayWnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!trayWnd) {
        return 0;
    }

    DWORD threadId = 0;
    GetWindowThreadProcessId(trayWnd, &threadId);
    return threadId;
}

bool IsTaskbarUiThread() {
    const DWORD taskbarThreadId = GetTaskbarUiThreadId();
    return taskbarThreadId != 0 &&
           taskbarThreadId == GetCurrentThreadId();
}

HWND GetPrimaryTaskListWnd() {
    HWND trayWnd = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!trayWnd) {
        return nullptr;
    }

    HWND taskBandWnd =
        reinterpret_cast<HWND>(GetPropW(trayWnd, L"TaskbandHWND"));
    if (!taskBandWnd) {
        return nullptr;
    }

    return FindWindowExW(taskBandWnd, nullptr, L"MSTaskListWClass", nullptr);
}

bool TaskbarSymbolsReady() {
    return CTaskBtnGroup_GetGroupType && CTaskBtnGroup_GetGroup &&
           CWindowTaskItem_GetWindow;
}

HWND GetHwndFromTaskItem(void* taskItem) {
    if (!taskItem) {
        return nullptr;
    }

    if (CImmersiveTaskItem_vftable &&
        *(void**)taskItem == CImmersiveTaskItem_vftable &&
        CImmersiveTaskItem_GetWindow) {
        return CImmersiveTaskItem_GetWindow(taskItem);
    }

    if (CWindowTaskItem_GetWindow) {
        return CWindowTaskItem_GetWindow(taskItem);
    }

    return nullptr;
}

bool IsPlausibleButtonGroupsDpa(HDPA hdpa) {
    if (!hdpa) {
        return false;
    }

    if (hdpa->cpItems < 1 || hdpa->cpItems > 500) {
        return false;
    }

    if (!hdpa->pArray) {
        return false;
    }

    return true;
}

bool ButtonGroupsDpaContainsTaskGroup(HDPA hdpa, void** sampleTaskGroupOut) {
    if (!IsPlausibleButtonGroupsDpa(hdpa)) {
        return false;
    }

    for (int i = 0; i < hdpa->cpItems; ++i) {
        void* btnGroup = ((void**)hdpa->pArray)[i];
        if (!btnGroup) {
            continue;
        }

        int groupType = CTaskBtnGroup_GetGroupType(btnGroup);
        if (groupType != 1 && groupType != 3) {
            continue;
        }

        void* taskGroup = CTaskBtnGroup_GetGroup(btnGroup);
        if (!taskGroup) {
            continue;
        }

        HDPA taskItems = GetTaskItemsArray(taskGroup);
        if (!taskItems || taskItems->cpItems < 1) {
            continue;
        }

        if (sampleTaskGroupOut) {
            *sampleTaskGroupOut = taskGroup;
        }
        return true;
    }

    return false;
}

HDPA GetButtonGroupsDpa(void* taskListWndPtr) {
    AutoReorder_TraceStepU(L"T52 get_button_groups_dpa:enter list_ptr",
                           reinterpret_cast<ULONG_PTR>(taskListWndPtr));
    if (!taskListWndPtr) {
        return nullptr;
    }

    size_t cachedOffset = g_taskListButtonGroupsOffset.load();
    if (cachedOffset != SIZE_MAX) {
        AutoReorder_TraceStep2U(L"T52b get_button_groups_dpa:cached_offset",
                              cachedOffset, 0);
        return *(HDPA*)((void**)taskListWndPtr)[cachedOffset];
    }

    for (size_t i = 0; i < 256; ++i) {
        HDPA candidate = *(HDPA*)((void**)taskListWndPtr)[i];
        void* sampleTaskGroup = nullptr;
        if (ButtonGroupsDpaContainsTaskGroup(candidate, &sampleTaskGroup)) {
            g_taskListButtonGroupsOffset.store(i);
            return candidate;
        }
    }

    return nullptr;
}

bool DesktopOrderAlreadyMatches(
    HDPA taskItems,
    const std::vector<int>& desktopSlotIndices,
    const std::vector<void*>& desiredTaskItems) {
    if (desktopSlotIndices.size() != desiredTaskItems.size()) {
        return false;
    }

    for (size_t i = 0; i < desktopSlotIndices.size(); ++i) {
        void* currentItem =
            ((void**)taskItems->pArray)[desktopSlotIndices[i]];
        if (currentItem != desiredTaskItems[i]) {
            return false;
        }
    }

    return true;
}

void NotifyThumbnailFlyoutReordered(void* taskGroup, void* taskItem) {
    if (!CTaskListThumbnailWnd_GetTaskGroup ||
        !CTaskListThumbnailWnd_TaskReordered || !taskGroup || !taskItem) {
        return;
    }

    struct FindThumbnailWndContext {
        void* taskGroup = nullptr;
        void* thumbnailLongPtr = nullptr;
    } context{taskGroup, nullptr};

    EnumWindows(
        [](HWND hwnd, LPARAM lParam) -> BOOL {
            auto* ctx = reinterpret_cast<FindThumbnailWndContext*>(lParam);
            wchar_t className[64]{};
            if (GetClassNameW(hwnd, className, ARRAYSIZE(className)) == 0) {
                return TRUE;
            }
            if (wcscmp(className, L"TaskListThumbnailWnd") != 0) {
                return TRUE;
            }

            void* thumbnailLongPtr =
                reinterpret_cast<void*>(GetWindowLongPtrW(hwnd, 0));
            if (!thumbnailLongPtr) {
                return TRUE;
            }

            if (CTaskListThumbnailWnd_GetTaskGroup(thumbnailLongPtr) ==
                ctx->taskGroup) {
                ctx->thumbnailLongPtr = thumbnailLongPtr;
                return FALSE;
            }

            return TRUE;
        },
        reinterpret_cast<LPARAM>(&context));

    if (context.thumbnailLongPtr) {
        CTaskListThumbnailWnd_TaskReordered(context.thumbnailLongPtr, taskItem);
    }
}

std::vector<HWND> BuildFlyoutAlignedApplyOrder(
    const std::vector<HWND>& computedOrder,
    const std::vector<HWND>& flyoutMappedHwnds) {
    if (flyoutMappedHwnds.size() < 2) {
        return computedOrder;
    }

    std::unordered_set<HWND> mappedSet(flyoutMappedHwnds.begin(),
                                       flyoutMappedHwnds.end());
    std::vector<HWND> result;
    result.reserve(flyoutMappedHwnds.size());
    std::unordered_set<HWND> added;

    for (HWND hwnd : computedOrder) {
        if (mappedSet.find(hwnd) != mappedSet.end() &&
            added.insert(hwnd).second) {
            result.push_back(hwnd);
        }
    }

    for (HWND hwnd : flyoutMappedHwnds) {
        if (added.insert(hwnd).second) {
            result.push_back(hwnd);
        }
    }

    return result;
}

int FindTaskItemIndex(HDPA taskItems, void* taskItem) {
    if (!taskItems || !taskItem) {
        return -1;
    }

    for (int i = 0; i < taskItems->cpItems; ++i) {
        if (((void**)taskItems->pArray)[i] == taskItem) {
            return i;
        }
    }

    return -1;
}

bool ApplyMovesToSlotIndices(void* taskGroup,
                             HDPA taskItems,
                             const std::vector<int>& slotIndices,
                             const std::vector<void*>& desiredTaskItems) {
    if (slotIndices.size() != desiredTaskItems.size()) {
        Wh_Log(L"Auto-reorder apply: slot/task item count mismatch for group %p",
               taskGroup);
        return false;
    }

    if (DesktopOrderAlreadyMatches(taskItems, slotIndices, desiredTaskItems)) {
        return true;
    }

    for (size_t targetPos = 0; targetPos < desiredTaskItems.size(); ++targetPos) {
        const size_t targetSlotIndex =
            static_cast<size_t>(slotIndices[targetPos]);
        void* itemToMove = desiredTaskItems[targetPos];

        while (true) {
            taskItems = GetTaskItemsArray(taskGroup);
            if (!taskItems) {
                return false;
            }

            const int currentIndex = FindTaskItemIndex(taskItems, itemToMove);
            if (currentIndex < 0) {
                Wh_Log(L"Auto-reorder apply: task item not found during move");
                return false;
            }

            if (static_cast<size_t>(currentIndex) == targetSlotIndex) {
                break;
            }

            const int stepIndex = currentIndex < static_cast<int>(targetSlotIndex)
                                      ? currentIndex + 1
                                      : currentIndex - 1;
            void* neighborItem = ((void**)taskItems->pArray)[stepIndex];
            if (!MoveTaskInGroup(taskGroup, itemToMove, neighborItem)) {
                Wh_Log(L"Auto-reorder apply: MoveTaskInGroup failed (%d -> %zu)",
                       currentIndex, targetSlotIndex);
                return false;
            }
        }
    }

    return true;
}

void VisitTaskListLongPtrFromTray(HWND trayWnd,
                                  bool secondary,
                                  const std::function<void(void*)>& callback) {
    if (!trayWnd) {
        return;
    }

    HWND taskBandWnd =
        secondary ? FindWindowEx(trayWnd, nullptr, L"WorkerW", nullptr)
                  : reinterpret_cast<HWND>(GetProp(trayWnd, L"TaskbandHWND"));
    if (!taskBandWnd) {
        return;
    }

    HWND taskListWnd =
        FindWindowEx(taskBandWnd, nullptr, L"MSTaskListWClass", nullptr);
    if (!taskListWnd) {
        return;
    }

    void* taskListLongPtr =
        reinterpret_cast<void*>(GetWindowLongPtr(taskListWnd, 0));
    if (taskListLongPtr) {
        callback(taskListLongPtr);
    }
}

void ForEachTaskListLongPtr(
    const std::function<void(void* taskListLongPtr)>& callback) {
    VisitTaskListLongPtrFromTray(FindWindowW(L"Shell_TrayWnd", nullptr), false,
                                 callback);

    EnumWindows(
        [](HWND hwnd, LPARAM lParam) -> BOOL {
            WCHAR className[32];
            if (GetClassNameW(hwnd, className, ARRAYSIZE(className)) == 0) {
                return TRUE;
            }

            if (_wcsicmp(className, L"Shell_SecondaryTrayWnd") != 0) {
                return TRUE;
            }

            auto* callbackRef =
                reinterpret_cast<const std::function<void(void*)>*>(lParam);
            VisitTaskListLongPtrFromTray(hwnd, true, *callbackRef);
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&callback));
}

void* FindTaskGroupForDesktopWindows(void* taskListLongPtr,
                                     const std::vector<HWND>& targetHwnds);

void* TryCachedFlyoutTaskGroup(const std::vector<HWND>& targetHwnds) {
    if (!g_cachedTaskGroupFromFlyout) {
        return nullptr;
    }

    HDPA taskItems = GetTaskItemsArray(g_cachedTaskGroupFromFlyout);
    if (!taskItems || taskItems->cpItems <= 1) {
        return nullptr;
    }

    size_t matchCount = 0;
    for (HWND targetHwnd : targetHwnds) {
        for (int j = 0; j < taskItems->cpItems; ++j) {
            void* taskItem = ((void**)taskItems->pArray)[j];
            if (GetHwndFromTaskItem(taskItem) == targetHwnd) {
                ++matchCount;
                break;
            }
        }
    }

    if (matchCount < 2) {
        return nullptr;
    }

    Wh_Log(L"Auto-reorder apply: using cached flyout task group %p (%zu "
           L"match(es))",
           g_cachedTaskGroupFromFlyout, matchCount);
    return g_cachedTaskGroupFromFlyout;
}

void* FindTaskGroupForTargetHwnds(const std::vector<HWND>& targetHwnds) {
    AutoReorder_TraceStep2U(L"T50 find_task_group:enter", targetHwnds.size(), 0);
    if (void* cached = TryCachedFlyoutTaskGroup(targetHwnds)) {
        AutoReorder_TraceStepU(L"T50b find_task_group:from_flyout_cache",
                               reinterpret_cast<ULONG_PTR>(cached));
        return cached;
    }

    if (void* fromMapping = FindTaskGroupViaThumbnailMapping(targetHwnds)) {
        AutoReorder_TraceStepU(L"T51 find_task_group:from_thumbnail_mapping",
                               reinterpret_cast<ULONG_PTR>(fromMapping));
        return fromMapping;
    }

    AutoReorder_TraceStep(L"T52 find_task_group:before_foreach_tasklist");
    void* bestTaskGroup = nullptr;
    size_t bestMatchCount = 0;

    ForEachTaskListLongPtr([&](void* taskListLongPtr) {
        AutoReorder_TraceStepU(L"T53 find_task_group:foreach_list_ptr",
                               reinterpret_cast<ULONG_PTR>(taskListLongPtr));
        void* taskGroup =
            FindTaskGroupForDesktopWindows(taskListLongPtr, targetHwnds);
        if (!taskGroup) {
            return;
        }

        HDPA taskItems = GetTaskItemsArray(taskGroup);
        if (!taskItems) {
            return;
        }

        size_t matchCount = 0;
        for (HWND targetHwnd : targetHwnds) {
            for (int j = 0; j < taskItems->cpItems; ++j) {
                void* taskItem = ((void**)taskItems->pArray)[j];
                if (GetHwndFromTaskItem(taskItem) == targetHwnd) {
                    ++matchCount;
                    break;
                }
            }
        }

        if (matchCount > bestMatchCount) {
            bestMatchCount = matchCount;
            bestTaskGroup = taskGroup;
        }
    });

    AutoReorder_TraceStep2U(L"T55 find_task_group:exit",
                              reinterpret_cast<ULONG_PTR>(bestTaskGroup),
                              bestMatchCount);
    return bestMatchCount > 0 ? bestTaskGroup : nullptr;
}

std::vector<HWND> GetHwndsInTaskGroup(void* taskGroup) {
    std::vector<HWND> hwnds;
    HDPA taskItems = GetTaskItemsArray(taskGroup);
    if (!taskItems) {
        return hwnds;
    }

    for (int i = 0; i < taskItems->cpItems; ++i) {
        void* taskItem = ((void**)taskItems->pArray)[i];
        HWND hwnd = GetHwndFromTaskItem(taskItem);
        if (hwnd) {
            hwnds.push_back(hwnd);
        }
    }

    return hwnds;
}

void* FindTaskGroupForDesktopWindows(
    void* taskListLongPtr,
    const std::vector<HWND>& targetHwnds) {
    HDPA buttonGroups = GetButtonGroupsDpa(taskListLongPtr);
    if (!buttonGroups) {
        return nullptr;
    }

    void* bestTaskGroup = nullptr;
    size_t bestMatchCount = 0;

    for (int i = 0; i < buttonGroups->cpItems; ++i) {
        void* btnGroup = ((void**)buttonGroups->pArray)[i];
        if (!btnGroup) {
            continue;
        }

        int groupType = CTaskBtnGroup_GetGroupType(btnGroup);
        if (groupType != 1 && groupType != 3) {
            continue;
        }

        void* taskGroup = CTaskBtnGroup_GetGroup(btnGroup);
        if (!taskGroup) {
            continue;
        }

        HDPA taskItems = GetTaskItemsArray(taskGroup);
        if (!taskItems) {
            continue;
        }

        size_t matchCount = 0;
        for (HWND targetHwnd : targetHwnds) {
            for (int j = 0; j < taskItems->cpItems; ++j) {
                void* taskItem = ((void**)taskItems->pArray)[j];
                if (GetHwndFromTaskItem(taskItem) == targetHwnd) {
                    ++matchCount;
                    break;
                }
            }
        }

        if (matchCount > bestMatchCount) {
            bestMatchCount = matchCount;
            bestTaskGroup = taskGroup;
        }
    }

    return bestMatchCount > 0 ? bestTaskGroup : nullptr;
}

bool ApplyOrderToTaskGroup(void* taskGroup,
                           const GUID& desktopId,
                           const std::vector<HWND>& targetOrder,
                           bool flyoutOnlyApply) {
    AutoReorder_TraceStep2U(
        L"T60 apply_order_to_group:enter",
        reinterpret_cast<ULONG_PTR>(taskGroup),
        flyoutOnlyApply ? 1 : 0);
    HDPA taskItems = GetTaskItemsArray(taskGroup);
    if (!taskItems || taskItems->cpItems <= 1) {
        Wh_Log(L"Auto-reorder apply: skipping group %p (%d thumbnail(s) in "
               L"group)",
               taskGroup, taskItems ? taskItems->cpItems : 0);
        return true;
    }

    std::vector<int> desktopSlotIndices;
    std::unordered_map<HWND, void*> hwndToTaskItem;

    for (int i = 0; i < taskItems->cpItems; ++i) {
        void* taskItem = ((void**)taskItems->pArray)[i];
        HWND hwnd = GetHwndFromTaskItem(taskItem);
        if (!hwnd) {
            continue;
        }

        GUID windowDesktopId{};
        if (!GetWindowVirtualDesktopIdForHwnd(hwnd, windowDesktopId)) {
            continue;
        }

        if (!IsEqualGUID(windowDesktopId, desktopId)) {
            continue;
        }

        desktopSlotIndices.push_back(i);
        hwndToTaskItem[hwnd] = taskItem;
    }

    if (desktopSlotIndices.size() <= 1) {
        Wh_Log(L"Auto-reorder apply: skipping group %p (<=1 window on desktop)",
               taskGroup);
        return true;
    }

    const std::vector<HWND> flyoutMapped =
        GetFlyoutMappedHwndsForTaskGroup(taskGroup);
    const size_t flyoutMappedCount = flyoutMapped.size();

    if (targetOrder.size() <= 1) {
        Wh_Log(L"Auto-reorder apply: skipping group %p (<=1 window in apply set)",
               taskGroup);
        return true;
    }

    Wh_Log(L"Auto-reorder apply: task group %p — %s %zu window(s) on desktop "
           L"(%zu in flyout mapping)",
           taskGroup,
           flyoutOnlyApply ? L"flyout preview for" : L"reordering",
           desktopSlotIndices.size(), flyoutMappedCount);

    std::vector<void*> desiredTaskItems;
    desiredTaskItems.reserve(desktopSlotIndices.size());

    for (HWND hwnd : targetOrder) {
        auto it = hwndToTaskItem.find(hwnd);
        if (it != hwndToTaskItem.end()) {
            desiredTaskItems.push_back(it->second);
        }
    }

    for (int slotIndex : desktopSlotIndices) {
        void* taskItem = ((void**)taskItems->pArray)[slotIndex];
        HWND hwnd = GetHwndFromTaskItem(taskItem);
        if (!hwnd) {
            continue;
        }

        bool alreadyListed = false;
        for (void* listed : desiredTaskItems) {
            if (listed == taskItem) {
                alreadyListed = true;
                break;
            }
        }

        if (!alreadyListed) {
            desiredTaskItems.push_back(taskItem);
        }
    }

    if (desiredTaskItems.size() != desktopSlotIndices.size()) {
        Wh_Log(L"Auto-reorder apply: task item count mismatch for group %p",
               taskGroup);
        return false;
    }

    const std::vector<int> moveSlotIndices = desktopSlotIndices;

    if (flyoutMappedCount >= 2) {
        AutoReorder_TraceStep2U(L"T62 apply_order_to_group:before_try_xaml",
                                reinterpret_cast<ULONG_PTR>(taskGroup),
                                flyoutMappedCount);
        std::vector<HWND> flyoutVisualOrder =
            BuildFlyoutAlignedApplyOrder(targetOrder, flyoutMapped);
        if (TryApplyXamlFlyoutOrder(taskGroup, flyoutVisualOrder)) {
            Wh_Log(L"Auto-reorder apply: flyout preview updated (%zu of %zu "
                   L"visible thumbnail(s))",
                   flyoutVisualOrder.size(), desktopSlotIndices.size());
        } else {
            QueuePendingXamlFlyoutOrder(taskGroup, flyoutVisualOrder);
        }
    } else if (flyoutOnlyApply && targetOrder.size() >= 2) {
        QueuePendingXamlFlyoutOrder(taskGroup, targetOrder);
    }

    if (flyoutOnlyApply) {
        AutoReorder_TraceStep(L"T64 apply_order_to_group:exit flyout_only");
        return true;
    }

    if (DesktopOrderAlreadyMatches(taskItems, moveSlotIndices,
                                   desiredTaskItems)) {
        Wh_Log(L"Auto-reorder apply: group %p order already correct (idempotent "
               L"skip)",
               taskGroup);
        return true;
    }

    if (!IsTaskbarUiThread()) {
        Wh_Log(L"Auto-reorder apply: internal task-group moves skipped on "
               L"thread %u (taskbar UI is %u); XAML/pending flyout only",
               GetCurrentThreadId(), GetTaskbarUiThreadId());
        return true;
    }

    if (!ApplyMovesToSlotIndices(taskGroup, taskItems, moveSlotIndices,
                                 desiredTaskItems)) {
        return false;
    }

    if (!desiredTaskItems.empty() && AutoReorder_IsXamlFlyoutPreviewActive()) {
        NotifyThumbnailFlyoutReordered(taskGroup, desiredTaskItems.front());
    }

    return true;
}

bool ApplyFlyoutOnlyFromCacheImpl(
    const std::vector<AutoReorderAppRules>& appRules) {
    Wh_Log(L"Auto-reorder apply: flyout-only safe pass (pending/cache only)");
    AutoReorder_TraceStep2U(L"T20 flyout_cache:enter",
                            g_pendingXamlFlyoutOrder.size(), appRules.size());

    std::vector<void*> pendingGroups;
    pendingGroups.reserve(g_pendingXamlFlyoutOrder.size());
    for (const auto& entry : g_pendingXamlFlyoutOrder) {
        pendingGroups.push_back(entry.first);
    }
    bool pendingApplied = false;
    for (void* taskGroup : pendingGroups) {
        AutoReorder_TraceStepU(L"T21 flyout_cache:before_try_pending",
                               reinterpret_cast<ULONG_PTR>(taskGroup));
        if (AutoReorder_TryApplyPendingXamlFlyoutOrder(taskGroup)) {
            pendingApplied = true;
        }
        AutoReorder_TraceStepU(L"T22 flyout_cache:after_try_pending",
                               reinterpret_cast<ULONG_PTR>(taskGroup));
    }

    const bool flyoutOpen = AutoReorder_IsXamlFlyoutPreviewActive();
    if (flyoutOpen && !AutoReorder_FlyoutThumbnailsReady()) {
        Wh_Log(L"Auto-reorder apply: flyout thumbnails not ready yet (waiting "
               L"for preview to finish loading)");
        AutoReorder_TraceStep(L"T23b flyout_cache:exit thumbnails_not_ready");
        return true;
    }

    if (pendingApplied && flyoutOpen) {
        Wh_Log(L"Auto-reorder apply: flyout preview updated from pending queue "
               L"(skipping duplicate cache apply)");
        AutoReorder_TraceStep(L"T32 flyout_cache:exit after_pending_apply");
        return true;
    }

    const bool hasMapping = AutoReorder_HasFlyoutThumbnailMapping();
    AutoReorder_TraceStep2U(L"T23 flyout_cache:flyout_state",
                            flyoutOpen ? 1 : 0, hasMapping ? 1 : 0);
    if (!flyoutOpen && !hasMapping) {
        if (pendingGroups.empty()) {
            Wh_Log(L"Auto-reorder apply: flyout not open; hover the taskbar "
                   L"button to apply");
        }
        return true;
    }

    if (appRules.empty()) {
        AutoReorder_TraceStep(L"T32 flyout_cache:exit no_rules");
        return true;
    }

    AutoReorder_TraceStep(L"T24 flyout_cache:before_enumerate_desktops");
    std::vector<GUID> desktops = EnumerateVirtualDesktops();
    AutoReorder_TraceStep2U(L"T25 flyout_cache:after_enumerate_desktops",
                            desktops.size(), 0);
    if (desktops.empty()) {
        return false;
    }

    bool anyApplied = false;

    for (const GUID& desktopId : desktops) {
        for (const AutoReorderAppRules& appRule : appRules) {
            AutoReorder_TraceStepU(L"T26 flyout_cache:app_loop",
                                   reinterpret_cast<ULONG_PTR>(
                                       appRule.app.c_str()));
            std::vector<HWND> targetOrder;
            if (!AutoReorder_TryGetCachedDesktopOrder(appRule.app, desktopId,
                                                      targetOrder) ||
                targetOrder.size() <= 1) {
                continue;
            }

            AutoReorder_TraceStep(L"T27 flyout_cache:before_resolve_task_group");
            void* taskGroup = g_cachedTaskGroupFromFlyout;
            if (!taskGroup) {
                taskGroup = FindTaskGroupViaThumbnailMapping(targetOrder);
            }

            AutoReorder_TraceStepU(L"T28 flyout_cache:task_group",
                                   reinterpret_cast<ULONG_PTR>(taskGroup));

            if (!taskGroup) {
                Wh_Log(L"Auto-reorder apply: no cached task group for %s "
                       L"(hover the taskbar button)",
                       appRule.app.c_str());
                continue;
            }

            const std::vector<HWND> flyoutMapped =
                GetFlyoutMappedHwndsForTaskGroup(taskGroup);
            AutoReorder_TraceStep2U(L"T29 flyout_cache:flyout_mapped_count",
                                    reinterpret_cast<ULONG_PTR>(taskGroup),
                                    flyoutMapped.size());
            if (flyoutMapped.size() < 2) {
                QueuePendingXamlFlyoutOrder(taskGroup, targetOrder);
                continue;
            }

            const std::vector<HWND> flyoutOrder =
                BuildFlyoutAlignedApplyOrder(targetOrder, flyoutMapped);
            if (flyoutOrder.size() < 2) {
                continue;
            }

            AutoReorder_TraceStep(L"T30 flyout_cache:before_try_xaml");
            if (TryApplyXamlFlyoutOrder(taskGroup, flyoutOrder)) {
                AutoReorder_TraceStep(L"T31 flyout_cache:after_try_xaml ok");
                anyApplied = true;
                Wh_Log(L"Auto-reorder apply: flyout preview updated from cache "
                       L"for %s (%zu thumbnail(s))",
                       appRule.app.c_str(), flyoutOrder.size());
            } else {
                AutoReorder_TraceStep(L"T31 flyout_cache:after_try_xaml fail");
                QueuePendingXamlFlyoutOrder(taskGroup, flyoutOrder);
            }
        }
    }

    if (!anyApplied && pendingGroups.empty()) {
        Wh_Log(L"Auto-reorder apply: flyout safe pass made no changes yet");
    }

    AutoReorder_TraceStep(L"T32 flyout_cache:exit");
    return true;
}

}  // namespace

std::atomic<bool> g_deferApplyUntilFlyoutMapping{false};
std::atomic<bool> g_pendingSessionApply{false};

bool AutoReorder_ShouldDeferApply() {
    return g_deferApplyUntilFlyoutMapping.load();
}

namespace {

void QueuePendingFromEngineCache() {
    if (!g_rulesValid || g_rules.apps.empty()) {
        return;
    }

    std::vector<GUID> desktops = EnumerateVirtualDesktops();
    bool queued = false;

    for (const AutoReorderAppRules& appRule : g_rules.apps) {
        for (const GUID& desktopId : desktops) {
            std::vector<HWND> targetOrder;
            if (!AutoReorder_TryGetCachedDesktopOrder(appRule.app, desktopId,
                                                      targetOrder) ||
                targetOrder.size() <= 1) {
                continue;
            }

            void* taskGroup = g_cachedTaskGroupFromFlyout;
            if (!taskGroup) {
                taskGroup = FindTaskGroupViaThumbnailMapping(targetOrder);
            }

            if (!taskGroup) {
                continue;
            }

            const std::vector<HWND> flyoutMapped =
                GetFlyoutMappedHwndsForTaskGroup(taskGroup);
            std::vector<HWND> flyoutOrder = targetOrder;
            if (flyoutMapped.size() >= 2) {
                flyoutOrder =
                    BuildFlyoutAlignedApplyOrder(targetOrder, flyoutMapped);
            }

            if (flyoutOrder.size() >= 2) {
                QueuePendingXamlFlyoutOrder(taskGroup, flyoutOrder);
                queued = true;
            }
        }
    }

    if (queued) {
        Wh_Log(L"Auto-reorder apply: queued engine order for next flyout open");
    }
}

void MarkSessionApplyPending() {
    g_pendingSessionApply = true;
    QueuePendingFromEngineCache();

    if (!g_cachedTaskGroupFromFlyout &&
        g_pendingXamlFlyoutOrder.empty()) {
        Wh_Log(L"Auto-reorder apply: session apply pending (hover the app's "
               L"taskbar button to apply)");
        return;
    }

    Wh_Log(L"Auto-reorder apply: session apply pending (hover the taskbar "
           L"button if the preview was closed)");
}

}  // namespace

void AutoReorder_QueuePendingFromEngineCache() {
    QueuePendingFromEngineCache();
}

bool AutoReorder_ApplyFlyoutOnlyFromCache(
    const std::vector<AutoReorderAppRules>& appRules) {
    AutoReorder_TraceStep(L"T19 flyout_cache_wrapper:enter");
    const bool ok = ApplyFlyoutOnlyFromCacheImpl(appRules);
    AutoReorder_TraceStep(L"T19b flyout_cache_wrapper:exit");
    return ok;
}

bool RunAutoReorderApplyOnTaskbarThread(
    const std::vector<AutoReorderAppRules>& appRules,
    bool fromDeferredTimer) {
    AutoReorder_TraceStep2U(L"T10 run_apply:enter", fromDeferredTimer ? 1 : 0,
                            appRules.size());
    AutoReorder_TraceStep(L"T11 run_apply:before_mutex");
    std::lock_guard<std::mutex> lock(g_autoReorderPassMutex);
    AutoReorder_TraceStep(L"T11b run_apply:mutex_acquired");

    bool expected = false;
    if (!g_autoReorderPassRunning.compare_exchange_strong(expected, true)) {
        AutoReorder_TraceStep(L"T12 run_apply:pass_already_running_skip");
        Wh_Log(L"Auto-reorder apply: pass already in progress, skipped");
        return false;
    }

    struct PassRunningGuard {
        ~PassRunningGuard() { g_autoReorderPassRunning = false; }
    } runningGuard;

    g_deferApplyUntilFlyoutMapping = false;

    if (!fromDeferredTimer) {
        AutoReorder_TraceStep(L"T13 run_apply:queue_deferred_path");
        Wh_Log(L"Auto-reorder apply: queuing flyout-only apply (open or hover "
               L"the taskbar preview)");
        MarkSessionApplyPending();
        ScheduleDeferredAutoReorderApply();
        AutoReorder_TraceStep(L"T13b run_apply:queue_deferred_path_done");
        return true;
    }

    AutoReorder_TraceStep(L"T14 run_apply:before_flyout_cache");
    const bool ok = AutoReorder_ApplyFlyoutOnlyFromCache(appRules);
    AutoReorder_TraceStep(L"T15 run_apply:after_flyout_cache");
    return ok;
}

bool AutoReorder_ApplyReorders(
    const std::vector<AutoReorderAppRules>& appRules,
    bool flyoutOnlyApply) {
    AutoReorder_TraceStep2U(L"T40 full_apply:enter", flyoutOnlyApply ? 1 : 0,
                            appRules.size());
    if (appRules.empty()) {
        return true;
    }

    if (!TaskbarSymbolsReady()) {
        Wh_Log(L"Auto-reorder apply: taskbar symbols not available");
        return false;
    }

    std::vector<GUID> desktops = EnumerateVirtualDesktops();
    if (desktops.empty()) {
        Wh_Log(L"Auto-reorder apply: no virtual desktops to process");
        return false;
    }

    bool anyApplied = false;

    for (size_t desktopIndex = 0; desktopIndex < desktops.size();
         ++desktopIndex) {
        const GUID& desktopId = desktops[desktopIndex];

        for (const AutoReorderAppRules& appRule : appRules) {
            AutoReorder_TraceStepU(L"T41 full_apply:before_get_windows",
                                   reinterpret_cast<ULONG_PTR>(
                                       appRule.app.c_str()));
            std::vector<HWND> windowHandles =
                GetWindowsForAppOnDesktop(appRule.app, desktopId);
            AutoReorder_TraceStep2U(L"T42 full_apply:after_get_windows",
                                    windowHandles.size(), desktopIndex);
            if (windowHandles.empty()) {
                Wh_Log(L"Auto-reorder apply: desktop[%zu] %s has 0 visible "
                       L"window(s), skipping",
                       desktopIndex, appRule.app.c_str());
                continue;
            }

            std::vector<HWND> targetOrder;
            if (!AutoReorder_TryGetCachedDesktopOrder(appRule.app, desktopId,
                                                      targetOrder)) {
                Wh_Log(L"Auto-reorder apply: no cached order for %s on "
                       L"desktop[%zu] (run engine pass first)",
                       appRule.app.c_str(), desktopIndex);
                continue;
            }

            if (targetOrder.size() <= 1) {
                Wh_Log(L"Auto-reorder apply: cached order too small for %s",
                       appRule.app.c_str());
                continue;
            }

            AutoReorder_TraceStep(L"T43 full_apply:before_find_task_group");
            void* taskGroup = FindTaskGroupForTargetHwnds(windowHandles);
            AutoReorder_TraceStepU(L"T44 full_apply:after_find_task_group",
                                   reinterpret_cast<ULONG_PTR>(taskGroup));
            if (!taskGroup && g_cachedTaskGroupFromFlyout &&
                GetFlyoutMappedHwndsForTaskGroup(g_cachedTaskGroupFromFlyout)
                        .size() >= 2) {
                taskGroup = g_cachedTaskGroupFromFlyout;
            }

            if (!taskGroup) {
                Wh_Log(L"Auto-reorder apply: no task group for %s on "
                       L"desktop[%zu] (%zu candidate window(s))",
                       appRule.app.c_str(), desktopIndex,
                       windowHandles.size());
                continue;
            }

            const size_t flyoutMappedCount =
                GetFlyoutMappedHwndsForTaskGroup(taskGroup).size();

            AutoReorder_TraceStep(L"T45 full_apply:before_apply_order_to_group");
            if (!ApplyOrderToTaskGroup(taskGroup, desktopId, targetOrder,
                                       flyoutOnlyApply)) {
                Wh_Log(L"Auto-reorder apply: failed for %s on desktop[%zu]",
                       appRule.app.c_str(), desktopIndex);
                continue;
            }
            AutoReorder_TraceStep(L"T46 full_apply:after_apply_order_to_group");

            anyApplied = true;
            Wh_Log(L"Auto-reorder apply: apply pass done for %s on desktop[%zu] "
                   L"(%zu in cached order, %zu in flyout mapping)",
                   appRule.app.c_str(), desktopIndex, targetOrder.size(),
                   flyoutMappedCount);
        }
    }

    if (!anyApplied) {
        Wh_Log(L"Auto-reorder apply: no groups updated");
    }

    AutoReorder_TraceStep(L"T47 full_apply:exit");
    return true;
}

void AutoReorder_UninstallTraySubclass() {}

std::optional<bool> IsOsFeatureEnabled(UINT32 featureId) {
    enum FEATURE_ENABLED_STATE {
        FEATURE_ENABLED_STATE_DEFAULT = 0,
        FEATURE_ENABLED_STATE_DISABLED = 1,
        FEATURE_ENABLED_STATE_ENABLED = 2,
    };

#pragma pack(push, 1)
    struct RTL_FEATURE_CONFIGURATION {
        unsigned int featureId;
        unsigned __int32 group : 4;
        FEATURE_ENABLED_STATE enabledState : 2;
        unsigned __int32 enabledStateOptions : 1;
        unsigned __int32 unused1 : 1;
        unsigned __int32 variant : 6;
        unsigned __int32 variantPayloadKind : 2;
        unsigned __int32 unused2 : 16;
        unsigned int payload;
    };
#pragma pack(pop)

    using RtlQueryFeatureConfiguration_t =
        int(NTAPI*)(UINT32, int, INT64*, RTL_FEATURE_CONFIGURATION*);
    static RtlQueryFeatureConfiguration_t pRtlQueryFeatureConfiguration = []() {
        HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
        return hNtDll ? (RtlQueryFeatureConfiguration_t)GetProcAddress(
                            hNtDll, "RtlQueryFeatureConfiguration")
                      : nullptr;
    }();

    if (!pRtlQueryFeatureConfiguration) {
        Wh_Log(L"RtlQueryFeatureConfiguration not found");
        return std::nullopt;
    }

    RTL_FEATURE_CONFIGURATION feature = {0};
    INT64 changeStamp = 0;
    HRESULT hr =
        pRtlQueryFeatureConfiguration(featureId, 1, &changeStamp, &feature);
    if (SUCCEEDED(hr)) {
        Wh_Log(L"RtlQueryFeatureConfiguration result for %u: %d", featureId,
               feature.enabledState);

        switch (feature.enabledState) {
            case FEATURE_ENABLED_STATE_DISABLED:
                return false;
            case FEATURE_ENABLED_STATE_ENABLED:
                return true;
            case FEATURE_ENABLED_STATE_DEFAULT:
                return std::nullopt;
        }
    } else {
        Wh_Log(L"RtlQueryFeatureConfiguration error for %u: %08X", featureId,
               hr);
    }

    return std::nullopt;
}

BOOL Wh_ModInit() {
    Wh_Log(L">");

    LoadSettings();

    g_winVersion = GetExplorerVersion();
    if (g_winVersion == WinVersion::Unsupported) {
        Wh_Log(L"Unsupported Windows version");
        return FALSE;
    }

    if (g_settings.oldTaskbarOnWin11) {
        bool hasWin10Taskbar = g_winVersion < WinVersion::Win11_24H2;

        if (g_winVersion >= WinVersion::Win11) {
            g_winVersion = WinVersion::Win10;
        }

        if (hasWin10Taskbar && !HookTaskbarSymbols()) {
            return FALSE;
        }
    } else if (g_winVersion >= WinVersion::Win11) {
        if (!HookTaskbarSymbols()) {
            return FALSE;
        }

        if (HMODULE taskbarViewModule = GetTaskbarViewModuleHandle()) {
            g_taskbarViewDllLoaded = true;
            if (!HookTaskbarViewDllSymbols(taskbarViewModule)) {
                return FALSE;
            }
        } else {
            Wh_Log(L"Taskbar view module not loaded yet");
        }

        constexpr UINT kThumbnailAnimations = 60511437;
        if (g_noClassicThumbnails &&
            IsOsFeatureEnabled(kThumbnailAnimations).value_or(true)) {
            g_hasThumbnailAnimations = true;
            Wh_Log(L"Thumbnail animations are enabled");
        }
    } else {
        if (!HookTaskbarSymbols()) {
            return FALSE;
        }
    }

    if (!HandleLoadedExplorerPatcher()) {
        Wh_Log(L"HandleLoadedExplorerPatcher failed");
        return FALSE;
    }

    HMODULE kernelBaseModule = GetModuleHandle(L"kernelbase.dll");
    auto pKernelBaseLoadLibraryExW = (decltype(&LoadLibraryExW))GetProcAddress(
        kernelBaseModule, "LoadLibraryExW");
    WindhawkUtils::Wh_SetFunctionHookT(pKernelBaseLoadLibraryExW,
                                       LoadLibraryExW_Hook,
                                       &LoadLibraryExW_Original);

    WindhawkUtils::Wh_SetFunctionHookT(DPA_GetPtr, DPA_GetPtr_Hook,
                                       &DPA_GetPtr_Original);

    g_initialized = true;

    return TRUE;
}

void Wh_ModAfterInit() {
    Wh_Log(L">");

    if (g_winVersion >= WinVersion::Win11 && !g_taskbarViewDllLoaded) {
        if (HMODULE taskbarViewModule = GetTaskbarViewModuleHandle()) {
            if (!g_taskbarViewDllLoaded.exchange(true)) {
                Wh_Log(L"Got Taskbar.View.dll");

                if (HookTaskbarViewDllSymbols(taskbarViewModule)) {
                    Wh_ApplyHookOperations();
                }
            }
        }
    }

    // Try again in case there's a race between the previous attempt and the
    // LoadLibraryExW hook.
    if (!g_explorerPatcherInitialized) {
        HandleLoadedExplorerPatcher();
    }

    StartAutoReorderTimerIfNeeded();
    UpdateReorderListener();
}

void Wh_ModUninit() {
    Wh_Log(L">");

    CancelAutoReorderTimer();
    CancelDeferredApplyTimer();
    AutoReorder_UninstallTraySubclass();
    StopReorderListenerThread();
    AutoReorderUia_Uninit();
    AutoReorderEnumeration_Uninit();
}

BOOL Wh_ModSettingsChanged(BOOL* bReload) {
    Wh_Log(L">");

    bool prevOldTaskbarOnWin11 = g_settings.oldTaskbarOnWin11;

    LoadSettings();

    UpdateReorderListener();

    if (g_autoReorderDone.load()) {
        // Re-parse rules on settings change, but do not schedule another auto-reorder
        // pass in the same Explorer session (g_autoReorderDone remains set).
        Wh_Log(L"Auto-reorder: settings updated; session pass already done, "
               L"not re-running until Explorer/mod reload");
    }

    *bReload = g_settings.oldTaskbarOnWin11 != prevOldTaskbarOnWin11;

    return TRUE;
}
