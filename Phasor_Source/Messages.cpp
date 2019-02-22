#include "Messages.h"
#include "Common.h"
#include "Timers.h"
#include "Server.h"

namespace messages
{
	namespace welcome
	{
		using namespace halo;
		std::vector<std::wstring> msgs;

		bool sv_welcome_add(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (tokens.size() == 2)
			{
				msgs.push_back(ToWideChar(tokens[1]));
				if (exec)
					hprintf(L"The welcome message has been added.");
			}
			else
				hprintf(L"Correct usage: sv_welcome_add \"message\"");

			return true;
		}

		bool sv_welcome_del(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (tokens.size() == 2)
			{
				// make sure the index is valid
				size_t index = atoi(tokens[1].c_str());
				if (index >= 0 && index < msgs.size())
				{
					msgs.erase(msgs.begin() + index);
					hprintf(L"The specified welcome message has been removed.");
				}
				else
					hprintf(L"The supplied index was invalid.");
			}
			else
				hprintf("Correct usage: sv_welcome_del <index>");

			return true;
		}

		bool sv_welcome_list(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			hprintf(L"Current welcome message: (index, message)");

			for (size_t x = 0; x < msgs.size(); x++)
				hprintf(L"%i, %s", x, msgs[x].c_str());

			return true;
		}

		void OnPlayerJoin(halo::h_player* player)
		{
			// send the welcome messages
			for (size_t x = 0; x < msgs.size(); x++)
				player->ServerMessage(msgs[x].c_str());
		}
	}

	namespace automated
	{
		using namespace halo;
		std::vector<std::wstring> msgs;
		DWORD msgTimer = 0, delay = 0;

		bool broadcast_callback(LPARAM unused)
		{
			for (size_t x = 0; x < msgs.size(); x++)
				server::GlobalMessage(L"%s", msgs[x].c_str());

			return true; // keep the timer
		}

		bool sv_msg_add(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (tokens.size() == 2)
			{
				msgs.push_back(ToWideChar(tokens[1]));
				if (exec)
					hprintf(L"The message has been added to the broadcast list.");
			}
			else
				hprintf(L"Correct usage: sv_msg_add \"message\"");

			return true;
		}

		bool sv_msg_del(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (tokens.size() == 2)
			{
				// make sure the index is valid
				size_t index = atoi(tokens[1].c_str());
				if (index >= 0 && index < msgs.size())
				{
					msgs.erase(msgs.begin() + index);
					hprintf(L"The specified message has been removed.");
				}
				else
					hprintf(L"The supplied index was invalid.");
			}
			else
				hprintf(L"Correct usage: sv_msg_del <index>");

			return true;
		}

		bool sv_msg_list(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			hprintf(L"Current broadcast message: (index, message)");

			for (size_t x = 0; x < msgs.size(); x++)
				hprintf(L"%i, %s", x, msgs[x].c_str());

			return true;
		}

		bool sv_msg_start(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (tokens.size() == 2)
			{
				if (msgs.size())
				{
					int delay_in_min = atoi(tokens[1].c_str());

					if (delay_in_min > 0)
					{
						// if the message is already going, stop it
						if (msgTimer)
							Timer::RemoveTimer(msgTimer);

						delay = delay_in_min * 60000;
						msgTimer = Timer::CreateTimer(delay, 0, broadcast_callback);
						hprintf(L"The messages will be broadcast every %i minutes", delay_in_min);
					}
					else
						hprintf(L"You need to enter a delay of at least one minute.");
				}
				else
					hprintf(L"The broadcast list is empty, use sv_msg_add to add messages.");
			}
			else
				hprintf(L"Correct usage: sv_msg_start <delay in minutes>");

			return true;
		}

		bool sv_msg_end(halo::h_player* exec, std::vector<std::string> & tokens)
		{
			if (delay)
			{
				if (msgTimer)
					Timer::RemoveTimer(msgTimer);

				delay = 0;
				hprintf(L"Message broadcasting has been disabled.");
			}
			else
				hprintf(L"Message broadcasting isn't currently active.");

			return true;
		}

		void GameEnd()
		{
			// remove the timer (it's added again when next game starts)
			if (msgTimer)
			{
				Timer::RemoveTimer(msgTimer);
				msgTimer = 0;
			}
		}

		void GameStart()
		{
			if (msgTimer) // if there's currently a timer, remove it.
			{
				Timer::RemoveTimer(msgTimer);
				msgTimer = 0;
			}

			if (delay)
				msgTimer = Timer::CreateTimer(delay, 0, broadcast_callback);
		}
	}
}