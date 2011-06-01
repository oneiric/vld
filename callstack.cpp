////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - CallStack Class Implementations
//  Copyright (c) 2005-2011 Dan Moulding, Arkadiy Shapkin, Laurent Lessieux
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
//  See COPYING.txt for the full terms of the GNU Lesser General Public License.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#define VLDBUILD
#include "callstack.h"  // This class' header.
#include "utility.h"    // Provides various utility functions.
#include "vldheap.h"    // Provides internal new and delete operators.
#include "vldint.h"     // Provides access to VLD internals.

#define MAXSYMBOLNAMELENGTH 256

// Imported global variables.
extern HANDLE             g_currentprocess;
extern HANDLE             g_currentthread;
extern CriticalSection    g_stackwalklock;
extern CriticalSection    g_symbollock;
extern VisualLeakDetector vld;

// Constructor - Initializes the CallStack with an initial size of zero and one
//   Chunk of capacity.
//
CallStack::CallStack ()
{
	m_capacity   = CALLSTACKCHUNKSIZE;
	m_size       = 0;
	m_status     = 0x0;
	m_store.next = NULL;
	m_topchunk   = &m_store;
	m_topindex   = 0;
	m_Resolved   = NULL;
	m_ResolvedCapacity   = 0;
	m_ResolvedLength = 0;
}

// Destructor - Frees all memory allocated to the CallStack.
//
CallStack::~CallStack ()
{
	CallStack::chunk_t *chunk = m_store.next;
	CallStack::chunk_t *temp;

	while (chunk) {
		temp = chunk;
		chunk = temp->next;
		delete temp;
	}

	if (m_Resolved)
	{
		delete [] m_Resolved;
	}

	m_Resolved = NULL;
	m_ResolvedCapacity = 0;
	m_ResolvedLength = 0;
}

CallStack* CallStack::Create()
{
	CallStack* result = NULL;
	if (vld.GetOptions() & VLD_OPT_SAFE_STACK_WALK) {
		result = new SafeCallStack();
	}
	else {
		result = new FastCallStack();
	}
	return result;
	
}
// operator == - Equality operator. Compares the CallStack to another CallStack
//   for equality. Two CallStacks are equal if they are the same size and if
//   every frame in each is identical to the corresponding frame in the other.
//
//  other (IN) - Reference to the CallStack to compare the current CallStack
//    against for equality.
//
//  Return Value:
//
//    Returns true if the two CallStacks are equal. Otherwise returns false.
//
BOOL CallStack::operator == (const CallStack &other) const
{
	if (m_size != other.m_size) {
		// They can't be equal if the sizes are different.
		return FALSE;
	}

	// Walk the chunk list and within each chunk walk the frames array until we
	// either find a mismatch, or until we reach the end of the call stacks.
	const CallStack::chunk_t *prevchunk = NULL;
	const CallStack::chunk_t *chunk = &m_store;
	const CallStack::chunk_t *otherchunk = &other.m_store;
	while (prevchunk != m_topchunk) {
		for (UINT32 index = 0; index < ((chunk == m_topchunk) ? m_topindex : CALLSTACKCHUNKSIZE); index++) {
			if (chunk->frames[index] != otherchunk->frames[index]) {
				// Found a mismatch. They are not equal.
				return FALSE;
			}
		}
		prevchunk = chunk;
		chunk = chunk->next;
		otherchunk = otherchunk->next;
	}

	// Reached the end of the call stacks. They are equal.
	return TRUE;
}

// operator [] - Random access operator. Retrieves the frame at the specified
//   index.
//
//   Note: We give up a bit of efficiency here, in favor of efficiency of push
//     operations. This is because walking of a CallStack is done infrequently
//     (only if a leak is found), whereas pushing is done very frequently (for
//     each frame in the program's call stack when the program allocates some
//     memory).
//
//  - index (IN): Specifies the index of the frame to retrieve.
//
//  Return Value:
//
//    Returns the program counter for the frame at the specified index. If the
//    specified index is out of range for the CallStack, the return value is
//    undefined.
//
UINT_PTR CallStack::operator [] (UINT32 index) const
{
	UINT32                    chunknumber = index / CALLSTACKCHUNKSIZE;
	const CallStack::chunk_t *chunk = &m_store;

	for (UINT32 count = 0; count < chunknumber; count++) {
		chunk = chunk->next;
	}

	return chunk->frames[index % CALLSTACKCHUNKSIZE];
}

// clear - Resets the CallStack, returning it to a state where no frames have
//   been pushed onto it, readying it for reuse.
//
//   Note: Calling this function does not release any memory allocated to the
//     CallStack. We give up a bit of memory-usage efficiency here in favor of
//     performance of push operations.
//
//  Return Value:
//
//    None.
//
VOID CallStack::clear ()
{
	m_size     = 0;
	m_topchunk = &m_store;
	m_topindex = 0;
	if (m_Resolved)
	{
		delete [] m_Resolved;
		m_Resolved = NULL;
	}
	m_ResolvedCapacity = 0;
	m_ResolvedLength = 0;
}


// dump - Dumps a nicely formatted rendition of the CallStack, including
//   symbolic information (function names and line numbers) if available.
//
//   Note: The symbol handler must be initialized prior to calling this
//     function.
//
//  - showinternalframes (IN): If true, then all frames in the CallStack will be
//      dumped. Otherwise, frames internal to the heap will not be dumped.
//
//  Return Value:
//
//    None.
//
void CallStack::dump(BOOL showinternalframes, UINT start_frame) const
{
	// The stack was dumped already
	if (m_Resolved)
	{
		DumpResolved();
		return;
	}

	if (m_status & CALLSTACK_STATUS_INCOMPLETE) {
		// This call stack appears to be incomplete. Using StackWalk64 may be
		// more reliable.
		report(L"    HINT: The following call stack may be incomplete. Setting \"StackWalkMethod\"\n"
			L"      in the vld.ini file to \"safe\" instead of \"fast\" may result in a more\n"
			L"      complete stack trace.\n");
	}

	CriticalSectionLocker cs(g_symbollock);

	IMAGEHLP_LINE64  sourceinfo = { 0 };
	sourceinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	const UINT32 symbolBufSize = sizeof(SYMBOL_INFO) + (MAXSYMBOLNAMELENGTH * sizeof(WCHAR)) - 1;
	BYTE symbolbuffer [symbolBufSize] = { 0 };
	
	WCHAR undecoratedname [MAXSYMBOLNAMELENGTH];
	WCHAR callingmodulename [MAX_PATH];

	const size_t max_size = MAXREPORTLENGTH + 1;

	// Iterate through each frame in the call stack.
	for (UINT32 frame = start_frame; frame < m_size; frame++)
	{
		// Try to get the source file and line number associated with
		// this program counter address.
		SIZE_T programcounter = (*this)[frame];
		BOOL             foundline = FALSE;
		DWORD            displacement = 0;
		foundline = SymGetLineFromAddrW64(g_currentprocess, programcounter, &displacement, &sourceinfo);
		if (foundline && !showinternalframes) {
			_wcslwr_s(sourceinfo.FileName, wcslen(sourceinfo.FileName) + 1);
			if (IsInternalModule(sourceinfo.FileName)) {
				// Don't show frames in files internal to the heap.
				continue;
			}
		}

		// Initialize structures passed to the symbol handler.
		SYMBOL_INFO* functioninfo = (SYMBOL_INFO*)&symbolbuffer;
		functioninfo->SizeOfStruct = sizeof(SYMBOL_INFO);
		functioninfo->MaxNameLen = MAXSYMBOLNAMELENGTH;

		// Try to get the name of the function containing this program
		// counter address.
		DWORD64          displacement64 = 0;
		LPWSTR           functionname;
		if (SymFromAddrW(g_currentprocess, programcounter, &displacement64, functioninfo)) {
			// Undecorate function name.
			if (UnDecorateSymbolName(functioninfo->Name, undecoratedname, MAXSYMBOLNAMELENGTH, UNDNAME_NAME_ONLY) > 0)
				functionname = undecoratedname;
			else
				functionname = functioninfo->Name;
		}
		else {
			// GetFormattedMessage( GetLastError() );
			functionname = L"(Function name unavailable)";
			displacement64 = 0;
		}

		HMODULE hCallingModule = GetCallingModule(programcounter);
		LPWSTR modulename = L"(Module name unavailable)";
		if (hCallingModule && 
			GetModuleFileName(hCallingModule, callingmodulename, _countof(callingmodulename)) > 0)
		{
			modulename = wcsrchr(callingmodulename, L'\\');
			if (modulename == NULL)
				modulename = wcsrchr(callingmodulename, L'/');
			if (modulename != NULL)
				modulename++;
			else
				modulename = callingmodulename;
		}

		// Use static here to increase performance, and avoid heap allocs. Hopefully this won't
		// prove to be an issue in thread safety. If it does, it will have to be simply non-static.
		static WCHAR stack_line[MAXREPORTLENGTH + 1] = L"";
		int NumChars = -1;
		// Display the current stack frame's information.
		if (foundline) {
			if (displacement == 0)
				NumChars = _snwprintf_s(stack_line, max_size, _TRUNCATE, L"    %s (%d): %s!%s\n", 
				sourceinfo.FileName, sourceinfo.LineNumber, modulename, functionname);
			else
				NumChars = _snwprintf_s(stack_line, max_size, _TRUNCATE, L"    %s (%d): %s!%s + 0x%X bytes\n", 
				sourceinfo.FileName, sourceinfo.LineNumber, modulename, functionname, displacement);
		}
		else {
			if (displacement64 == 0)
				NumChars = _snwprintf_s(stack_line, max_size, _TRUNCATE, L"    " ADDRESSFORMAT L" (File and line number not available): %s!%s\n", 
				programcounter, modulename, functionname);
			else
				NumChars = _snwprintf_s(stack_line, max_size, _TRUNCATE, L"    " ADDRESSFORMAT L" (File and line number not available): %s!%s + 0x%X bytes\n", 
				programcounter, modulename, functionname, (DWORD)displacement64);	
		}

		print(stack_line);
	}
}

// Resolve - Creates a nicely formatted rendition of the CallStack, including
//   symbolic information (function names and line numbers) if available. and 
//   saves it for later retrieval. This is almost identical to Callstack::dump above.
//
//   Note: The symbol handler must be initialized prior to calling this
//     function.
//
//  - showinternalframes (IN): If true, then all frames in the CallStack will be
//      dumped. Otherwise, frames internal to the heap will not be dumped.
//
//  - ResolveOnly (IN): If true, it does not print the results out to the standard
//    outputs, but saves the formatted rendition for later retrieval.
//
//  Return Value:
//
//    None.
//
void CallStack::Resolve(BOOL showinternalframes)
{
	if (m_Resolved)
	{
		// already resolved, no need to do it again
		// resolving twice may report an incorrect module for the stack frames
		// if the memory was leaked in a dynamic library that was already unloaded.
		return;
	}
	if (m_status & CALLSTACK_STATUS_INCOMPLETE) {
		// This call stack appears to be incomplete. Using StackWalk64 may be
		// more reliable.
		report(L"    HINT: The following call stack may be incomplete. Setting \"StackWalkMethod\"\n"
			L"      in the vld.ini file to \"safe\" instead of \"fast\" may result in a more\n"
			L"      complete stack trace.\n");
	}

	CriticalSectionLocker cs(g_symbollock);

	IMAGEHLP_LINE64  sourceinfo = { 0 };
	sourceinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	const UINT32 symbolBufSize = sizeof(SYMBOL_INFO) + (MAXSYMBOLNAMELENGTH * sizeof(WCHAR)) - 1;
	BYTE symbolbuffer [symbolBufSize] = { 0 };
	
	WCHAR undecoratedname [MAXSYMBOLNAMELENGTH] = L"";
	WCHAR callingmodulename [MAX_PATH] = L"";

	const size_t max_line_length = MAXREPORTLENGTH + 1;
	m_ResolvedCapacity = m_size * max_line_length;
	m_Resolved = new WCHAR[m_ResolvedCapacity];
	const size_t allocedBytes = m_ResolvedCapacity * sizeof(WCHAR);
	ZeroMemory(m_Resolved, allocedBytes);
	
	// Iterate through each frame in the call stack.
	for (UINT32 frame = 0; frame < m_size; frame++)
	{
		// Try to get the source file and line number associated with
		// this program counter address.
		SIZE_T programcounter = (*this)[frame];
		BOOL             foundline = FALSE;
		DWORD            displacement = 0;

		// It turns out that calls to SymGetLineFromAddrW64 may free the very memory we are scrutinizing here
		// in this method. If this is the case, m_Resolved will be null after SymGetLineFromAddrW64 returns. 
		// When that happens there is nothing we can do except crash.
		foundline = SymGetLineFromAddrW64(g_currentprocess, programcounter, &displacement, &sourceinfo);
		assert(m_Resolved != NULL);

		if (foundline && !showinternalframes) {
			_wcslwr_s(sourceinfo.FileName, wcslen(sourceinfo.FileName) + 1);
			if (IsInternalModule(sourceinfo.FileName)) {
				// Don't show frames in files internal to the heap.
				continue;
			}
		}

		// Initialize structures passed to the symbol handler.
		SYMBOL_INFO* functioninfo = (SYMBOL_INFO*)&symbolbuffer;
		functioninfo->SizeOfStruct = sizeof(SYMBOL_INFO);
		functioninfo->MaxNameLen = MAXSYMBOLNAMELENGTH;

		// Try to get the name of the function containing this program
		// counter address.
		DWORD64          displacement64 = 0;
		LPWSTR           functionname;
		if (SymFromAddrW(g_currentprocess, programcounter, &displacement64, functioninfo)) {
			// Undecorate function name.
			if (UnDecorateSymbolName(functioninfo->Name, undecoratedname, MAXSYMBOLNAMELENGTH, UNDNAME_NAME_ONLY) > 0)
				functionname = undecoratedname;
			else
				functionname = functioninfo->Name;
		}
		else {
			// GetFormattedMessage( GetLastError() );
			functionname = L"(Function name unavailable)";
			displacement64 = 0;
		}
		
		HMODULE hCallingModule = GetCallingModule(programcounter);
		LPWSTR modulename = L"(Module name unavailable)";
		if (hCallingModule && 
			GetModuleFileName(hCallingModule, callingmodulename, _countof(callingmodulename)) > 0)
		{
			modulename = wcsrchr(callingmodulename, L'\\');
			if (modulename == NULL)
				modulename = wcsrchr(callingmodulename, L'/');
			if (modulename != NULL)
				modulename++;
			else
				modulename = callingmodulename;
		}

		// Use static here to increase performance, and avoid heap allocs. Hopefully this won't
		// prove to be an issue in thread safety. If it does, it will have to be simply non-static.
		static WCHAR stack_line[max_line_length] = L"";
		int NumChars = -1;
		// Display the current stack frame's information.
		if (foundline) {
			// Just truncate anything that is too long.
			if (displacement == 0)
				NumChars = _snwprintf_s(stack_line, max_line_length, _TRUNCATE, L"    %s (%d): %s!%s\n", 
				sourceinfo.FileName, sourceinfo.LineNumber, modulename, functionname);
			else
				NumChars = _snwprintf_s(stack_line, max_line_length, _TRUNCATE, L"    %s (%d): %s!%s + 0x%X bytes\n", 
				sourceinfo.FileName, sourceinfo.LineNumber, modulename, functionname, displacement);
		}
		else {
			if (displacement64 == 0)
				NumChars = _snwprintf_s(stack_line, max_line_length, _TRUNCATE, L"    " ADDRESSFORMAT L" (File and line number not available): %s!%s\n", 
				programcounter, modulename, functionname);
			else
				NumChars = _snwprintf_s(stack_line, max_line_length, _TRUNCATE, L"    " ADDRESSFORMAT L" (File and line number not available): %s!%s + 0x%X bytes\n", 
				programcounter, modulename, functionname, (DWORD)displacement64);
		}

		if (NumChars >= 0) {
			assert(m_Resolved != NULL);
			m_ResolvedLength += NumChars;
			wcsncat_s(m_Resolved, m_ResolvedCapacity, stack_line, NumChars);
		}
	} // end for loop
}

// DumpResolve
void CallStack::DumpResolved() const
{
	if (m_Resolved)
	{
		int index = 0;
		WCHAR* resolved_stack = m_Resolved;
		while(index < m_ResolvedLength)
		{
			print(resolved_stack);
			resolved_stack += MAXREPORTLENGTH;
			index += MAXREPORTLENGTH;
		}
	}
}



// getHashValue - Generate callstack hash value.
//
//  Return Value:
//
//    None.
//
DWORD CallStack::getHashValue () const
{
	DWORD       hashcode = 0xD202EF8D;

	// Iterate through each frame in the call stack.
	for (UINT32 frame = 0; frame < m_size; frame++) {
		UINT_PTR programcounter = (*this)[frame];
		hashcode = CalculateCRC32(programcounter, hashcode);
	}
	return hashcode;
}

// push_back - Pushes a frame's program counter onto the CallStack. Pushes are
//   always appended to the back of the chunk list (aka the "top" chunk).
//
//   Note: This function will allocate additional memory as necessary to make
//     room for new program counter addresses.
//
//  - programcounter (IN): The program counter address of the frame to be pushed
//      onto the CallStack.
//
//  Return Value:
//
//    None.
//
VOID CallStack::push_back (const UINT_PTR programcounter)
{
	if (m_size == m_capacity) {
		// At current capacity. Allocate additional storage.
		CallStack::chunk_t *chunk = new CallStack::chunk_t;
		chunk->next = NULL;
		m_topchunk->next = chunk;
		m_topchunk = chunk;
		m_topindex = 0;
		m_capacity += CALLSTACKCHUNKSIZE;
	}
	else if (m_topindex >= CALLSTACKCHUNKSIZE) {
		// There is more capacity, but not in this chunk. Go to the next chunk.
		// Note that this only happens if this CallStack has previously been
		// cleared (clearing resets the data, but doesn't give up any allocated
		// space).
		m_topchunk = m_topchunk->next;
		m_topindex = 0;
	}

	m_topchunk->frames[m_topindex++] = programcounter;
	m_size++;
}

bool CallStack::IsInternalModule( const PWSTR filename ) const
{
	return wcsstr(filename, L"afxmem.cpp") ||
		wcsstr(filename, L"dbgheap.c") ||
		wcsstr(filename, L"malloc.c") ||
		wcsstr(filename, L"new.cpp") ||
		wcsstr(filename, L"newaop.cpp") ||
		wcsstr(filename, L"dbgcalloc.c") ||
		wcsstr(filename, L"realloc.c") ||
		wcsstr(filename, L"dbgrealloc.c") ||
		wcsstr(filename, L"free.c");
}

// getstacktrace - Traces the stack as far back as possible, or until 'maxdepth'
//   frames have been traced. Populates the CallStack with one entry for each
//   stack frame traced.
//
//   Note: This function uses a very efficient method to walk the stack from
//     frame to frame, so it is quite fast. However, unconventional stack frames
//     (such as those created when frame pointer omission optimization is used)
//     will not be successfully walked by this function and will cause the
//     stack trace to terminate prematurely.
//
//  - maxdepth (IN): Maximum number of frames to trace back.
//
//  - framepointer (IN): Frame (base) pointer at which to begin the stack trace.
//      If NULL, then the stack trace will begin at this function.
//
//  Return Value:
//
//    None.
//
VOID FastCallStack::getstacktrace (UINT32 maxdepth, const context_t& context)
{
	UINT32  count = 0;
	UINT_PTR* framepointer = context.fp;

#if defined(_M_IX86)
	while (count < maxdepth) {
		if (*framepointer < (UINT_PTR)framepointer) {
			if (*framepointer == NULL) {
				// Looks like we reached the end of the stack.
				break;
			}
			else {
				// Invalid frame pointer. Frame pointer addresses should always
				// increase as we move up the stack.
				m_status |= CALLSTACK_STATUS_INCOMPLETE;
				break;
			}
		}
		if (*framepointer & (sizeof(UINT_PTR*) - 1)) {
			// Invalid frame pointer. Frame pointer addresses should always
			// be aligned to the size of a pointer. This probably means that
			// we've encountered a frame that was created by a module built with
			// frame pointer omission (FPO) optimization turned on.
			m_status |= CALLSTACK_STATUS_INCOMPLETE;
			break;
		}
		if (IsBadReadPtr((UINT*)*framepointer, sizeof(UINT_PTR*))) {
			// Bogus frame pointer. Again, this probably means that we've
			// encountered a frame built with FPO optimization.
			m_status |= CALLSTACK_STATUS_INCOMPLETE;
			break;
		}
		count++;
		push_back(*(framepointer + 1));
		framepointer = (UINT_PTR*)*framepointer;
	}
#elif defined(_M_X64)
	UINT32 maxframes = min(62, maxdepth + 10);
	static USHORT (WINAPI *s_pfnCaptureStackBackTrace)(ULONG FramesToSkip, ULONG FramesToCapture, PVOID* BackTrace, PULONG BackTraceHash) = 0;  
	if (s_pfnCaptureStackBackTrace == 0)  
	{  
		const HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");  
		reinterpret_cast<void*&>(s_pfnCaptureStackBackTrace)
			= ::GetProcAddress(hNtDll, "RtlCaptureStackBackTrace");
		if (s_pfnCaptureStackBackTrace == 0)  
			return;
	}
	UINT_PTR* myFrames = new UINT_PTR[maxframes];
	ZeroMemory(myFrames, sizeof(UINT_PTR) * maxframes);
	s_pfnCaptureStackBackTrace(0, maxframes, (PVOID*)myFrames, NULL);
	UINT32  startIndex = 0;
	while (count < maxframes) {
		if (myFrames[count] == 0)
			break;
		if (myFrames[count] == *(framepointer + 1))
			startIndex = count;
		count++;
	}
	count = startIndex;
	while (count < maxframes) {
		if (myFrames[count] == 0)
			break;
		push_back(myFrames[count]);
		count++;
	}
	delete [] myFrames;
#endif
}

// getstacktrace - Traces the stack as far back as possible, or until 'maxdepth'
//   frames have been traced. Populates the CallStack with one entry for each
//   stack frame traced.
//
//   Note: This function uses a documented Windows API to walk the stack. This
//     API is supposed to be the most reliable way to walk the stack. It claims
//     to be able to walk stack frames that do not follow the conventional stack
//     frame layout. However, this robustness comes at a cost: it is *extremely*
//     slow compared to walking frames by following frame (base) pointers.
//
//  - maxdepth (IN): Maximum number of frames to trace back.
//
//  - framepointer (IN): Frame (base) pointer at which to begin the stack trace.
//      If NULL, then the stack trace will begin at this function.
//
//  Return Value:
//
//    None.
//
VOID SafeCallStack::getstacktrace (UINT32 maxdepth, const context_t& context)
{
	UINT_PTR* framepointer = context.fp;
	DWORD   architecture   = X86X64ARCHITECTURE;
	CONTEXT currentcontext;
	memset(&currentcontext, 0, sizeof(currentcontext));

	// Get the required values for initialization of the STACKFRAME64 structure
	// to be passed to StackWalk64(). Required fields are AddrPC and AddrFrame.
#if defined(_M_IX86)
	UINT_PTR programcounter = *(framepointer + 1);
	UINT_PTR stackpointer   = (*framepointer) - maxdepth * 10 * sizeof(void*);  // An approximation.
	currentcontext.SPREG  = stackpointer;
	currentcontext.BPREG  = (DWORD64)framepointer;
	currentcontext.IPREG  = programcounter;
#elif defined(_M_X64)
	currentcontext.SPREG  = context.Rsp;
	currentcontext.BPREG  = (DWORD64)framepointer;
	currentcontext.IPREG  = context.Rip;
#else
	// If you want to retarget Visual Leak Detector to another processor
	// architecture then you'll need to provide architecture-specific code to
	// obtain the program counter and stack pointer from the given frame pointer.
#error "Visual Leak Detector is not supported on this architecture."
#endif // _M_IX86 || _M_X64

	// Initialize the STACKFRAME64 structure.
	STACKFRAME64 frame;
	memset(&frame, 0x0, sizeof(frame));
	frame.AddrPC.Offset       = currentcontext.IPREG;
	frame.AddrPC.Mode         = AddrModeFlat;
	frame.AddrStack.Offset    = currentcontext.SPREG;
	frame.AddrStack.Mode      = AddrModeFlat;
	frame.AddrFrame.Offset    = currentcontext.BPREG;
	frame.AddrFrame.Mode      = AddrModeFlat;
	frame.Virtual             = TRUE;

	// Walk the stack.
	CriticalSectionLocker cs(g_stackwalklock);
	UINT32 count = 0;
	while (count < maxdepth) {
		count++;
		if (!StackWalk64(architecture, g_currentprocess, g_currentthread, &frame, &currentcontext, NULL,
			SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
				// Couldn't trace back through any more frames.
				break;
		}
		if (frame.AddrFrame.Offset == 0) {
			// End of stack.
			break;
		}

		// Push this frame's program counter onto the CallStack.
		push_back((UINT_PTR)frame.AddrPC.Offset);
	}
}
