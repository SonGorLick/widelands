/*
 * Copyright (C) 2002-2003, 2006-2007 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __S__INTPLAYER_H
#define __S__INTPLAYER_H

#include "game.h"
#include "interactive_base.h"
#include "network.h" // For chat

#include <vector>

class Player;
namespace UI {
struct Multiline_Textarea;
struct Textarea;
};

/** class Interactive_Player
 *
 * This is the interactive player. this one is
 * responsible to show the correct map
 * to the player and draws the user interface,
 * cares for input and so on.
 */
class Interactive_Player : public Interactive_Base {
public:
	struct Game_Main_Menu_Windows {
		UI::UniqueWindow::Registry loadgame;
		UI::UniqueWindow::Registry savegame;
		UI::UniqueWindow::Registry readme;
		UI::UniqueWindow::Registry keys;
		UI::UniqueWindow::Registry authors;
		UI::UniqueWindow::Registry licence;
		UI::UniqueWindow::Registry options;

		UI::UniqueWindow::Registry building_stats;
		UI::UniqueWindow::Registry general_stats;
		UI::UniqueWindow::Registry ware_stats;
		UI::UniqueWindow::Registry stock;

		UI::UniqueWindow::Registry mission_objectives;
		UI::UniqueWindow::Registry chat;
		UI::UniqueWindow::Registry objectives;
	};

public:
	Interactive_Player(Game &, const uchar pln);
	~Interactive_Player();

	virtual void think();

	void start();

	void main_menu_btn();
	void toggle_buildhelp();
	void open_encyclopedia();

	void field_action();

	bool handle_key(bool down, int code, char c);

	Game * get_game() const {return m_game;}
	uchar get_player_number() const {return m_player_number;}
	Player & player() const throw () {return m_game->player(m_player_number);}
	Player * get_player() const throw () {
		assert(m_game);
		return m_game->get_player(m_player_number);
	}

	// for savegames
	void set_player_number(uint plrn);

	// For load
	virtual void cleanup_for_load(void);
	void postload();

	// Chat messages
	bool show_chat_overlay(void) { return m_do_chat_overlays; }
	void set_show_chat_overlay(bool t) { m_do_chat_overlays = t; }
	const std::vector< NetGame::Chat_Message >* get_chatmsges(void) {
		return &m_chatmsges;
	}

private:
	struct Overlay_Chat_Messages {
		NetGame::Chat_Message msg;
		uint starttime;
	};

private:
	Game                     * m_game;
	Player_Number m_player_number;

	UI::Textarea             * m_label_speed;
	UI::Multiline_Textarea   * m_chat_messages;
	UI::Textarea             * m_type_message;

	UI::UniqueWindow::Registry m_mainmenu;
	UI::UniqueWindow::Registry m_fieldaction;
	UI::UniqueWindow::Registry m_encyclopedia;
	Game_Main_Menu_Windows  m_mainm_windows;

	// Chat message stack
	std::vector< NetGame::Chat_Message > m_chatmsges;
	std::vector< Overlay_Chat_Messages > m_show_chatmsg;
	bool m_do_chat_overlays;
	bool m_is_typing_msg; // Is the user typing a chat message
	std::string m_typed_message;
};


#endif // __S__INTPLAYER_H
