/*
 * Copyright (C) 2014 by the Widelands Development Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef WL_UI_FSMENU_SUGGESTED_TEAMS_BOX_H
#define WL_UI_FSMENU_SUGGESTED_TEAMS_BOX_H

#include <set>
#include <string>

#include "logic/map.h"
#include "ui_basic/box.h"
#include "ui_basic/icon.h"
#include "ui_basic/textarea.h"


namespace UI {
	struct Box;

struct SuggestedTeamsBox : public UI::Box {
	SuggestedTeamsBox(Panel * parent,
							int32_t x, int32_t y,
							uint32_t orientation,
							int32_t padding, int32_t indent, int32_t label_height,
							int32_t max_x = 0, int32_t max_y = 0,
							uint32_t inner_spacing = 0);

	void hide();
	void show(std::vector<Widelands::Map::SuggestedTeamLineup*> suggested_teams);

private:
	int32_t const m_padding;
	int32_t const m_indent;
	int32_t const m_label_height;
	UI::Textarea * m_suggested_teams_box_label;
	UI::Box* m_lineup_box;
	std::vector<UI::Icon*> m_player_icons;
	std::vector<UI::Textarea*> m_vs_labels;
	std::vector<Widelands::Map::SuggestedTeamLineup*> m_suggested_teams;
};

}

#endif  // end of include guard: WL_UI_FSMENU_SUGGESTED_TEAMS_BOX_H
