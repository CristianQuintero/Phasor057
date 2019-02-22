#pragma once

#include "Halo.h"
#include <string>
#include <vector>

namespace messages
{
	namespace welcome
	{
		// Server commands
		bool sv_welcome_add(halo::h_player* exec, std::vector<std::string> & tokens);
		bool sv_welcome_del(halo::h_player* exec, std::vector<std::string> & tokens);
		bool sv_welcome_list(halo::h_player* exec, std::vector<std::string> & tokens);

		// Called when a player joins
		void OnPlayerJoin(halo::h_player* player);
	}

	namespace automated
	{
		// server commands
		bool sv_msg_add(halo::h_player* exec, std::vector<std::string> & tokens);
		bool sv_msg_del(halo::h_player* exec, std::vector<std::string> & tokens);
		bool sv_msg_list(halo::h_player* exec, std::vector<std::string> & tokens);	
		bool sv_msg_start(halo::h_player* exec, std::vector<std::string> & tokens);
		bool sv_msg_end(halo::h_player* exec, std::vector<std::string> & tokens);
		
		// events that are needed
		void GameEnd();
		void GameStart();
	}
}
