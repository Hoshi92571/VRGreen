#pragma once
#include "Discord.hpp"

#include <list>

struct Variables // TODO: move to Hack.hpp
{
	inline static bool speedHack;
	inline static bool offlineMode;
	inline static bool forceMute;
	inline static bool autoDestroy;
	inline static bool worldTriggers;
	inline static bool antiPortal;
	inline static bool printApiRequests;
	inline static bool spamEmoji;
	inline static bool testing;
	inline static bool fly;
	inline static float flySpeed;
	inline static std::list<std::string> whiteList;
	inline static Discord* g_Discord;
};