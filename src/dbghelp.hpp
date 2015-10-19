#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <DbgHelp.h>
#include "criticalsection.h"

class DgbHelp
{
public:
    DgbHelp() {
        m_lock.Initialize();
        m_lock1.Initialize();
        m_lock2.Initialize();
        m_lock3.Initialize();
    }
    ~DgbHelp() {
        m_lock.Delete();
        m_lock1.Delete();
        m_lock2.Delete();
        m_lock3.Delete();
    }
    BOOL IsLockedByCurrentThread() {
        return
            m_lock.IsLockedByCurrentThread() ||
            m_lock1.IsLockedByCurrentThread() ||
            m_lock2.IsLockedByCurrentThread() ||
            m_lock3.IsLockedByCurrentThread();
    }
    BOOL SymInitializeW(_In_ HANDLE hProcess, _In_opt_ PCWSTR UserSearchPath, _In_ BOOL fInvadeProcess) {
        CriticalSectionLocker cs(m_lock);
        return ::SymInitializeW(hProcess, UserSearchPath, fInvadeProcess);
    }
    BOOL SymCleanup(_In_ HANDLE hProcess) {
        CriticalSectionLocker cs(m_lock);
        return ::SymCleanup(hProcess);
    }
    DWORD SymSetOptions(__in DWORD SymOptions) {
        CriticalSectionLocker cs(m_lock);
        return ::SymSetOptions(SymOptions);
    }
    BOOL SymFromAddrW(_In_ HANDLE hProcess, _In_ DWORD64 Address, _Out_opt_ PDWORD64 Displacement, _Inout_ PSYMBOL_INFOW Symbol) {
        CriticalSectionLocker cs(m_lock);
        return ::SymFromAddrW(hProcess, Address, Displacement, Symbol);
    }
    BOOL SymGetLineFromAddrW64(_In_ HANDLE hProcess, _In_ DWORD64 dwAddr, _Out_ PDWORD pdwDisplacement, _Out_ PIMAGEHLP_LINEW64 Line) {
        CriticalSectionLocker cs(m_lock);
        return ::SymGetLineFromAddrW64(hProcess, dwAddr, pdwDisplacement, Line);
    }
    BOOL SymGetModuleInfoW64(_In_ HANDLE hProcess, _In_ DWORD64 qwAddr, _Out_ PIMAGEHLP_MODULEW64 ModuleInfo) {
        CriticalSectionLocker cs(m_lock);
        return ::SymGetModuleInfoW64(hProcess, qwAddr, ModuleInfo);
    }
    DWORD64 SymLoadModuleExW(_In_ HANDLE hProcess, _In_opt_ HANDLE hFile, _In_opt_ PCWSTR ImageName, _In_opt_ PCWSTR ModuleName, _In_ DWORD64 BaseOfDll, _In_ DWORD DllSize, _In_opt_ PMODLOAD_DATA Data, _In_opt_ DWORD Flags) {
        CriticalSectionLocker cs(m_lock);
        return ::SymLoadModuleExW(hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize, Data, Flags);
    }
    BOOL SymUnloadModule64(_In_ HANDLE hProcess, _In_ DWORD64 BaseOfDll) {
        CriticalSectionLocker cs(m_lock);
        return ::SymUnloadModule64(hProcess, BaseOfDll);
    }
    BOOL StackWalk64(__in DWORD MachineType, __in HANDLE hProcess, __in HANDLE hThread, __inout LPSTACKFRAME64 StackFrame, __inout PVOID ContextRecord, __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress) {
        CriticalSectionLocker cs(m_lock1);
        return ::StackWalk64(MachineType, hProcess, hThread, StackFrame, ContextRecord, ReadMemoryRoutine, FunctionTableAccessRoutine, GetModuleBaseRoutine, TranslateAddress);
    }
    PVOID ImageDirectoryEntryToDataEx(__in PVOID Base, __in BOOLEAN MappedAsImage, __in USHORT DirectoryEntry, __out PULONG Size, __out_opt PIMAGE_SECTION_HEADER *FoundHeader) {
        CriticalSectionLocker cs(m_lock2);
        return ::ImageDirectoryEntryToDataEx(Base, MappedAsImage, DirectoryEntry, Size, FoundHeader);
    }
    BOOL EnumerateLoadedModulesW64(__in HANDLE hProcess, __in PENUMLOADED_MODULES_CALLBACKW64 EnumLoadedModulesCallback, __in_opt PVOID UserContext) {
        CriticalSectionLocker cs(m_lock3);
        return ::EnumerateLoadedModulesW64(hProcess, EnumLoadedModulesCallback, UserContext);
    }
private:
    // Disallow certain operations
    DgbHelp(const DgbHelp&);
    DgbHelp& operator=(const DgbHelp&);

private:
    CriticalSection m_lock;
    CriticalSection m_lock1;
    CriticalSection m_lock2;
    CriticalSection m_lock3;
};
