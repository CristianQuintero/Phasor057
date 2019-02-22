#include <windows.h>
#include <vector>

BOOL WriteBytes(DWORD destAddress, LPVOID patch, DWORD numBytes);
BOOL ReadBytes(DWORD sourceAddress, LPVOID buffer, DWORD numBytes);
BOOL CreateCodeCave(DWORD destAddress, BYTE patchSize, VOID (*function)(VOID));
std::vector<DWORD> FindSignature(LPBYTE sigBuffer, LPBYTE sigWildCard, DWORD sigSize, LPBYTE pBuffer, DWORD size);
DWORD FindAddress(LPBYTE data, DWORD size, LPBYTE sig, DWORD sig_len, DWORD occ, int offset);

HMODULE s_hDll = 0;

// Called once the dll has been loaded and initialized
void __stdcall OnLoad()
{
	// Make sure this is a server's process
	// The servers OEP is PUSH 0x18 whereas client is PUSH 0x60
	LPBYTE module = (LPBYTE)GetModuleHandle(0);
	DWORD OffsetToPE = *(DWORD*)(module + 0x3C);
	DWORD ep_rva = *(DWORD*)(module + OffsetToPE + 0x28);
	
	LPBYTE entry_point = module + ep_rva;

	if (*(WORD*)entry_point == 0x186A) // PUSH 0x18 if server
	{
		// Load the Phasor dll
		HMODULE hModule = LoadLibrary("Phasor.dll");

		if (hModule)
		{
			typedef void (__cdecl *PhasorOnLoad)();
			PhasorOnLoad OnLoad = (PhasorOnLoad)GetProcAddress(hModule, "OnLoad");
			OnLoad();
		}
	}
}

DWORD dwLoad_ret = 0;
__declspec(naked) void OnLoad_CC()
{
	__asm
	{
		pop dwLoad_ret

		pushad

		call OnLoad

		popad

		LEA EDX,DWORD PTR SS:[ESP+8]
		PUSH EDX

		push dwLoad_ret
		ret
	}
}

// Entry point for the dll
BOOL WINAPI DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	if(ulReason == DLL_PROCESS_ATTACH)
	{
		s_hDll = hModule;

		// Do not notify this DLL of thread based events
		DisableThreadLibraryCalls(hModule);

		// Create a codecave so we get notified once it's been loaded
		LPBYTE module = (LPBYTE)GetModuleHandle(0);
		DWORD OffsetToPE = *(DWORD*)(module + 0x3C);
		DWORD codeSize = *(DWORD*)(module + OffsetToPE + 0x1C);
		DWORD BaseOfCode = *(DWORD*)(module + OffsetToPE + 0x2C);
		LPBYTE codeSection = (LPBYTE)(module + BaseOfCode);

		BYTE sig[] = {0x8D, 0x54, 0x24, 0x08, 0x52, 0x68, 0x19, 0x00, 0x02, 0x00, 0x6A, 0x00};
		DWORD cc_loaded = FindAddress(codeSection, codeSize, sig, sizeof(sig), 0, 0);

		if (cc_loaded)
			CreateCodeCave(cc_loaded, 5, OnLoad_CC);	
	}

	return TRUE;
}

DWORD FindAddress(LPBYTE data, DWORD size, LPBYTE sig, DWORD sig_len, DWORD occ, int offset)
{
	DWORD dwAddress = 0;

	// find the address
	std::vector<DWORD> results = FindSignature(sig, NULL, sig_len, data, size);

	if (results.size() && results.size()-1 >= occ)
		dwAddress = (DWORD)data + results[occ] + offset;

	return dwAddress;
}

// Writes bytes in the current process (Made by Drew_Benton).
BOOL WriteBytes(DWORD destAddress, LPVOID patch, DWORD numBytes)
{
	// Store old protection of the memory page
	DWORD oldProtect = 0;

	// Store the source address
	DWORD srcAddress = PtrToUlong(patch);

	// Result of the function
	BOOL result = TRUE;

	// Make sure page is writable
	result = result && VirtualProtect(UlongToPtr(destAddress), numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);

	// Copy over the patch
	memcpy(UlongToPtr(destAddress), patch, numBytes);

	// Restore old page protection
	result = result && VirtualProtect(UlongToPtr(destAddress), numBytes, oldProtect, &oldProtect);

	// Make sure changes are made
	result = result && FlushInstructionCache(GetCurrentProcess(), UlongToPtr(destAddress), numBytes); 

	// Return the result
	return result;
}

// Reads bytes in the current process (Made by Drew_Benton).
BOOL ReadBytes(DWORD sourceAddress, LPVOID buffer, DWORD numBytes)
{
	// Store old protection of the memory page
	DWORD oldProtect = 0;

	// Store the source address
	DWORD dstAddress = PtrToUlong(buffer);

	// Result of the function
	BOOL result = TRUE;

	// Make sure page is writable
	result = result && VirtualProtect(UlongToPtr(sourceAddress), numBytes, PAGE_EXECUTE_READWRITE, &oldProtect);

	// Copy over the patch
	memcpy(buffer, UlongToPtr(sourceAddress), numBytes);

	// Restore old page protection
	result = result && VirtualProtect(UlongToPtr(sourceAddress), numBytes, oldProtect, &oldProtect);

	// Return the result
	return result;
}

// Creates a codecave
BOOL CreateCodeCave(DWORD destAddress, BYTE patchSize, VOID (*function)(VOID))
{
	// Offset to make the codecave at
	DWORD offset = 0;

	// Bytes to write
	BYTE patch[5] = {0};

	// Number of extra nops we need
	BYTE nopCount = 0;

	// NOP buffer
	static BYTE nop[0xFF] = {0};

	// Is the buffer filled?
	static BOOL filled = FALSE;

	// Need at least 5 bytes to be patched
	if(patchSize < 5)
		return FALSE;

	// Calculate the code cave
	offset = (PtrToUlong(function) - destAddress) - 5;

	// Construct the patch to the function call
	patch[0] = 0xE8;
	memcpy(patch + 1, &offset, sizeof(DWORD));
	WriteBytes(destAddress, patch, 5);

	// We are done if we do not have NOPs
	nopCount = patchSize - 5;
	if(nopCount == 0)
		return TRUE;

	// Fill in the buffer
	if(filled == FALSE)
	{
		memset(nop, 0x90, 0xFF);
		filled = TRUE;
	}

	// Make the patch now
	WriteBytes(destAddress + 5, nop, nopCount);

	// Success
	return TRUE;
}

std::vector<DWORD> FindSignature(LPBYTE sigBuffer, LPBYTE sigWildCard, DWORD sigSize, LPBYTE pBuffer, DWORD size)
{
	std::vector<DWORD> results;
	for(DWORD index = 0; index < size; ++index)
	{
		bool found = true;
		for(DWORD sindex = 0; sindex < sigSize; ++sindex)
		{
			// Make sure we don't overrun the buffer!
			if(sindex + index >= size)
			{
				found = false;
				break;
			}

			if(sigWildCard != 0)
			{
				if(pBuffer[index + sindex] != sigBuffer[sindex] && sigWildCard[sindex] == 0)
				{
					found = false;
					break;
				}
			}
			else
			{
				if(pBuffer[index + sindex] != sigBuffer[sindex])
				{
					found = false;
					break;
				}
			}
		}
		if(found)
		{
			results.push_back(index);
		}
	}
	return results;
}
