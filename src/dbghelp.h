#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <DbgHelp.h>
#include "criticalsection.h"

class DbgHelp
{
public:
    DbgHelp() {
        m_lock.Initialize();
    }
    ~DbgHelp() {
        m_lock.Delete();
    }
    void Enter()
    {
        m_lock.Enter();
    }
    void Leave()
    {
        m_lock.Leave();
    }
    BOOL IsLockedByCurrentThread() {
        return
            m_lock.IsLockedByCurrentThread();
    }
    BOOL SymInitializeW(_In_ HANDLE hProcess, _In_opt_ PCWSTR UserSearchPath, _In_ BOOL fInvadeProcess, CriticalSectionLocker<DbgHelp>&) {
        return ::SymInitializeW(hProcess, UserSearchPath, fInvadeProcess);
    }
    BOOL SymInitializeW(_In_ HANDLE hProcess, _In_opt_ PCWSTR UserSearchPath, _In_ BOOL fInvadeProcess) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::SymInitializeW(hProcess, UserSearchPath, fInvadeProcess);
    }
    BOOL SymCleanup(_In_ HANDLE hProcess, CriticalSectionLocker<DbgHelp>&) {
        return ::SymCleanup(hProcess);
    }
    BOOL SymCleanup(_In_ HANDLE hProcess) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::SymCleanup(hProcess);
    }
    DWORD SymSetOptions(__in DWORD SymOptions, CriticalSectionLocker<DbgHelp>&) {
        return ::SymSetOptions(SymOptions);
    }
    DWORD SymSetOptions(__in DWORD SymOptions) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::SymSetOptions(SymOptions);
    }
    BOOL SymFromAddrW(_In_ HANDLE hProcess, _In_ DWORD64 Address, _Out_opt_ PDWORD64 Displacement, _Inout_ PSYMBOL_INFOW Symbol, CriticalSectionLocker<DbgHelp>&) {
        return ::SymFromAddrW(hProcess, Address, Displacement, Symbol);
    }
    BOOL SymFromAddrW(_In_ HANDLE hProcess, _In_ DWORD64 Address, _Out_opt_ PDWORD64 Displacement, _Inout_ PSYMBOL_INFOW Symbol) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::SymFromAddrW(hProcess, Address, Displacement, Symbol);
    }
    BOOL SymGetLineFromAddrW64(_In_ HANDLE hProcess, _In_ DWORD64 dwAddr, _Out_ PDWORD pdwDisplacement, _Out_ PIMAGEHLP_LINEW64 Line, CriticalSectionLocker<DbgHelp>&) {
        return ::SymGetLineFromAddrW64(hProcess, dwAddr, pdwDisplacement, Line);
    }
    BOOL SymGetLineFromAddrW64(_In_ HANDLE hProcess, _In_ DWORD64 dwAddr, _Out_ PDWORD pdwDisplacement, _Out_ PIMAGEHLP_LINEW64 Line) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::SymGetLineFromAddrW64(hProcess, dwAddr, pdwDisplacement, Line);
    }
    BOOL SymGetModuleInfoW64(_In_ HANDLE hProcess, _In_ DWORD64 qwAddr, _Out_ PIMAGEHLP_MODULEW64 ModuleInfo, CriticalSectionLocker<DbgHelp>&) {
        return ::SymGetModuleInfoW64(hProcess, qwAddr, ModuleInfo);
    }
    BOOL SymGetModuleInfoW64(_In_ HANDLE hProcess, _In_ DWORD64 qwAddr, _Out_ PIMAGEHLP_MODULEW64 ModuleInfo) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::SymGetModuleInfoW64(hProcess, qwAddr, ModuleInfo);
    }
    DWORD64 SymLoadModuleExW(_In_ HANDLE hProcess, _In_opt_ HANDLE hFile, _In_opt_ PCWSTR ImageName, _In_opt_ PCWSTR ModuleName, _In_ DWORD64 BaseOfDll, _In_ DWORD DllSize, _In_opt_ PMODLOAD_DATA Data, _In_opt_ DWORD Flags, CriticalSectionLocker<DbgHelp>&) {
        return ::SymLoadModuleExW(hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize, Data, Flags);
    }
    DWORD64 SymLoadModuleExW(_In_ HANDLE hProcess, _In_opt_ HANDLE hFile, _In_opt_ PCWSTR ImageName, _In_opt_ PCWSTR ModuleName, _In_ DWORD64 BaseOfDll, _In_ DWORD DllSize, _In_opt_ PMODLOAD_DATA Data, _In_opt_ DWORD Flags) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::SymLoadModuleExW(hProcess, hFile, ImageName, ModuleName, BaseOfDll, DllSize, Data, Flags);
    }
    BOOL SymUnloadModule64(_In_ HANDLE hProcess, _In_ DWORD64 BaseOfDll, CriticalSectionLocker<DbgHelp>&) {
        return ::SymUnloadModule64(hProcess, BaseOfDll);
    }
    BOOL SymUnloadModule64(_In_ HANDLE hProcess, _In_ DWORD64 BaseOfDll) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::SymUnloadModule64(hProcess, BaseOfDll);
    }
    BOOL StackWalk64(__in DWORD MachineType, __in HANDLE hProcess, __in HANDLE hThread,
        __inout LPSTACKFRAME64 StackFrame, __inout PVOID ContextRecord,
        __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
        __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
        __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
        __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress, CriticalSectionLocker<DbgHelp>&)
    {
        return ::StackWalk64(MachineType, hProcess, hThread, StackFrame, ContextRecord, ReadMemoryRoutine,
            FunctionTableAccessRoutine, GetModuleBaseRoutine, TranslateAddress);
    }
    BOOL StackWalk64(__in DWORD MachineType, __in HANDLE hProcess, __in HANDLE hThread,
        __inout LPSTACKFRAME64 StackFrame, __inout PVOID ContextRecord,
        __in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
        __in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
        __in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
        __in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress)
    {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::StackWalk64(MachineType, hProcess, hThread, StackFrame, ContextRecord, ReadMemoryRoutine,
            FunctionTableAccessRoutine, GetModuleBaseRoutine, TranslateAddress);
    }
private:
    // Disallow certain operations
    DbgHelp(const DbgHelp&);
    DbgHelp& operator=(const DbgHelp&);

private:
    CriticalSection m_lock;
};

class ImageDirectoryEntries
{
public:
    ImageDirectoryEntries() {
        m_lock.Initialize();
    }
    ~ImageDirectoryEntries() {
        m_lock.Delete();
    }
    void Enter()
    {
        m_lock.Enter();
    }
    void Leave()
    {
        m_lock.Leave();
    }
    BOOL IsLockedByCurrentThread() {
        return
            m_lock.IsLockedByCurrentThread();
    }
    PVOID ImageDirectoryEntryToDataEx(__in PVOID Base, __in BOOLEAN MappedAsImage, __in USHORT DirectoryEntry, __out PULONG Size, __out_opt PIMAGE_SECTION_HEADER *FoundHeader) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::ImageDirectoryEntryToDataEx(Base, MappedAsImage, DirectoryEntry, Size, FoundHeader);
    }
private:
    // Disallow certain operations
    ImageDirectoryEntries(const ImageDirectoryEntries&);
    ImageDirectoryEntries& operator=(const ImageDirectoryEntries&);

private:
    CriticalSection m_lock;
};

class LoadedModules
{
public:
    LoadedModules() {
        m_lock.Initialize();
    }
    ~LoadedModules() {
        m_lock.Delete();
    }
    void Enter()
    {
        m_lock.Enter();
    }
    void Leave()
    {
        m_lock.Leave();
    }
    BOOL IsLockedByCurrentThread() {
        return
            m_lock.IsLockedByCurrentThread();
    }
    BOOL EnumerateLoadedModulesW64(__in HANDLE hProcess, __in PENUMLOADED_MODULES_CALLBACKW64 EnumLoadedModulesCallback, __in_opt PVOID UserContext) {
        CriticalSectionLocker<CriticalSection> cs(m_lock);
        return ::EnumerateLoadedModulesW64(hProcess, EnumLoadedModulesCallback, UserContext);
    }
private:
    // Disallow certain operations
    LoadedModules(const LoadedModules&);
    LoadedModules& operator=(const LoadedModules&);

private:
    CriticalSection m_lock;
};
