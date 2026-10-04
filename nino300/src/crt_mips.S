/*
 * Ftaghn: Cosmic Horror Ataxx - Philips Nino 300 (Windows CE 2.0 MIPS R4000)
 * Runtime Startup & COREDLL.DLL Import Thunks
 */

    .set noreorder
    .set noat

    .text
    .globl WinMainCRTStartup
    .ent WinMainCRTStartup
WinMainCRTStartup:
    /* Allocate stack frame */
    addiu   $sp, $sp, -32
    sw      $ra, 28($sp)
    sw      $s0, 24($sp)

    /* Call high-level C entry point */
    jal     WinMainStartupHelper
    nop

    /* Move return code to $a0 for ExitProcess */
    move    $a0, $v0
    jal     ExitProcess
    nop

    /* Fallback exit if ExitProcess returns */
    lw      $s0, 24($sp)
    lw      $ra, 28($sp)
    addiu   $sp, $sp, 32
    jr      $ra
    nop
    .end WinMainCRTStartup

/*
 * MIPS Import Thunk Generator Macro
 * Loads resolved function address from IAT pointer (__imp_FuncName)
 * and executes tail-jump with zero register corruption.
 */
#define DECLARE_THUNK(func) \
    .globl func; \
    .ent func; \
func: \
    lui     $25, %hi(__imp_##func); \
    lw      $25, %lo(__imp_##func)($25); \
    jr      $25; \
    nop; \
    .end func;

    DECLARE_THUNK(RegisterClassW)
    DECLARE_THUNK(CreateWindowExW)
    DECLARE_THUNK(DefWindowProcW)
    DECLARE_THUNK(ShowWindow)
    DECLARE_THUNK(UpdateWindow)
    DECLARE_THUNK(GetMessageW)
    DECLARE_THUNK(TranslateMessage)
    DECLARE_THUNK(DispatchMessageW)
    DECLARE_THUNK(PostQuitMessage)
    DECLARE_THUNK(DestroyWindow)
    DECLARE_THUNK(BeginPaint)
    DECLARE_THUNK(EndPaint)
    DECLARE_THUNK(SetTimer)
    DECLARE_THUNK(KillTimer)
    DECLARE_THUNK(Sleep)
    DECLARE_THUNK(GetModuleHandleW)
    DECLARE_THUNK(CreateCompatibleDC)
    DECLARE_THUNK(DeleteDC)
    DECLARE_THUNK(CreateDIBSection)
    DECLARE_THUNK(SelectObject)
    DECLARE_THUNK(DeleteObject)
    DECLARE_THUNK(BitBlt)
    DECLARE_THUNK(InvalidateRect)
    DECLARE_THUNK(LoadLibraryW)
    DECLARE_THUNK(GetProcAddress)
    DECLARE_THUNK(FreeLibrary)
    DECLARE_THUNK(CreateFileW)
    DECLARE_THUNK(CloseHandle)
    DECLARE_THUNK(GetTickCount)
    DECLARE_THUNK(GetModuleFileNameW)
    DECLARE_THUNK(ExitProcess)

/*
 * Import Address Table (IAT) storage in .data
 * The Windows CE loader will populate these pointers with actual COREDLL addresses.
 */
    .data
    .align 2
    .globl s_iat_start
s_iat_start:

#define DECLARE_POINTER(func) \
    .globl __imp_##func; \
__imp_##func: \
    .4byte 0;

    DECLARE_POINTER(RegisterClassW)
    DECLARE_POINTER(CreateWindowExW)
    DECLARE_POINTER(DefWindowProcW)
    DECLARE_POINTER(ShowWindow)
    DECLARE_POINTER(UpdateWindow)
    DECLARE_POINTER(GetMessageW)
    DECLARE_POINTER(TranslateMessage)
    DECLARE_POINTER(DispatchMessageW)
    DECLARE_POINTER(PostQuitMessage)
    DECLARE_POINTER(DestroyWindow)
    DECLARE_POINTER(BeginPaint)
    DECLARE_POINTER(EndPaint)
    DECLARE_POINTER(SetTimer)
    DECLARE_POINTER(KillTimer)
    DECLARE_POINTER(Sleep)
    DECLARE_POINTER(GetModuleHandleW)
    DECLARE_POINTER(CreateCompatibleDC)
    DECLARE_POINTER(DeleteDC)
    DECLARE_POINTER(CreateDIBSection)
    DECLARE_POINTER(SelectObject)
    DECLARE_POINTER(DeleteObject)
    DECLARE_POINTER(BitBlt)
    DECLARE_POINTER(InvalidateRect)
    DECLARE_POINTER(LoadLibraryW)
    DECLARE_POINTER(GetProcAddress)
    DECLARE_POINTER(FreeLibrary)
    DECLARE_POINTER(CreateFileW)
    DECLARE_POINTER(CloseHandle)
    DECLARE_POINTER(GetTickCount)
    DECLARE_POINTER(GetModuleFileNameW)
    DECLARE_POINTER(ExitProcess)

    .globl s_iat_end
s_iat_end:
    .4byte 0
