// CE COMPATIBLE CHECKED
/* Phasor - Halo PC Server Extension
   Copyright (C) 2010-2011  Oxide (http://haloapps.wordpress.com)
  
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "Hooks.h"
#include "Common.h"
#include "Server.h"
#include "Maps.h"
#include "objects.h"
#include <stack>

// These two functions are used to track which codecave is being executed,
// this information is useful for debugging.
std::stack<std::string> cc_execution;
void PushHookStack(const char* func)
{
	cc_execution.push(func);
}

void PopHookStack()
{
	if (!cc_execution.empty())
		cc_execution.pop();
}

// Returns the execution stack
std::stack<std::string> GetExecutionStack()
{
	return cc_execution;
}

#define CC_BEGIN \
__asm {pushad}\
__asm {pushfd}\
PushHookStack(__FUNCTION__);\
__asm {popfd}\
__asm {popad}

#define CC_END_RET \
	__asm {pushad}\
	__asm {pushfd}\
	__asm {call PopHookStack}\
	__asm {popfd}\
	__asm {popad}\
	__asm {ret}

// Server function codecaves
// ------------------------------------------------------------------------
DWORD consoleProc_ret = 0;

// Codecave for timers, safer than creating threads (hooking console chceking routine)
__declspec(naked) void OnConsoleProcessing_CC()
{
	CC_BEGIN
	__asm
	{
		pop consoleProc_ret

		pushad

		call server::OnConsoleProcessing

		popad

		PUSH EBX
		XOR EBX,EBX

		push consoleProc_ret

		CMP AL,BL

		CC_END_RET
	}
}

DWORD conHandler_ret = 0;

// Codecave for hooking console events (closing etc)
__declspec(naked) void ConsoleHandler_CC()
{
	CC_BEGIN
	__asm
	{
		pop conHandler_ret

		pushad

		push esi
		call server::ConsoleHandler

		popad

		mov eax, 1

		push conHandler_ret

		CC_END_RET
	}
}

// Used to store the return address for the function
DWORD cmd_ret = 0;

// Codecave for intercepting server commands
__declspec(naked) void OnServerCommand()
{
	CC_BEGIN
	__asm
	{
		// Save return address
		pop cmd_ret

		pushad

		push edi
		call server::ConsoleCommand

		cmp al, 1
		je PROCESSED

		popad

		MOV AL,BYTE PTR DS:[EDI]
		SUB ESP,0x500

		push cmd_ret

		CC_END_RET

PROCESSED:

		popad

		mov eax, 1

		CC_END_RET
	}
}

DWORD mapload_ret = 0;

// Codecave for loading maps into the cyc;e
__declspec(naked) void OnMapLoading_CC()
{
	CC_BEGIN
	__asm
	{
		pop mapload_ret

		pushad

		lea eax, dword ptr ds:[eax + esi]
		push eax
		call server::OnMapLoad

		cmp al, 0
		je RELOAD_MAP_DATA

		popad

		MOV ECX,DWORD PTR DS:[ESI+EAX]
		PUSH 0x3F

		push mapload_ret

		CC_END_RET

RELOAD_MAP_DATA:

		popad

		// reset mapycle index
		mov eax, DWORD PTR ds:[ADDR_MAPCYCLEINDEX]
		MOV [eax], 0
		mov esi, 0 // value of ADDR_MAPCYCLEINDEX, which we just set to 0

		// get data for current map
		mov eax, dword ptr ds:[ADDR_CURRENTMAPDATA]
		mov eax, [eax]
		
		//MOV ESI,DWORD PTR DS:[ADDR_MAPCYCLEINDEX]
		//mov ESI, [ESI]

		MOV ECX,DWORD PTR DS:[EAX+ESI]

		push 0x3F

		push mapload_ret

		CC_END_RET
	}
}

// Game function codecaves
// ------------------------------------------------------------------------
// Codecave for detecting game ending
__declspec(naked) void OnGameEnd_CC()
{
	CC_BEGIN
	__asm
	{
		pop edx // ret addr

		pushad

		push eax
		call game::OnEnd

		popad

		SUB ESP,0x0C                      
		PUSH 0

		push edx

		CC_END_RET
	}
}

DWORD newGame_ret = 0;

// Codecave used for detecting a new game
__declspec(naked) void OnNewGame_CC()
{
	CC_BEGIN

	__asm
	{
		pop newGame_ret

		MOV EAX,DWORD PTR DS:[ADDR_NEWGAMEMAP]
		mov eax, [eax]

		cmp EAX, 0
		je NOT_NEW_MAP
		pushad

		lea eax, dword ptr ds:[esp + 0x24]
		mov eax, [eax]
		push eax
		call game::OnNew

		popad

NOT_NEW_MAP:

		push newGame_ret

		CC_END_RET
	}
}

DWORD playerjoin_ret = 0;

// Codecave for people joining
__declspec(naked) void OnPlayerWelcome_CC()
{
	CC_BEGIN
	__asm
	{
		pop playerjoin_ret
		#ifdef PHASOR_PC
			PUSH EAX
			MOV ESI,EBP
		#elif PHASOR_CE
			PUSH EBP
			MOV ESI,EAX
		#endif

		call dword ptr ds:[FUNC_PLAYERJOINING]
		pushad

		mov eax, [esp - 0x60]
		and eax, 0xff
		push eax
		call game::OnPlayerWelcome

		popad

		push playerjoin_ret

		CC_END_RET
	}
}

DWORD leaving_ret = 0;

// Codecave used for detecting people leaving
__declspec(naked) void OnPlayerQuit_CC()
{
	CC_BEGIN
	__asm
	{
		pop leaving_ret

		pushad

		and eax, 0xff
		push eax
		call game::OnPlayerQuit

		popad

		PUSH EBX
		PUSH ESI
		MOV ESI,EAX
		MOV EAX,DWORD PTR DS:[ADDR_PLAYERBASE]
		mov eax, [eax]

		push leaving_ret

		CC_END_RET
	}
}

DWORD teamsel_ret = 0, selection = 0;

// Codecave for team selection
__declspec(naked) void OnTeamSelection_CC()
{
	CC_BEGIN
	__asm
	{
		pop teamsel_ret

		// ecx can be modified safely (see FUNC_TEAMSELECT)
		mov ecx, FUNC_TEAMSELECT
		call ecx

		pushad

		movsx eax, al
		push eax
		call game::OnTeamSelection
		mov selection, eax

		popad

		mov eax, selection

		push teamsel_ret

		CC_END_RET
	}
}

// Codecave for handling team changes
DWORD teamchange_ret = 0;
__declspec(naked) void OnTeamChange_CC()
{
	CC_BEGIN
	__asm
	{
		pop teamchange_ret // ret addr cant safely modify

		pushad

		and eax, 0xff

		push ebx // team
		push eax // player
		call game::OnTeamChange

		cmp al, 0
		je DO_NOT_CHANGE

		// Allow them to change
		popad

		// overwritten code
		#ifdef PHASOR_PC
		MOVZX ESI,AL
		LEA ECX,DWORD PTR DS:[EDI+8]
		#elif PHASOR_CE
		ADD EAX, 8
		PUSH EBX
		PUSH EAX
		#endif

		push teamchange_ret // return address

		CC_END_RET

DO_NOT_CHANGE:

		// Don't let them change
		popad

#ifdef PHASOR_PC
		POP EDI
#endif
		POP ESI
		POP EBX
		ADD ESP,0x1C
		CC_END_RET
	}
}

// Codecave for player spawns (just before server notification)
__declspec(naked) void OnPlayerSpawn_CC()
{
	CC_BEGIN
	__asm
	{
		pop ecx // can safely use ecx (see function call after codecave)

		pushad

		and ebx, 0xff
		push esi // memory object id
		push ebx // player memory id
		call game::OnPlayerSpawn

		popad

		// orig code
		push 0x7ff8

		push ecx

		CC_END_RET
	}
}

DWORD playerSpawnEnd_ret = 0;

// Codecave for player spawns (after server has been notified).
__declspec(naked) void OnPlayerSpawnEnd_CC()
{
	CC_BEGIN
	__asm
	{
		pop playerSpawnEnd_ret

		mov ecx, FUNC_AFTERSPAWNROUTINE
		call ecx
		add esp, 0x0C

		pushad

		and ebx, 0xff

		push esi // memory object id
		push ebx // player memory id
		call game::OnPlayerSpawnEnd

		popad

		push playerSpawnEnd_ret

		CC_END_RET
	}
}

DWORD objcreation_ret = 0;

// Codecave for modifying weapons as they're created
__declspec(naked) void OnObjectCreation_CC()
{
	CC_BEGIN
	__asm
	{
		pop objcreation_ret

		pushad

		push ebx
		call game::OnObjectCreation

		popad

		MOV ECX,DWORD PTR DS:[EAX+4]
		TEST ECX,ECX

		push objcreation_ret

		CC_END_RET
	}
}

DWORD wepassignment_ret = 0, wepassign_val = 0;

// Codecave for handling weapon assignment to spawning players
__declspec(naked) void OnWeaponAssignment_CC()
{
	CC_BEGIN
	__asm
	{
		pop wepassignment_ret

		pushad

		mov edi, [esp + 0xFC]
		and edi, 0xff

		mov ecx, [esp + 0x2c]
		push ecx
		push eax
		push esi
		push edi
		call game::OnWeaponAssignment
		mov wepassign_val, eax
		cmp eax, 0
		jnz USE_RETNED_VALUE

		popad

		MOV EAX,DWORD PTR DS:[EAX+0x0C] // original instruction
		jmp WEP_ASSIGN_RET
USE_RETNED_VALUE:

		popad
		mov eax, wepassign_val

WEP_ASSIGN_RET:
		push wepassignment_ret

		CMP EAX,-1

		CC_END_RET
	}
}

DWORD objInteraction_ret = 0;

// Codecave for handling object pickup interactions
__declspec(naked) void OnObjectInteraction_CC()
{
	CC_BEGIN
	__asm
	{
		pop objInteraction_ret

		pushad

		mov eax, dword ptr ds:[esp + 0x38]
		mov edi, dword ptr ds:[esp + 0x3c]

		and eax, 0xff
		push edi
		push eax
		call game::OnObjectInteraction

		cmp al, 0
		je DO_NOT_CONTINUE

		popad

		PUSH EBX
		MOV EBX,DWORD PTR SS:[ESP+0x20]

		push objInteraction_ret

		CC_END_RET

DO_NOT_CONTINUE:

		popad
		add esp, 0x14
		CC_END_RET
	}
}

// Codecave for server chat
__declspec(naked) void OnChat_CC()
{
	CC_BEGIN
	__asm
	{
		// don't need the return address as we skip this function
		add esp, 4

		pushad

		#ifdef PHASOR_PC
		mov eax, dword ptr ds:[esp + 0x2c]
		and eax, 0xff // other 3 bytes aren't used for player number and can be non-zero
		mov [esp + 0x2c], eax
		lea eax, dword ptr ds:[esp + 0x28]
		#elif PHASOR_CE
		mov eax, dword ptr ds:[esp + 0x40]
		and eax, 0xff // other 3 bytes aren't used for player number and can be non-zero
		mov [esp + 0x40], eax
		lea eax, dword ptr ds:[esp + 0x3C]
		#endif
		
		push eax
		call game::OnChat

		popad

		POP EDI
		#ifdef PHASOR_PC
		ADD ESP,0x224
		#elif PHASOR_CE
		ADD ESP, 0x228
		#endif
		CC_END_RET
	}
}

// Codecave for player position updates
__declspec(naked) void OnClientUpdate_CC()
{
	CC_BEGIN
	__asm
	{
		pop edi // can safely use edi as its poped in return stub

		// Execute the original code
		MOV BYTE PTR DS:[EAX+0x2A6],DL
		JE L008
		MOV DWORD PTR DS:[EAX+0x4BC],EBP
		MOV BYTE PTR DS:[EAX+0x4B8],0x1
		jmp START_PROCESSING
L008:
		MOV BYTE PTR DS:[EAX+0x4B8],0x0
START_PROCESSING:

		pushad

		push eax // player's object
		call game::OnClientUpdate

		popad

		// return out of function
		POP EDI
		POP ESI
		POP EBP
		CC_END_RET
	}
}

DWORD dmglookup_ret = 0;

// Codecaves for handling weapon damage
__declspec(naked) void OnDamageLookup_CC()
{
	CC_BEGIN
	__asm
	{
		pop dmglookup_ret

		pushad

		mov esi, [ebp + 0x0C] // object causing the damage
		lea ecx, dword ptr ds:[edx + eax] // table entry for the damage tag
		mov edi, [esp + 0xcc] // object taking the damage

		push ecx // damage tag		
		push esi // object causing damage
		push edi // object taking damage
		call game::OnDamageLookup

		popad

		ADD EDI,0x1C4
		push dmglookup_ret
		CC_END_RET
	}
}

DWORD OnVehicleEntry_ret = 0;
__declspec(naked) void OnVehicleEntry_CC()
{
	CC_BEGIN
	__asm
	{
		pop OnVehicleEntry_ret

		pushad
		pushfd

		mov eax, [ebp + 0x8]
		and eax, 0xff

		push eax
		call game::OnVehicleEntry

		cmp al, 1

		je ALLOW_VEHICLE_ENTRY

		popfd
		popad

		POP EDI
		POP ESI
		POP EBX
		MOV ESP,EBP
		POP EBP

		mov eax, 1
		CC_END_RET // don't let them change

ALLOW_VEHICLE_ENTRY:

		popfd
		popad

		MOV DWORD PTR SS:[EBP-8],-1

		push OnVehicleEntry_ret

		CC_END_RET
	}
}

DWORD ondeath_ret = 0;

// Codecave for handling player deaths
__declspec(naked) void OnDeath_CC()
{
	CC_BEGIN
	__asm
	{
		pop ondeath_ret

		pushad

		lea eax, dword ptr ds:[esp + 0x38] // victim
		mov eax, [eax]
		and eax, 0xff
		lea ecx, dword ptr ds:[esp + 0x34] // killer
		mov ecx, [ecx]
		and ecx, 0xff

		push esi // mode of death
		push eax // victim
		push ecx // killer
		call game::OnDeath

		popad

		PUSH EBP
		MOV EBP,DWORD PTR SS:[ESP+0x20]

		push ondeath_ret

		CC_END_RET
	}
}

DWORD killmultiplier_ret = 0;

// Codecave for detecting double kills, sprees etc
__declspec(naked) void OnKillMultiplier_CC()
{
	CC_BEGIN
	__asm
	{
		pop killmultiplier_ret

		pushad

		push esi // muliplier
		push ebp // player
		call game::OnKillMultiplier

		popad

		MOV EAX,DWORD PTR SS:[ESP+0x1C]
		PUSH EBX

		push killmultiplier_ret

		CC_END_RET
	}
}

// Codecave for handling weapon reloading
DWORD weaponReload_ret = 0;
__declspec(naked) void OnWeaponReload_CC()
{
	CC_BEGIN
	__asm
	{
		pop weaponReload_ret

		pushad

		mov ecx, [esp + 0x40]
		push ecx
		call game::OnWeaponReload

		cmp al, 1
		je ALLOW_RELOAD

		popad

		AND DWORD PTR DS:[EAX+0x22C],0xFFFFFFF7
		POP EDI
		POP ESI
		POP EBP
		POP EBX
		ADD ESP,0xC
		CC_END_RET

ALLOW_RELOAD:

		popad

		MOV CL,BYTE PTR SS:[ESP+0x28]
		OR EBP,0xFFFFFFFF

		push weaponReload_ret

		CC_END_RET
	}
}

// used to control object respawning
DWORD objres_ret = 0;
__declspec(naked) void OnObjectRespawn_CC()
{
	CC_BEGIN
	__asm
	{
		pop objres_ret

		pushad
		push ebx // object's memory address
		push esi // object's id
		call objects::ObjectRespawnCheck // returns true if we should respawn, false if not
		cmp al, 2
		je OBJECT_DESTROYED
		cmp al, 1 // return to a JL statement, jump if not respawn
		popad
		push objres_ret
		CC_END_RET

OBJECT_DESTROYED:
		popad

		mov eax, 1
		pop edi
		pop esi
		pop ebx
		mov esp,ebp
		pop ebp

		CC_END_RET
	}
}

// used to control itmc destruction
DWORD itmcdes_ret = 0;
__declspec(naked) void OnEquipmentDestroy_CC()
{
	CC_BEGIN
	__asm
	{
		pop itmcdes_ret

		pushad
		push ebp // equipment's memory address
		push ebx // equipment's memory id
		push esi // tick count of when the item is due for destruction
		call objects::EquipmentDestroyCheck // returns true if should destroy
		xor al, 1 // 1 -> 0, 0 -> 1
		cmp al, 1 // returns to a JGE so if al < 1 item is destroyed
		popad

		push itmcdes_ret
		CC_END_RET
	}
}

DWORD vehfeject_ret = 0;
__declspec(naked) void OnVehicleForceEject_CC()
{
	CC_BEGIN

	__asm
	{
		pop vehfeject_ret

		pushad
		push 1 // force eject
		push ebx 
		call game::OnVehicleEject // false - don't eject

		cmp al, 1
		je DO_FORCE_EJECT

		// don't let them eject
		popad
		sub vehfeject_ret, 0x2d //56e6d2-56e6a5 = 0x2d
		cmp al, al // to force jump

		push vehfeject_ret
		CC_END_RET

DO_FORCE_EJECT:
		popad

		CMP WORD PTR DS:[EBX+0x2F0],0x0FFFF

		push vehfeject_ret
		CC_END_RET
	}
}

DWORD vehueject_ret = 0;
__declspec(naked) void OnVehicleUserEject_CC()
{
	CC_BEGIN
	__asm
	{
		pop vehueject_ret

		TEST BYTE PTR DS:[EBX+0x208],0x40
		je NOT_EJECTING

		pushad
		push 0
		push ebx // not a forceable ejection
		call game::OnVehicleEject // false - don't eject

		cmp al, 1
		je DO_USER_EJECT

		popad 
		cmp al, al // force the jump

		push vehueject_ret
		CC_END_RET

DO_USER_EJECT:
		popad
		TEST BYTE PTR DS:[EBX+0x208],0x40
NOT_EJECTING:
		
		push vehueject_ret
		CC_END_RET
	}
}

namespace halo
{
	// Installs all codecaves and applies all patches
	void InstallHooks()
	{
		// Patches
		// ----------------------------------------------------------------
		// 
		// Ensure we always decide the team
		BYTE nopSkipSelection[2] = {0x90, 0x90};
		WriteBytes(PATCH_TEAMSLECTION, &nopSkipSelection, 2);

		// Stop the server from processing map additions (sv_map, sv_mapcycle_begin)
		BYTE mapPatch[] = {0xB0, 0x01, 0xC3};
		WriteBytes(PATCH_NOMAPPROCESS, &mapPatch, sizeof(mapPatch));

		#ifdef PHASOR_PC
		// Make Phasor control the loading of a map
		DWORD addr = (DWORD)UlongToPtr(game::maps::GetLoadingMapBuffer());
		WriteBytes(PATCH_MAPLOADING, &addr, 4);

		// Set the map table
		DWORD table = game::maps::GetMapTable();
		WriteBytes(PATCH_MAPTABLE, &table, 4);

		// Remove where the map table is allocated/reallocated by the server
		BYTE nopPatch[0x3E];
		memset(nopPatch, 0x90, sizeof(nopPatch));
		WriteBytes(PATCH_MAPTABLEALLOCATION, &nopPatch, sizeof(nopPatch));	
		#endif

		// Server hooks
		// ----------------------------------------------------------------
		// 
		// Codecave for timers, safer than creating threads (hooking console chceking routine)
		CreateManagedCodeCave(CC_CONSOLEPROC, 5, OnConsoleProcessing_CC);

		// Codecave for hooking console events (closing etc)
		CreateManagedCodeCave(CC_CONSOLEHANDLER, 5, ConsoleHandler_CC);

		// Codecave to intercept server commands
		CreateManagedCodeCave(CC_SERVERCMD, 8, OnServerCommand);

		// Codecave used to load non-default maps
		CreateManagedCodeCave(CC_MAPLOADING, 5, OnMapLoading_CC);
		 
		// Game Hooks
		// ----------------------------------------------------------------
		//
		// Codecave for detecting game ending
		CreateManagedCodeCave(CC_GAMEEND, 5, OnGameEnd_CC);

		// Codecave used to detect a new game
		CreateManagedCodeCave(CC_NEWGAME, 5, OnNewGame_CC);

		// Codecave called when a player joins
		CreateManagedCodeCave(CC_PLAYERWELCOME, 8, OnPlayerWelcome_CC);

		// Codecave used to detect people leaving
		CreateManagedCodeCave(CC_PLAYERQUIT, 9, OnPlayerQuit_CC);

		// Codecave used to decide the player's team
		CreateManagedCodeCave(CC_TEAMSELECTION, 5, OnTeamSelection_CC);

		// Codecave for handling team changes
		#ifdef PHASOR_PC
		CreateManagedCodeCave(CC_TEAMCHANGE, 6, OnTeamChange_CC);
		#elif PHASOR_CE
		CreateManagedCodeCave(CC_TEAMCHANGE, 5, OnTeamChange_CC);
		#endif

		// Codecaves for detecting player spawns
		CreateManagedCodeCave(CC_PLAYERSPAWN, 5, OnPlayerSpawn_CC);
		CreateManagedCodeCave(CC_PLAYERSPAWNEND, 8, OnPlayerSpawnEnd_CC);

		// Codecave called when a weapon is created
		CreateManagedCodeCave(CC_OBJECTCREATION, 5, OnObjectCreation_CC);

		// Codecave for handling weapon assignment to spawning players
		CreateManagedCodeCave(CC_WEAPONASSIGN, 6, OnWeaponAssignment_CC);

		// Codecave for interations with pickable objects
		CreateManagedCodeCave(CC_OBJECTINTERACTION, 5, OnObjectInteraction_CC);

		// Codecave for position updates
		CreateManagedCodeCave(CC_CLIENTUPDATE, 6, OnClientUpdate_CC);

		// Codecave for handling damage being done
		CreateManagedCodeCave(CC_DAMAGELOOKUP, 6, OnDamageLookup_CC);

		// Codecave for server chat
		CreateManagedCodeCave(CC_CHAT, 7, OnChat_CC);

		// Codecave for handling vehicle entry
		CreateManagedCodeCave(CC_VEHICLEENTRY, 7, OnVehicleEntry_CC);

		// Codecave for handling weapon reloading
		CreateManagedCodeCave(CC_WEAPONRELOAD, 7, OnWeaponReload_CC);

		// Codecave for detecting player deaths
		CreateManagedCodeCave(CC_DEATH, 5, OnDeath_CC);

		// Codecave for detecting double kills, sprees etc
		CreateManagedCodeCave(CC_KILLMULTIPLIER, 5, OnKillMultiplier_CC);

		// used to control whether or not objects respawn
		CreateManagedCodeCave(CC_OBJECTRESPAWN, 32, OnObjectRespawn_CC);
		CreateManagedCodeCave(CC_EQUIPMENTDESTROY, 6, OnEquipmentDestroy_CC);

		// Codecaves for detecting vehicle ejections
		CreateManagedCodeCave(CC_VEHICLEFORCEEJECT, 8, OnVehicleForceEject_CC);
		CreateManagedCodeCave(CC_VEHICLEUSEREJECT, 7, OnVehicleUserEject_CC);
	}
}