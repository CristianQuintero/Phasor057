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

#pragma once

#include <windows.h>
#include "Halo.h"

#pragma pack(push, 1)

struct vect3d
{
	float x;
	float y;
	float z;
};

// Some structure issues were clarified thanks to the code at:
// http://code.google.com/p/halo-devkit/source/browse/trunk/halo_sdk/Engine/objects.h
struct G_Object // generic object header
{
	DWORD mapId; // 0x0000
	long unk[3]; // 0x0004
	char unkBits : 2; // 0x0010
	bool ignoreGravity : 1;
	char unk1 : 4;
	bool noCollision : 1; // has no collision box, projectiles etc pass right through
	char unkBits1[3];
	unsigned long timer; // 0x0014
	char empty[0x44]; // 0x0018
	vect3d location; // 0x005c
	vect3d velocity; // 0x0068
	vect3d rotation; // 0x0074 (not sure why this is used, doesn't yaw do orientation?)
	vect3d axial; // 0x0080 (yaw, pitch, roll)
	vect3d unkVector; // 0x008C (not sure, i let server deal with it)
	char unkChunk[0x28]; // 0x0098
	unsigned long ownerPlayer; // 0x00c0 (index of owner (if has one))
	unsigned long ownerObject; // 0x00c4 (object id of owner, if projectile is player id)
	char unkChunk1[0x18]; // 0x00c8
	float health; // 0x00e0
	float shield; // 0x00e4
	char unkChunk2[0x10]; // 0x00e8
	vect3d location1; // 0x00f8 set when in a vehicle unlike other one. best not to use tho (isnt always set)
	char unkChunk3[0x10]; // 0x0104
	unsigned long veh_weaponId; // 0x0114
	unsigned long player_curWeapon; // 0x0118
	unsigned long vehicleId; // 0x011C
	BYTE bGunner; // 0x0120
	short unkShort; // 0x0121
	BYTE bFlashlight; // 0x0123
	long unkLong; // 0x0124
	float shield1; // 0x0128 (same as other shield)
	float flashlightCharge; // 0x012C (1.0 when on)
	long unkLong1; // 0x0130
	float flashlightCharge1; // 0x0134
	long unkChunk4[0x2f]; // 0x0138
};

struct G_Biped
{
	char unkChunk[0x10]; // 0x1F4
	long invisible; // 0x204 (0x41 inactive, 0x51 active. probably bitfield but never seen anything else referenced)
	struct aFlags // these are action flags, basically client button presses
	{  //these don't actually control whether or not an event occurs
		bool crouching : 1; // 0x0208 (a few of these bit flags are thanks to halo devkit)
		bool jumping : 1; // 2
		char UnknownBit : 2; // 3
		bool Flashlight : 1; // 5
		bool UnknownBit2 : 1; // 6
		bool actionPress : 1; // 7 think this is just when they initially press the action button
		bool melee : 1; // 8
		char UnknownBit3 : 2; // 9
		bool reload : 1; // 11
		bool primaryWeaponFire : 1; // 12 right mouse
		bool secondaryWeaponFire : 1; // 13 left mouse
		bool secondaryWeaponFire1 : 1; // 14
		bool actionHold : 1; // 15 holding action button
		char UnknownBit4 : 1; // 16
	} actionFlags;
	char unkChunk1[0x26]; // 0x020A
	unsigned long cameraX; // 0x0230
	unsigned long cameraY; // 0x0234
	char unkChunk2[0x6f]; // 0x238
	BYTE bodyState; // 0x2A7 (2 = standing, 3 = crouching, 0xff = invalid, like in vehicle)
	char unkChunk3[0x50]; // 0x2A8
	unsigned long primaryWeaponId; // 0x2F8
	unsigned long secondaryWeaponId; // 0x2FC
	unsigned long tertiaryWeaponId; // 0x300
	unsigned long ternaryWeaponId; // 0x304
	char unkChunk4[0x18]; // 0x308
	BYTE zoomLevel; // 0x320 (0xff - no zoom, 0 - 1 zoom, 1 - 2 zoom etc)
	BYTE zoomLevel1; // 0x321
	char unkChunk5[0x1AA]; // 0x322
	BYTE isAirbourne; // 0x4CC (only when not in vehicle)
};


struct PObject // structure for objects
{
	G_Object base;
	union
	{
		G_Biped biped;
	};
};

#pragma pack(pop)

namespace objects
{
	// ----------------------------------------------------------------
	// Structure definitions
	#pragma pack(push, 1)

	// Structure of tag index table
	struct hTagIndexTableHeader
	{
		DWORD next_ptr;
		DWORD starting_index; // ??
		DWORD unk;
		DWORD entityCount;
		DWORD unk1;
		DWORD readOffset;
		BYTE unk2[8];
		DWORD readSize;
		DWORD unk3;
	};

	// Structure of the tag header
	struct hTagHeader
	{
		DWORD tagType; // ie weap
		DWORD unk[2]; // I don't know
		DWORD id; // unique id for map
		char* tagName; // name of tag
		LPBYTE metaData; // data for this tag
		DWORD unk1[2]; // I don't know
	};

	struct objInfo
	{
		DWORD tagType;
		char* tagName;
		DWORD mapObjId;
	};

	#pragma pack(pop)

	// Swaps the endian of a 32bit value
	DWORD endian_swap(unsigned int x);

	// Called when a new game starts
	void OnGameStart();

	// This function gets an object's address
	PObject* GetObjectAddress(int mode, DWORD objectId);

	// --------------------------------------------------------------------
	// Teleporting stuff
	namespace tp
	{
		// Called to load location data
		void LoadData();

		// Called to unload location data
		void Cleanup();

		// Saves a location (x,y,z) with a reference name
		bool AddLocation(std::string name, float x, float y, float z);

		// Removes a saved location
		bool RemoveLocation(int index);

		// Moves an object to the specified position
		bool MoveObject(DWORD m_objId, float x, float y, float z);
		bool MoveObject(DWORD m_objId, std::string location);

		bool sv_teleport(halo::h_player* execPlayer, std::vector<std::string>& tokens);
		bool sv_teleport_add(halo::h_player* execPlayer, std::vector<std::string>& tokens);
		bool sv_teleport_del(halo::h_player* execPlayer, std::vector<std::string>& tokens);
		bool sv_teleport_list(halo::h_player* execPlayer, std::vector<std::string>& tokens);
	}

	// ----------------------------------------------------------------
	// Tag functions
	void ClearTagCache();

	// Lookup a tag and return its header entry
	hTagHeader* LookupTag(DWORD dwMapId);
	hTagHeader* LookupTag(const char* tagType, const char* tag);

	// ----------------------------------------------------------------
	// GENERIC STUFF
	// Returns the object that controls the given object's coordinates
	// It currently only works with players in vehicles
	PObject* GetPositionalObject(PObject* m_obj);

	// ----------------------------------------------------------------
	// OBJECT SPAWNING
	bool sv_map_reset(halo::h_player* execPlayer, std::vector<std::string>& tokens);

	void ClearObjectSpawns();

	// Destroys an object
	void DestroyObject(DWORD m_objId);

	// Called when an object is being checked to see if it should respawn
	int __stdcall ObjectRespawnCheck(DWORD m_objId, LPBYTE m_obj);

	// This is called when weapons/equipment are going to be destroyed.
	bool __stdcall EquipmentDestroyCheck(int checkTicks, DWORD m_objId, LPBYTE m_obj);

	// parentId - Object id for this object's parent
	// respawnTime - Number of seconds inactive before it should respawn (0 for no respawn, -1 for gametype value)
	// bRecycle - Specifies whether or not the object should be respawn once respawnTimer has passed (if false it is destroyed)
	// location - Location of the object's respawn point
	// Return value: Created object's id, 0 on failure.
	// Note: When spawning items with an item collection tag (weapons, nades
	// powerups) bRecycle is ignored and respawnTime is considered to be
	// destruction time (time until object is destroyed)
	DWORD SpawnObject(const char* tagType, const char* tagName, DWORD parentId=0, int respawnTime=-1, bool bRecycle=false, vect3d* location=0);

	// Forces a player to equip (and change to) the specified weapon.
	bool AssignPlayerWeapon(halo::h_player* player, DWORD m_weaponId);

	// Forces a player into a vehicle
	// Seat numbers: 0 (driver) 1 (passenger) 2 (gunner)
	void EnterVehicle(halo::h_player* player, DWORD m_vehicleId, DWORD seat);

	// Forces a player to exit a vehicle
	void ExitVehicle(halo::h_player* player);
}
