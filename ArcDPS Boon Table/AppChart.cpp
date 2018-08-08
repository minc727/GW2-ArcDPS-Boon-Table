#include "AppChart.h"



AppChart::AppChart()
{
	active_player = -1;
	active_column = -1;
	last_active_player = -1;
	last_active_column = -1;
	sorting_collumn = 1;
}


AppChart::~AppChart()
{
}

void AppChart::Draw(const char* title, bool* p_open = nullptr, Tracker* tracker = nullptr,ImGuiWindowFlags flags)
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiSetCond_FirstUseEver);
	ImGui::Begin(title, p_open, flags);
	ImGui::PushAllowKeyboardFocus(false);
	last_active_player = active_player;
	last_active_column = active_column;
	active_player = INDEX_NONE;
	active_column = INDEX_NONE;
	float current_boon_uptime = 0.0f;

	std::lock_guard<std::mutex> lock(tracker->players_mtx);

	//menu
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Show..."))
		{
			active_player = INDEX_HIDE_ALL;
			active_column = INDEX_HIDE_ALL;

			ImGui::MenuItem("Players", NULL, &show_players);
			ImGui::MenuItem("Subgroups", NULL, &show_subgroups);
			ImGui::MenuItem("Total", NULL, &show_total);

			if (ImGui::BeginMenu("Buffs"))
			{
				for (auto boon = tracked_buffs.begin(); boon != tracked_buffs.end(); ++boon)
				{
					ImGui::MenuItem(boon->name.c_str(), NULL, &boon->is_relevant);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	int column_number = 2;
	for (auto current_buff : tracked_buffs)
	{
		if (current_buff.is_relevant) column_number++;
	}
	
	if (bShowPlayers(tracker))
	{
		ImGui::Columns(column_number, "Players");
		if (highlightedSmallButton(INDEX_SORTING_BUTTON, "Name")) tracker->setSortMethod(name);

		for (std::list<Player>::iterator current_player = tracker->players.begin(); current_player != tracker->players.end(); ++current_player)
		{
			highlightedText(current_player->id, current_player->name.c_str());
		}

		//show player subgroup numbers
		ImGui::NextColumn();
		if (highlightedSmallButton(INDEX_SORTING_BUTTON, "Subgrp")) tracker->setSortMethod(subgroup);

		for (std::list<Player>::iterator current_player = tracker->players.begin(); current_player != tracker->players.end(); ++current_player)
		{
			highlightedText(current_player->id, "%d", current_player->subgroup);
		}

		//show boon uptimes
		for (std::list<BoonDef>::iterator current_buff = tracked_buffs.begin(); current_buff != tracked_buffs.end(); ++current_buff)
		{
			if (!current_buff->is_relevant)continue;

			ImGui::NextColumn();

			if (highlightedSmallButton(INDEX_SORTING_BUTTON, current_buff->name.c_str())) tracker->setSortMethod(boon, &*current_buff);

			//players
			for (std::list<Player>::iterator current_player = tracker->players.begin(); current_player != tracker->players.end(); ++current_player)
			{
				current_boon_uptime = current_player->getBoonUptime(&*current_buff);

				buffProgressBar(&*current_buff, current_boon_uptime, current_player->id);
			}
		}
		ImGui::Columns(1);
	}

	if (bShowSubgroups(tracker))
	{
		ImGui::Separator();

		ImGui::Columns(column_number,"Subgroups");

		for (auto current_subgroup : tracker->subgroups)
		{
			highlightedText(current_subgroup, "Subgroup");
		}
		
		ImGui::NextColumn();

		for (std::list<uint8_t>::iterator current_subgroup = tracker->subgroups.begin(); current_subgroup != tracker->subgroups.end(); ++current_subgroup)
		{
			highlightedText(*current_subgroup, "%d", *current_subgroup);
		}

		for (std::list<BoonDef>::iterator current_buff = tracked_buffs.begin(); current_buff != tracked_buffs.end(); ++current_buff)
		{
			if (!current_buff->is_relevant)continue;

			ImGui::NextColumn();

			for (auto current_subgroup : tracker->subgroups)
			{
				current_boon_uptime = tracker->getSubgroupBoonUptime(&*current_buff, current_subgroup);

				buffProgressBar(&*current_buff, current_boon_uptime, current_subgroup);
			}
		}
		ImGui::Columns(1);
	}
	
	if (bShowTotal(tracker))
	{
		ImGui::Separator();

		ImGui::Columns(column_number, "Total");

		highlightedText(INDEX_TOTAL, "Total");

		ImGui::NextColumn();
		
		highlightedText(INDEX_TOTAL, "All");

		for (std::list<BoonDef>::iterator current_buff = tracked_buffs.begin(); current_buff != tracked_buffs.end(); ++current_buff)
		{
			if (!current_buff->is_relevant)continue;

			ImGui::NextColumn();

			current_boon_uptime = tracker->getAverageBoonUptime(&*current_buff);

			buffProgressBar(&*current_buff, current_boon_uptime, INDEX_TOTAL);
		}
		ImGui::Columns(1);
	}

	ImGui::PopAllowKeyboardFocus();
	ImGui::End();
}

void AppChart::buffProgressBar(BoonDef* current_buff, float current_boon_uptime, uintptr_t current_player)
{
	if (last_active_player == current_player || last_active_column == ImGui::GetColumnIndex())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, active_bar_color);
	}
	else if (last_active_player != -1 || last_active_column != -1)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, hidden_bar_color);
	}
	if (current_buff->is_duration_stacking)
	{
		ImGui::ProgressBar(current_boon_uptime, ImVec2(-1, ImGui::GetFontSize()));
	}
	else
	{
		char label[5];
		sprintf(label, "%.1f", current_boon_uptime);
		current_boon_uptime /= 25;
		ImGui::ProgressBar(current_boon_uptime, ImVec2(-1, ImGui::GetFontSize()), label);
	}
	if (last_active_player != -1 || last_active_column != -1)
	{
		ImGui::PopStyleColor();
	}
	if (ImGui::IsItemHovered())
	{
		active_player = current_player;
		active_column = ImGui::GetColumnIndex();
	}
}

void AppChart::highlightedText(uintptr_t player_id, const char* fmt, ...)
{
	if (last_active_player == player_id || last_active_column == ImGui::GetColumnIndex())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, active_bar_color);
	}
	else if (last_active_player != -1 || last_active_column != -1)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, hidden_bar_color);
	}
	va_list args;
	va_start(args, fmt);
	ImGui::TextV(fmt,args);
	va_end(args);

	if (ImGui::IsItemHovered())
	{
		active_player = player_id;
		active_column = ImGui::GetColumnIndex();
	}
	if (last_active_player != -1 || last_active_column != -1)
	{
		ImGui::PopStyleColor();
	}
}

bool AppChart::highlightedSmallButton(uintptr_t player_id, const char* fmt)
{
	if (last_active_column == ImGui::GetColumnIndex())
	{
		ImGui::PushStyleColor(ImGuiCol_Text, active_bar_color);
	}
	else if (last_active_player != -1 || last_active_column != -1)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, hidden_bar_color);
	}
	bool out = ImGui::SmallButton(fmt);

	if (ImGui::IsItemHovered())
	{
		active_player = player_id;
		active_column = ImGui::GetColumnIndex();
	}
	if (last_active_player != -1 || last_active_column != -1)
	{
		ImGui::PopStyleColor();
	}
	return out;
}

void AppChart::setShowPlayers(bool new_show)
{
	show_players = new_show;
}

void AppChart::setShowSubgroups(bool new_show)
{
	show_subgroups = new_show;
}

void AppChart::setShowTotal(bool new_show)
{
	show_total = new_show;
}

bool AppChart::bShowPlayers(Tracker * tracker)
{
	return show_players;
}

bool AppChart::bShowSubgroups(Tracker* tracker)
{
	return show_subgroups && tracker->subgroups.size()>1;
}

bool AppChart::bShowTotal(Tracker* tracker)
{
	return show_total && tracker->players.size() > 1;
}
