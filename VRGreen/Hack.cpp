﻿#include "Hack.hpp"

//#define SENDINFO

#pragma comment(lib, "urlmon.lib")
#include <urlmon.h>
#pragma comment(lib, "winmm.lib")
// for sounds
#include <regex>


#include "Variables.hpp"
#include "Application.hpp"
#include <HardOffsets.hpp>
#include "VRCQuickMenu.hpp"
#include "VRCSocialMenu.hpp"
#include "Misc.hpp"
#include "VRC/Player.h"
#include "VRC/Core/APIUser.hpp"
#include "VRC/PlayerManager.hpp"
#include "Selectable.hpp"
#include "IL2CPP/IL2CPP.hpp"
#include "Text.hpp"
#include "Graphic.hpp"
#include "VRCPlayer.hpp"
#include <VRCUiShadowPlate.hpp>
#include "Assembly-CSharp/VRCUiManager.hpp"
#include "VRC/SDKBase/Networking.hpp"
#include "VRCPlayerApi.h"
#include "UnityEngine/GameObject.hpp"
#include "Renderer.hpp"
#include "Collider.hpp"
#include "Input.hpp"
#include "Camera.hpp"
#include "Time.hpp"
#include "sha1.hpp"
#include "ConsoleUtils.hpp"
#include "Material.hpp"
#include "HighlightsFX.hpp"
#include "ApiAvatar.hpp"
#include "API.hpp"
#include "ccs.hpp"
#include "Image.hpp"
#include "Ray.hpp"
#include "RaycastHit.hpp"
#include "PageAvatar.hpp"
#include "Type.hpp"
#include "SystemList.hpp"
#include "ExtendedAvatars.hpp"
#include <sstream>
#include "EventData.hpp"
#include "UserInteractMenu.hpp"
#include "ProperBitConverter.hpp"

using namespace UnityEngine;


static std::regex nigger("(<color=#?\\w+)>");


struct SendInfoParams
{
	std::string displayName;
	std::string location;
	std::string id;
	std::string ip;
	std::string last_update;
	std::string key;
	std::string hwid;
	List<VRC::Player*> players;
};

std::string FormatMyName(VRC::Player* player)
{
	auto apiuser = player->GetAPIUser();
	auto userid = apiuser->getId();
	bool isFriend = VRC::Core::APIUser::isFriendsWith(userid);

	if (userid == VRC::Core::APIUser::currentUser()->getId())
		return std::string("<color=#ee82ee>" + apiuser->displayName() + "</color>");
	else if (isFriend)
		return std::string("<color=yellow>" + apiuser->displayName() + "</color>");
	else
		return apiuser->displayName();
}

struct Hack::mode
{
	using lambda_t = void(*)(); // use lambda_t instead of void* because of compiler bug

	std::string name;
	long offset;
	bool set;
	void* trueFunc;
	lambda_t detourFunc;

	[[nodiscard]]
	bool ApplyDetour(HANDLE ThreadId)
	{
		if (set)
		{
			if (::DetourTransactionBegin() != NO_ERROR)
			{
				ConsoleUtils::Log("DetourTransactionBegin ERROR: ", name);
				return false;
			}
			if (::DetourUpdateThread(ThreadId) != NO_ERROR)
			{
				ConsoleUtils::Log("DetourUpdateThread ERROR: ", name);
				return false;
			}
			if (::DetourAttach(&trueFunc, detourFunc) != NO_ERROR)
			{
				ConsoleUtils::Log("DetourAttach ERROR", name);
				return false;
			}
			if (::DetourTransactionCommit() != NO_ERROR)
			{
				ConsoleUtils::Log("DetourTransactionCommit ERROR: ", name);
				return false;
			}
		}
		else
		{
			if (::DetourTransactionBegin() != NO_ERROR)
			{
				ConsoleUtils::Log("DetourTransactionBegin ERROR: ", name);
				return false;
			}
			if (::DetourUpdateThread(ThreadId) != NO_ERROR)
			{
				ConsoleUtils::Log("DetourUpdateThread ERROR: ", name);
				return false;
			}
			if (::DetourDetach(&trueFunc, detourFunc) != NO_ERROR)
			{
				ConsoleUtils::Log("DetourDetach ERROR: ", name);
				return false;
			}
			if (::DetourTransactionCommit() != NO_ERROR)
			{
				ConsoleUtils::Log("DetourTransactionCommit ERROR: ", name);
				return false;
			}
		}

		ConsoleUtils::Log(magenta, name, (set ? green : red), (set ? " ON" : " OFF"));

		return true;
	}
};

Hack& Hack::getInstance()
{
	static Hack hack;
	return hack;
}

void Hack::init()
{
	srand(time(0));

	threadId = GetCurrentThread();

	::GameAssemblyHandle = GetModuleHandleA("GameAssembly.dll");

	downloadSettings();

	setupVariables();

	setupSettings(); // Sets up all offsets and names

	initDetours(); // Sets up all detours

	ConsoleUtils::CreateConsole(); // Calling later because initDetours() spams console too much

	Variables::g_Discord->Initalize();
	//generateHWID();
}

void Hack::mainLoop()
{
	while (true)
	{
		if (!isTyping)
		{
			if (IsFocused())
			{
				if (::GetAsyncKeyState(0x12) && ::GetAsyncKeyState(0x52) & 1) // LEFT ALT + R
				{
					PlaySound(L"SerOn", GetModuleHandle(0), SND_RESOURCE | SND_ASYNC);
					Variables::serialize = !Variables::serialize;
					ConsoleUtils::Log(magenta, "Serialize ", (Variables::serialize ? green : red), (Variables::serialize ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(0x32) & 1)
				{
					Variables::speedHack = !Variables::speedHack;
					ConsoleUtils::Log(magenta, "Speed Hack ", (Variables::speedHack ? green : red), (Variables::speedHack ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(0x33) & 1)
				{
					gravityHack = !gravityHack;
					ConsoleUtils::Log(magenta, "Gravity Hack ", (gravityHack ? green : red), (gravityHack ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(0x34) & 1)
				{
					Variables::forceMute = !Variables::forceMute;
					ConsoleUtils::Log(magenta, "Force Mute ", (Variables::forceMute ? green : red), (Variables::forceMute ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(0x35) & 1)
				{
					toggleRPC = !toggleRPC;
					ConsoleUtils::Log(magenta, "RPC printing ", (toggleRPC ? green : red), (toggleRPC ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(0x36) & 1)
				{
					Variables::autoDestroy = !Variables::autoDestroy;
					ConsoleUtils::Log(magenta, "Auto Destroy ", (Variables::autoDestroy ? green : red), (Variables::autoDestroy ? "ON" : "OFF"));
				}


				if (::GetAsyncKeyState(0x37) & 1)
				{
					toggleSetting("BlackScreenPatch0");
					toggleSetting("BlackScreenPatch2");
				}

				if (::GetAsyncKeyState(0x38) & 1)
				{
					toggleSetting("TriggerEvent1");
					toggleSetting("TriggerEvent2");
					toggleSetting("TriggerEvent3");
					toggleSetting("TriggerEvent4");
					toggleSetting("TriggerEvent5");
					
					//Variables::testing = !Variables::testing;
					//ConsoleUtils::Log(magenta, "Testing ", (Variables::testing ? green : red), (Variables::testing ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(VK_NUMPAD1) & 1)
				{
					toggleSetting("KickUserRPC");
					toggleSetting("WarnUserRPC");
				}

				if (::GetAsyncKeyState(VK_NUMPAD3) & 1)
					toggleSetting("Infinite Portals");

				if (::GetAsyncKeyState(VK_NUMPAD4) & 1)
				{
					noGlobal = !noGlobal;
					ConsoleUtils::Log(magenta, "No Global ", (noGlobal ? green : red), (noGlobal ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(VK_NUMPAD5) & 1)
					toggleSetting("Offline Mode");

				if (::GetAsyncKeyState(VK_NUMPAD6) & 1)
				{
					Variables::worldTriggers = !Variables::worldTriggers;
					ConsoleUtils::Log(magenta, "World Triggers ", (Variables::worldTriggers ? green : red), (Variables::worldTriggers ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(VK_NUMPAD7) & 1)
					toggleSetting("Fake Ping");

				/*	if (::GetAsyncKeyState(VK_NUMPAD8) & 1)
					{
						seeBlocked = !seeBlocked;
						ConsoleUtils::Log(magenta, "See Blocked ", (seeBlocked ? green : red), (seeBlocked ? "ON" : "OFF"));
					}*/
				if (::GetAsyncKeyState(VK_NUMPAD9) & 1)
					promptRoomID();

				if (::GetAsyncKeyState(VK_F9) & 1)
				{
					Variables::spamEmoji = !Variables::spamEmoji;
					ConsoleUtils::Log(magenta, "Emoji Spam ", (Variables::spamEmoji ? green : red), (Variables::spamEmoji ? "ON" : "OFF"));
				}

				if (::GetAsyncKeyState(VK_F10) & 1)
				{
					Variables::portalLag = !Variables::portalLag;
					ConsoleUtils::Log(magenta, "Portal Lag ", (Variables::portalLag ? green : red), (Variables::portalLag ? "ON" : "OFF"));
				}
			}

			if (::GetAsyncKeyState(VK_DELETE) & 1)
			{
				break; // Stop's main loop 
					   // DLL will be unloaded
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(4));
	}
}

void Hack::destroy()
{
	disableDetours();

	{
		std::unique_lock lck(DetourMutex);
	}

	VRCQuickMenu::ClearInterface();
	VRCSocialMenu::ClearInterface();

	Variables::g_Discord->Shutdown();

	ConsoleUtils::Log("Client has been turned off");
	ConsoleUtils::Log("To enable it again press INSERT");
	ConsoleUtils::DestroyConsole();
}

void Hack::setupVariables()
{
	Variables::g_Discord = new	Discord();

	emojiSpamDelay = 1.f;
	portalKOSDelay = 11.f;
	updateDelay = 1.f;
	isTyping = false;
	Variables::forceMute = true;
	seeBlocked = true;
	Variables::speedHack = false;
	gravityHack = false;
	emojiSpam = true;
	toggleRPC = true;
	Variables::autoDestroy = false;
	disableTriggers = false;
	Variables::fakePing = true;
	Variables::portalKOS = true;
	Variables::worldTriggers = true;
	Variables::customPlates = true;
	Variables::friendRequestSent = false;
	Variables::instanceLock = false;
	Variables::rpcBlockNonFriends = false;
	Variables::fly = false;
	Variables::portalLag = false;
	Variables::antiWorldTriggers = true;
	Variables::esp = true;
	Variables::antiPortal = true;
	Variables::serialize = false;
	Variables::offlineMode = false;
	Variables::blockRPCs = false;
	Variables::spamEmoji = true;
	Variables::testing = true;
	noGlobal = true;
	lobbyLag = false;
	menuInitialized = false;
}

void Hack::downloadSettings()
{
	{
		URLDownloadToFile(NULL, L"http://vrgreen.xyz/client/settings/users.txt", L"users.txt", 0, NULL);

		std::string line;
		std::ifstream myfile("users.txt");
		if (myfile.is_open())
		{
			while (getline(myfile, line))
			{
				clientUsers.push_back(line);
			}
			myfile.close();
		}
	}
}

void Hack::generateHWID()
{
	ConsoleUtils::Log("Generating fake HWID");
	using TrueFunc_t = decltype(&get_DeviceId);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("DeviceId").trueFunc;
	ConsoleUtils::Log("HWID Generated: ", IL2CPP::StringChars(TrueFunc()));
}

void Hack::disableDetours()
{
	for (auto& mode : settings)
	{
		if (mode.set)
		{
			mode.set = false;
			if (!mode.ApplyDetour(GetCurrentThread()))
			{
				return;
			}
		}
	}
}

void* Hack::AvatarFav(void* _this, void* list, int sex, char nigger, void* maybe)
{
	//SystemList<Object> myList(list);

	//ConsoleUtils::Log(System::Type::ToString(System::Type::GetType(myList[0])));
	//return nullptr;
	//ConsoleUtils::Log(sex);
	//ConsoleUtils::Log((int)nigger);
	
	using TrueFunc_t = decltype(&AvatarFav);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("AvatarFav").trueFunc;
	return TrueFunc(_this, list, sex, nigger, maybe);
}

void Hack::RPCS(void* _this, int xxx, int VrcTargetType, UnityEngine::GameObject* gameObject, IL2CPP::String* RPC, void* bytes)
{
	if (Variables::blockRPCs)
		return;

	using func_t = void* (*)(void* bytes);
	func_t func = GetMethod<func_t>(RPCS_INTERNAL_FUNC);
	auto objarray = func(bytes);

	//ConsoleUtils::Log(IL2CPP::StringChars(RPC));

	auto player = VRC::PlayerManager::GetPlayer(xxx);
	auto apiuser = player->GetAPIUser();
	auto userid = apiuser->getId();


	if (userid == VRC::Core::APIUser::currentUser()->getId())
	{
		using TrueFunc_t = decltype(&RPCS);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("RPCS").trueFunc;
		TrueFunc(_this, xxx, VrcTargetType, gameObject, RPC, bytes);
		return;
	}

	if (Variables::rpcBlockNonFriends && Misc::GetUserRankName(apiuser) == "Visitor")
		return;

	if (Variables::rpcBlocked.find(userid) != Variables::rpcBlocked.end())
		return;



	using TrueFunc_t = decltype(&RPCS);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("RPCS").trueFunc;
	TrueFunc(_this, xxx, VrcTargetType, gameObject, RPC, bytes);
}

void Hack::OnEvent(void* _this, EventData* eventData)
{
	int code = eventData->GetCode();

	if (Variables::blockRPCs && code != 255 && code != 254)
		return;

	int playerId = DictionaryGet<int>(eventData->GetParameters(), (void*)254, "System.Byte");
	
	if (code != 255 && code != 254)
	{
		if (playerId != 0)
		{
			auto player = VRC::PlayerManager::GetPlayer(playerId);
			auto apiuser = player->GetAPIUser();
			auto userid = apiuser->getId();

			if (apiuser->getId() == VRC::Core::APIUser::currentUser()->getId())
			{
				using TrueFunc_t = decltype(&OnEvent);
				TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("OnEvent").trueFunc;
				TrueFunc(_this, eventData);
				return;
			}

			if (Variables::rpcBlockNonFriends && Misc::GetUserRankName(apiuser) == "Visitor") // TAKE THIS SHIT OUT!!!
				return;

			if (Variables::rpcBlocked.find(userid) != Variables::rpcBlocked.end())
				return;

			//if (Firewall.isExceedingCallLimit(Convert.ToString(__0.Code), apiuser2.id)) // TODO: do this
			//if (Variables::rpcBlocked.find(userid) != Variables::rpcBlocked.end() || (Variables::rpcBlockNonFriends && !VRC::Core::APIUser::isFriendsWith(userid)))
			//	return;
			//Firewall.addToCallHistory(Convert.ToString(__0.Code), apiuser2, false);
		}
	}

	if (code > 206)
	{
		if (code == 223)
		{
			ConsoleUtils::Log(white, "[223] Authenticate on server-side");
			ConsoleUtils::VRLog("[223] Authenticate on server-side");
		}
		else if (code == 224)
		{
			ConsoleUtils::Log(white, "[224] LobbyStats");
			ConsoleUtils::VRLog("[224] LobbyStats");
		}
		else
		{
			switch (code)
			{
			case 253:
				// Avatar Change
				if (playerId != 0)
				{
					auto apiuser = VRC::PlayerManager::GetPlayer(playerId)->GetAPIUser();
					ConsoleUtils::Log("[253] Set Properties by ", apiuser->displayName());
				}
				break;
			case 254:
			{
				auto player = VRC::PlayerManager::GetPlayer(playerId);
				if (getInstance().toggleRPC)
				{
					auto id = player->GetAPIUser()->getId();

					ConsoleUtils::Log(red, player->ToString(), " LEFT ", white, "[", id, "]");

					if (!VRC::Core::APIUser::isFriendsWith(id))
					{
						ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>left</color>");
					}
					else
					{
						//VRCUiManager::VRCUiManagerInstance()->HudMsg(player->ToString() + " left");
						ConsoleUtils::VRLog("<color=yellow>" + FormatMyName(player) + "</color>" + " <color=red>left</color>");
					}
				}
				break;
			}
			case 255:
				break;
			default:
				break;
			}
		}
	}
	else if (code == 202)
	{
	}
	else if (code == 206)
	{
		ConsoleUtils::Log(white, "Send Serialize Reliable by unknown player");
		ConsoleUtils::VRLog("Send Serialize Reliable by unknown player");
	}


	using TrueFunc_t = decltype(&OnEvent);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("OnEvent").trueFunc;
	TrueFunc(_this, eventData);
}

static bool DynPrefPrefix(void* _this, VRC::Player* player, IL2CPP::String* str)
{
	ConsoleUtils::Log(cyan, player->ToString(), " spawned object " + IL2CPP::StringChars(str));
	return true;
}

static bool KickBanTesting(void* _this, IL2CPP::String* str)
{
	std::string text = IL2CPP::StringChars(str);

	ConsoleUtils::Log(text);
	return false;
}

static bool AntiPublicWorldBan(void* _this, void* str)
{
	return false;
}

static bool BlackScreenPatch()
{
	//ConsoleUtils::Log("Black screen patch");
	return true;
}

static void AntiLogout(void* _this, void* reason)
{
	ConsoleUtils::Log("Logout? ", System::Type::ToString(reason));
}

int Hack::Send(void* _this, void* buffer)
{
	ConsoleUtils::Log(5);

	using TrueFunc_t = decltype(&Send);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("Send").trueFunc;
	return TrueFunc(_this, buffer);
}

void Hack::TakeOwnership(void* _this, VRC::SDKBase::VRCPlayerApi* vrcplayerapi)
{
	ConsoleUtils::Log(VRC::PlayerManager::GetPlayer(vrcplayerapi->PlayerId())->GetAPIUser()->displayName(), " takes ownership");
	ConsoleUtils::VRLog(VRC::PlayerManager::GetPlayer(vrcplayerapi->PlayerId())->GetAPIUser()->displayName() + " takes ownership");

	using TrueFunc_t = decltype(&TakeOwnership);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("TakeOwnership").trueFunc;
	TrueFunc(_this, vrcplayerapi);
}

void Hack::DestroyObject(void* _this, int xxx)
{
	using TrueFunc_t = decltype(&DestroyObject);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("DestroyObject").trueFunc;
	TrueFunc(_this, xxx);
}

void* Hack::TransportProtocolGet(void* _this)
{
	ConsoleUtils::Log("protocol get");

	using TrueFunc_t = decltype(&TransportProtocolGet);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("TransportProtocolGet").trueFunc;
	return TrueFunc(_this);
}

void Hack::TransportProtocolSet(void* _this, int value)
{
	/*ConsoleUtils::Log("protocol get");
	int protocol = 1;
	return &protocol;*/

	ConsoleUtils::Log("protocol is ", value, " and was set to 5");

	using TrueFunc_t = decltype(&TransportProtocolSet);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("TransportProtocolSet").trueFunc;
	return TrueFunc(_this, 5);
}

void Hack::RoundTripTimePhoton(void* _this, void* data, int length)
{
	ConsoleUtils::Log(length);



	using TrueFunc_t = decltype(&RoundTripTimePhoton);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("RoundTripTimePhoton").trueFunc;
	TrueFunc(_this, data, length);
}

void Hack::DisconnectReason(void* _this, int reason)
{
		// 0 None,
		// 1 ExceptionOnConnect,
		// 2 Exception,
		// 3 ServerTimeout,
		// 4 ClientTimeout,
		// 5 DisconnectByServerLogic,
		// 6 DisconnectByServerReasonUnknown,
		// 7 InvalidAuthentication,
		// 8 CustomAuthenticationFailed,
		// 9 AuthenticationTicketExpired,
		// 10 MaxCcuReached,
		// 11 InvalidRegion,
		// 12 OperationNotAllowedInCurrentState,
		// 13 DisconnectByClientLogic

	switch (reason)
	{
	case 0:
	{
		ConsoleUtils::Log("Disconnected by None");
		break;
	}
	case 1:
	{
		ConsoleUtils::Log("Disconnected by ExceptionOnConnect");
		break;
	}
	case 2:
	{
		ConsoleUtils::Log("Disconnected by Exception");
		break;
	}
	case 3:
	{
		ConsoleUtils::Log("Disconnected by ServerTimeout");
		break;
	}
	case 4:
	{
		ConsoleUtils::Log("Disconnected by ClientTimeout");
		break;
	}
	case 5:
	{
		ConsoleUtils::Log("Disconnected by server logic");
		break;
	}
	case 6:
	{
		ConsoleUtils::Log("Disconnected by server reason unknown");
		break;
	}
	case 7:
	{
		ConsoleUtils::Log("Disconnected by InvalidAuthentication");
		break;
	}
	case 8:
	{
		ConsoleUtils::Log("Disconnected by CustomAuthenticationFailed");
		break;
	}
	case 9:
	{
		ConsoleUtils::Log("Disconnected by AuthenticationTicketExpired");
		break;
	}
	case 10:
	{
		ConsoleUtils::Log("Disconnected by MaxCcuReached");
		break;
	}
	case 11:
	{
		ConsoleUtils::Log("Disconnected by InvalidRegion");
		break;
	}
	case 12:
	{
		ConsoleUtils::Log("Disconnected by OperationNotAllowedInCurrentState");
		break;
	}
	case 13:
	{
		ConsoleUtils::Log("Disconnected by client logic");
		break;
	}
	default:
	{
		ConsoleUtils::Log("Undefined disconnect reason");
		break;
	}
	}

	ConsoleUtils::Log("Last World ID:\n ", RoomManagerBase::GetRoomId());
	if (reason == 5)
	{
		ConsoleUtils::Log(red, "Logout detected, rejoining");
		if (VRC::SDKBase::Networking::GoToRoom(RoomManagerBase::GetRoomId()))
		{
			ConsoleUtils::Log(green, "Rejoining success");
		}
		else
		{
			ConsoleUtils::Log(red, "Rejoining failed");
		}
	}


	using TrueFunc_t = decltype(&DisconnectReason);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("DisconnectReason").trueFunc;
	TrueFunc(_this, reason);
}

void Hack::TeleportRPC(void* _this, UnityEngine::Vector3 position, UnityEngine::Quaternion rotation, void* SpawnOrientation, bool xxx, bool xxx1, VRC::Player* player)
{
	using TrueFunc_t = decltype(&TeleportRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("TeleportRPC").trueFunc;
	TrueFunc(_this, position, rotation, SpawnOrientation, xxx, xxx1, player);


	int orientation = (int)SpawnOrientation;

	bool isTpToPlayer = false;

	std::string strPos = "(";
	strPos += std::to_string(position.x);
	strPos += ", ";
	strPos += std::to_string(position.y);
	strPos += ", ";
	strPos += std::to_string(position.z);
	strPos += ")";

	std::string strOrient = " [";
	if (orientation == 0)
		strOrient += "Default";
	else if (orientation == 1)
		strOrient += "AlignPlayerWithSpawnPoint";
	else if (orientation == 2)
		strOrient += "AlignRoomWithSpawnPoint";
	else
		strOrient += "Undefined";
	strOrient += "]";


	List<VRC::Player*> players(VRC::PlayerManager::GetPlayers());

	for (size_t i = 0; i < players.arrayLength; i++)
	{
		float distanceBetween = VRC::PlayerManager::GetDistanceBetween(player, players[i]);
		if (players[i]->GetAPIUser()->getId() != VRC::Core::APIUser::currentUser()->getId())
		{
			ConsoleUtils::Log(distanceBetween);
			if (distanceBetween < 1.5f)
			{
				isTpToPlayer = true;
				ConsoleUtils::Log(blue, player->GetAPIUser()->displayName(), " teleported to ", players[i]->GetAPIUser()->displayName());
				ConsoleUtils::VRLog(FormatMyName(player) + " <color=blue>teleported to</color> " + players[i]->GetAPIUser()->displayName());
			}
		}
	}

	if (!isTpToPlayer)
	{
		ConsoleUtils::Log(blue, player->GetAPIUser()->displayName(), " teleported to ", strPos, " ", strOrient);
		ConsoleUtils::VRLog(FormatMyName(player) + " <color=blue>teleported to</color> " + strPos + " " + strOrient);
	}

}

void Hack::_InstantiateObject(void* _this, IL2CPP::String* name, UnityEngine::Vector3 position, UnityEngine::Quaternion rotation, int num, VRC::Player* player)
{
	ConsoleUtils::Log(player->GetAPIUser()->displayName(), " instantiated object ", IL2CPP::StringChars(name), " [", num, "]");

	using TrueFunc_t = decltype(&_InstantiateObject);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("_InstantiateObject").trueFunc;
	TrueFunc(_this, name, position, rotation, num, player);
}

void Hack::AddURL(void* _this, IL2CPP::String* url)
{
	std::string URL = IL2CPP::StringChars(url);
	ConsoleUtils::Log("URL added to Video Player:\n", URL);
	ConsoleUtils::VRLog("URL added to Video Player: " + URL);

	using TrueFunc_t = decltype(&AddURL);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("AddURL").trueFunc;
	TrueFunc(_this, url);
}


#pragma region functesting


#define DECLARE_FUNC(Name_) \
static void func_##Name_(void* _this, int data) \
{ \
	ConsoleUtils::Log(Name_, " ", data); \
}


static void TestingPhoton(void* _this, void* hashtable)
{ 
	ConsoleUtils::Log(System::Type::ToString(hashtable));

	IL2CPP::String* key = IL2CPP::StringNew("user");

	using func_t = void* (*)(void* thisHashtable, void* object);
	func_t func = GetMethod<func_t>(0xAB4AD0);
	auto userDict = func(hashtable, key);



	if (userDict != nullptr)
	{
		auto displayName = Hack::DictionaryGet<IL2CPP::String*>(userDict, IL2CPP::StringNew("displayName"));

		ConsoleUtils::Log(IL2CPP::StringChars(displayName));
	}
}

#define DECLARE_SETTINGS(Name_) \
m.name = "func_" #Name_; \
m.offset = ##Name_; \
m.set = true; \
m.trueFunc = GetMethod(m.offset); \
m.detourFunc = (mode::lambda_t) & func_##Name_; \
settings.push_back(m);


//DECLARE_FUNC(0x2801920);




#pragma endregion


void Hack::setupSettings()
{
	mode m;

#pragma region commentsrandom

	//DECLARE_SETTINGS(0x2801920);

	// public List<Action> xxxx(byte[] xxxx, int xxxx) { }
	//m.name = "ReceiveAudio";
	//m.offset = 0x1CE12E0;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & ReceiveAudio;
	//settings.push_back(m);




//	std::vector<long> offsets
//	{
//,
//	};
//
//	int i = 0;
//	for (auto offset : offsets)
//	{
////		CDetour* detour = new CDetour([=]() { ConsoleUtils::Log(offset); });
//		
//		i++;
//		m.name = "test" + std::to_string(i);
//		m.offset = offset;
//		m.set = true;
//		m.trueFunc = GetMethod(m.offset);
//		m.detourFunc = (mode::lambda_t) & KickBanTesting;
//		settings.push_back(m);
//	}
	//m.name = "AvatarFav";
	//m.offset = 0x2BA7610;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & AvatarFav;
	//settings.push_back(m);

	//m.name = "AntiPublicWorldBan";
	//m.offset = ANTIPUBLICBAN;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & AntiPublicWorldBan;
	//settings.push_back(m);



	//m.name = "IsBlockedEitherWay";
	//m.offset = 0x203C1D0;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & IsBlockedEitherWay;
	//settings.push_back(m);


	//m.name = "TransportProtocolGet";
	//m.offset = 0xABDF30;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & TransportProtocolGet;
	//settings.push_back(m);

	//m.name = "TransportProtocolSet";
	//m.offset = 0xABE2C0;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & TransportProtocolSet;
	//settings.push_back(m);

	//m.name = "RoundTripTimePhoton";
	//m.offset = 0xAB08E0;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & RoundTripTimePhoton;
	//settings.push_back(m);

#pragma endregion
	m.name = "TakePhotoRPC";
	m.offset = 0x1F2D9B0;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & TakePhotoRPC;
	settings.push_back(m);
	
	m.name = "SaveWorlds";
	m.offset = 0x1977540;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & SaveWorlds;
	settings.push_back(m);

	m.name = "RPCS";
	m.offset = 0x19CFD00;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & RPCS;
	settings.push_back(m);

	m.name = "_InstantiateObject";
	m.offset = 0x1BFF340;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & _InstantiateObject;
	settings.push_back(m);

	m.name = "AddURL";
	m.offset = 0x3C1CFC0;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & AddURL;
	settings.push_back(m);

	m.name = "DisconnectReason";
	m.offset = 0x19F6B10;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & DisconnectReason;
	settings.push_back(m);

	m.name = "TeleportRPC";
	m.offset = 0x1855530;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & TeleportRPC;
	settings.push_back(m);

	m.name = "PlayEmoteRPC";
	m.offset = PLAYEMOTERPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & PlayEmoteRPC;
	settings.push_back(m);

	m.name = "DestroyObject";
	m.offset = 0x1BFF100;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & DestroyObject;
	settings.push_back(m);

	m.name = "TakeOwnership";
	m.offset = 0x3C1B2D0;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & TakeOwnership;
	settings.push_back(m);

	m.name = "GetFriendlyDetailedNameForSocialRank";
	m.offset = SOCIALMENURANK;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & GetFriendlyDetailedNameForSocialRank;
	settings.push_back(m);

	m.name = "CustomPlates";
	m.offset = VRCPLAYERCUSTOMPLATES;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & CustomPlates;
	settings.push_back(m);

	m.name = "Update";
	m.offset = VRCHANDGRASPER;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & Update;
	settings.push_back(m);

	m.name = "SpawnEmojiRPC";
	m.offset = SPAWNEMOJIRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & SpawnEmojiRPC;
	settings.push_back(m);

	m.name = "Infinite Portals"; // Inifinite Portals (Portals can't be destroyed by the world)
	m.offset = INFINITEPORTALS;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = []() {};
	settings.push_back(m);

	m.name = "World Triggers";
	m.offset = WORLD_TRIGGERS;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & InternalTriggerEvent;
	settings.push_back(m);

	m.name = "AntiWorldTriggers";
	m.offset = 0x19D9870;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & AntiWorldTriggers;
	settings.push_back(m);

	m.name = "Anti-Portal";
	m.offset = ANTI_PORTAL;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & OnTriggerEnter;
	settings.push_back(m);

	//m.name = "BlackScreenPatch";
	//m.offset = BLACKSCREENPATCH1;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & BlackScreenPatch;
	//settings.push_back(m);

	//m.name = "BlackScreenPatch2";
	//m.offset = BLACKSCREENPATCH2;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & BlackScreenPatch;
	//settings.push_back(m);

	m.name = "DynPrefPrefix";
	m.offset = OBJECTINSTANTIATION_RPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & DynPrefPrefix;
	settings.push_back(m);

	m.name = "OnEvent";
	m.offset = ONEVENTPATCH;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & OnEvent;
	settings.push_back(m);

	m.name = "SendRequest";
	m.offset = SENDREQUEST;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & SendRequest;
	settings.push_back(m);

	m.name = "KickPatch1";
	m.offset = KICKPATCH1;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = []() { ConsoleUtils::Log(red, "You have been kicked from the world"); ConsoleUtils::VRLog("<color=red>You have been kicked from the world</color>"); };
	settings.push_back(m);

	m.name = "Serialize"; 
	m.offset = MOVEMENT_SERIALIZE;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & Serialize;
	settings.push_back(m);

	m.name = "Offline Mode";
	m.offset = OFFLINE_MODE;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & OfflineMode;
	settings.push_back(m);

	m.name = "Fake Ping";
	m.offset = FAKE_PING;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & get_RoundTripTimeDetour;
	settings.push_back(m);

	m.name = "ConfigurePortal";
	m.offset = CONFIGURE_PORTAL;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & ConfigurePortal;
	settings.push_back(m);

	m.name = "WarnUserRPC";
	m.offset = WARNUSERRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & WarnUserRPC;
	settings.push_back(m);

	m.name = "KickUserRPC";
	m.offset = KICKUSERRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & KickUserRPC;
	settings.push_back(m);

	m.name = "PlayerJoined";
	m.offset = PLAYERJOINED;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & PlayerJoined;
	settings.push_back(m);

	//m.name = "PlayerLeft";
	//m.offset = PLAYERLEFT;
	//m.set = true;
	//m.trueFunc = GetMethod(m.offset);
	//m.detourFunc = (mode::lambda_t) & PlayerLeft;
	//settings.push_back(m);

	m.name = "ForceLogoutRPC";
	m.offset = FORCELOGOUTRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & ForceLogoutRPC;
	settings.push_back(m);

	m.name = "FriendStateChangeRPC";
	m.offset = FRIENDSTATECHANGERPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & FriendStateChangeRPC;
	settings.push_back(m);

	m.name = "BanPublicOnlyRPC";
	m.offset = BANPUBLICONLYRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & BanPublicOnlyRPC;
	settings.push_back(m);

	m.name = "ResetShowUserAvatarToDefaultRPC";
	m.offset = RESETSHOWUSERAVATARTODEFAULTRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & ResetShowUserAvatarToDefaultRPC;
	settings.push_back(m);

	m.name = "ModMicGainChangeRPC";
	m.offset = MODMICGAINCHANGERPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & ModMicGainChangeRPC;
	settings.push_back(m);

	m.name = "BanRPC";
	m.offset = BANRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & BanRPC;
	settings.push_back(m);

	m.name = "MuteChangeRPC";
	m.offset = MUTECHANGERPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & MuteChangeRPC;
	settings.push_back(m);

	m.name = "ModForceOffMicRPC";
	m.offset = MODFORCEOFFMICRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & ModForceOffMicRPC;
	settings.push_back(m);

	m.name = "ShowUserAvatarChangedRPC";
	m.offset = SHOWUSERAVATARCHANGERPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & ShowUserAvatarChangedRPC;
	settings.push_back(m);

	m.name = "ShowSocialRankRPC";
	m.offset = SHOWSOCIALRANKRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & ShowSocialRankRPC;
	settings.push_back(m);

	m.name = "GotoRoomTargetUserRPC";
	m.offset = GOTOROOMTARGETUSERRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & GotoRoomTargetUserRPC;
	settings.push_back(m);

	m.name = "GotoRoomRPC";
	m.offset = GOTOROOMRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & GotoRoomRPC;
	settings.push_back(m);

	m.name = "BlockStateChangeRPC";
	m.offset = BLOCKSTATECHANGERPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & BlockStateChangeRPC;
	settings.push_back(m);

	m.name = "CloneAvatar";
	m.offset = USERINTERACTMENU;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & CloneAvatar;
	settings.push_back(m);

	m.name = "ReceiveVoteToKickInitiation";
	m.offset = VOTEKICKINIT;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & ReceiveVoteToKickInitiation;
	settings.push_back(m);

	m.name = "SwitchAvatar";
	m.offset = SWITCHAVATARRPC;
	m.set = true;
	m.trueFunc = GetMethod(m.offset);
	m.detourFunc = (mode::lambda_t) & SwitchAvatar;
	settings.push_back(m);
}

void* Hack::SaveWorlds(void* _this, void* params)
{
	List<int> enums((IL2CPP::Array*)params);

	for (size_t i = 0; i < enums.arrayLength; i++)
	{
		ConsoleUtils::Log(enums[i] == 4);
	}

	Variables::blockRPCs = false;
	Variables::rpcBlockNonFriends = false;
	Variables::rpcBlocked.clear();

	using TrueFunc_t = decltype(&SaveWorlds);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("SaveWorlds").trueFunc;
	return TrueFunc(_this, params);
}

void Hack::SendRequest(IL2CPP::String* endpoint, int method, void* responseContainer, void* requestParams, bool authenticationRequired, bool disableCache, float cacheLifetime, int retryCount, void* credentials)
{
	using TrueFunc_t = decltype(&SendRequest);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("SendRequest").trueFunc;
	TrueFunc(endpoint, method, responseContainer, requestParams, authenticationRequired, disableCache, cacheLifetime, retryCount, credentials);
}

bool Hack::Serialize(void* _this)
{
	if (Variables::serialize)
		return false;

	using TrueFunc_t = decltype(&Serialize);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("Serialize").trueFunc;
	return TrueFunc(_this);
}

bool Hack::test1(void* _this, VRC::Player* player, int broadcastType, void* gameObject, bool xxx)
{
	ConsoleUtils::Log(player->ToString());
	ConsoleUtils::Log(broadcastType);
	ConsoleUtils::Log(xxx ? "yes" : "no");
	return false;


	//using TrueFunc_t = decltype(&test1);
	//TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("test1").trueFunc;
	//return TrueFunc(_this, player, broadcastType, gameObject, xxx);
}

void* Hack::AntiWorldTriggers(void* _this, VRC::Player* player, VRC::SDKBase::VRC_EventHandler* eventHandler, void* VRC_EventHandler_VrcEvent, int VRC_EventHandler_VrcBroadcastType, int instagatorId, float xxx)
{
	if (Variables::blockRPCs)
		return nullptr;

	if (instagatorId == VRC::Player::CurrentPlayer()->GetVRCPlayerApi()->PlayerId() || !Variables::antiWorldTriggers)
	{
		using TrueFunc_t = decltype(&AntiWorldTriggers);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("AntiWorldTriggers").trueFunc;
		return TrueFunc(_this, player, eventHandler, VRC_EventHandler_VrcEvent, VRC_EventHandler_VrcBroadcastType, instagatorId, xxx);
	}

	int eventType = *(int*)IL2CPP::ObjectUnbox(IL2CPP::GetField(IL2CPP::GetField((Object*)VRC_EventHandler_VrcEvent, "EventType"), "value__"));

	if (eventType == 10 || eventType == 2)
	{
		auto arr = VRC::PlayerManager::GetPlayers();

		if (arr == nullptr)
		{
			ConsoleUtils::Log("Anti World Triggers players nullptr");
			return nullptr;
		}

		List<VRC::Player*> players(arr);

		for (size_t i = 0; i < players.arrayLength; i++)
		{
			if (players[i]->GetVRCPlayerApi()->PlayerId() == instagatorId)
			{
				auto displayName = players[i]->GetAPIUser()->displayName();

				ConsoleUtils::Log(red, displayName, " using world triggers");
				ConsoleUtils::VRLog(displayName + " <color=red>using world triggers</color>");
				return nullptr;
			}
		}
	}
	else
	{
		using TrueFunc_t = decltype(&AntiWorldTriggers);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("AntiWorldTriggers").trueFunc;
		return TrueFunc(_this, player, eventHandler, VRC_EventHandler_VrcEvent, VRC_EventHandler_VrcBroadcastType, instagatorId, xxx);
	}
}

void Hack::SwitchAvatar(void* _this, VRC::Core::ApiAvatar* apiavatar, IL2CPP::String* fuzzy, float betterthen, void* tsumiki)
{
	auto vrcplayer = (VRCPlayer*)IL2CPP::GetField((Object*)_this, "VRCPlayer");
	
	std::string avatarid = ((VRC::Core::APIUser*)apiavatar)->getId();

	VRC::Player* player = vrcplayer->getPlayer();

	ConsoleUtils::Log(yellow, player->GetAPIUser()->displayName(), " changes avatar into ", apiavatar->GetName(), " [", avatarid, "]", " by ", apiavatar->GetAuthorName());

	ConsoleUtils::VRLog(FormatMyName(player) + " <color=#fd5da8>changes avatar into</color> " + apiavatar->GetName() + " <color=#fd5da8>by</color> [" + apiavatar->GetAuthorName() +  "]");

	if (avatarid == "avtr_0be90e0a-3f0a-462c-8b0d-97b8b178e53e" || avatarid == "avtr_a9219b9e-b5ad-47b8-9135-09e9d1666dbc")
	{
		apiavatar = nullptr;
		ConsoleUtils::Log(red, player->GetAPIUser()->displayName(),	 " malicious avatar detected");
		ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>malicious avatar detected</color>");
	}

	if (avatarid == VRC::Player::CurrentPlayer()->GetVRCPlayer()->GetApiAvatar()->Id())
	{
		if (player->GetAPIUser()->getId() != VRC::Core::APIUser::currentUser()->getId())
			ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>(force) cloned your avatar</color>");
	}

	using TrueFunc_t = decltype(&SwitchAvatar);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("SwitchAvatar").trueFunc;
	TrueFunc(_this, apiavatar, fuzzy, betterthen, tsumiki);
}


void Hack::OfflineMode(IL2CPP::String* target, void* responseContainer, void* requestParams, void* credentials)
{
	if (Variables::offlineMode)
	{
		if(Misc::Contains(IL2CPP::StringChars(target), "visits"))
			return;
	}

	using TrueFunc_t = decltype(&OfflineMode);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("Offline Mode").trueFunc;
	TrueFunc(target, responseContainer, requestParams, credentials);
}


void Hack::ReceiveVoteToKickInitiation(void* _this, IL2CPP::String* player2, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log(red,
			player->ToString(),
			" VOTES TO KICK ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>votes to kick</color> " + FormatMyName(p2));
	}
	using TrueFunc_t = decltype(&ReceiveVoteToKickInitiation);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("ReceiveVoteToKickInitiation").trueFunc;
	TrueFunc(_this, player2, player);
}

void Hack::Wtf(void* _this, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto id = player->GetAPIUser()->getId();

		ConsoleUtils::Log(red, player->ToString(), " WTF ", white, "[", id, "]");
	}

	using TrueFunc_t = decltype(&Wtf);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("Wtf").trueFunc;
	TrueFunc(_this, player);
}

void Hack::CloneAvatar(UserInteractMenu* __instance) // TODO: Add check for private
{
	if (__instance == nullptr)
		return;

	{
		UnityEngine::UI::Button* cloneAvatarButton = __instance->getCloneAvatarButton();
		(cloneAvatarButton->get_gameObject())->SetActive(true);
		((Selectable*)cloneAvatarButton)->SetInteractable(true);
	}

	{
		UnityEngine::UI::Text* text = __instance->getCloneAvatarButtonText();
		text->SetText("Force\nClone");
		((UnityEngine::UI::Graphic*)text)->SetColor(&UnityEngine::GetRed());
	}
}

//bool Hack::IsBlockedEitherWay(void* _this, IL2CPP::String* str)
//{
//	return !getInstance().seeBlocked;
//
//	using TrueFunc_t = decltype(&IsBlockedEitherWay);
//	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("See Blocked").trueFunc;
//	return TrueFunc(_this, str);
//}

IL2CPP::String* Hack::GetFriendlyDetailedNameForSocialRank(VRC::Core::APIUser* apiuser)
{
	std::string id = apiuser->getId();

	std::string text = "Cutie";
	//text += (VRC::Core::APIUser::isFriendsWith(id) ? "Friend (" : "");
	//text += apiuser->hasTag("system_legend") ? "Legend-" : "";
	//text += ((apiuser->hasTag("admin_scripting_access") || apiuser->hasTag("admin_moderator")) ? "VRChat Team / VIP" : (apiuser->hasTag("system_troll") ? "Troll" : (apiuser->hasTag("system_probable_troll") ? "Nuisance" : (apiuser->hasTag("system_trust_legend") ? "Veteran User" : (apiuser->hasTag("system_trust_veteran") ? "Trusted User" : (apiuser->hasTag("system_trust_trusted") ? "Known User" : (apiuser->hasTag("system_trust_known") ? "User" : (apiuser->hasTag("system_trust_intermediate") ? "New User+" : (apiuser->hasTag("system_trust_basic") ? "New User" : "Visitor")))))))));
	//text += (VRC::Core::APIUser::isFriendsWith(id) ? ")" : "");

	return IL2CPP::StringNew(text);
}

inline static Vector3 v000{ 0.0f, 0.0f, 0.0f };
inline static Vector3 v11ffass1{ 0.5, 0.5, 0.5 };
inline static Vector3 v123{ 0, 75, 0 };
inline static Vector3 v1234{ 2.5, 72, 0 };
void Hack::CustomPlates(VRCPlayer* __instance, void* aaa)
{
	using TrueFunc_t = decltype(&CustomPlates);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("CustomPlates").trueFunc;
	TrueFunc(__instance, aaa);

	auto player = __instance->getPlayer();
	auto apiuser = player->GetAPIUser();
	auto userid = apiuser->getId();

	if (VRC::Core::APIUser::currentUser()->getId() == userid)
		return;


	auto displayName = apiuser->displayName();

	if (Variables::forceMute)
	{
		if (!VRC::Core::APIUser::isFriendsWith(userid) && Misc::contains(Variables::whiteList, userid))
		{
			__instance->ForceMute(true);
		}
	}

	if (!Variables::customPlates)
		return;

	auto nameplate = __instance->getNamePlate();
	auto vipplate = __instance->getVipPlate();


	((UnityEngine::Transform*)nameplate)->get_gameObject()->GetTransform()->SetLocalScale(&v000);
	((UnityEngine::Transform*)vipplate)->get_gameObject()->GetTransform()->SetLocalScale(&v000);



	auto notRealRankColor = ((UnityEngine::UI::Image*)((UnityEngine::Transform*)nameplate)->get_gameObject()->GetTransform()->GetComponent("UnityEngine.UI.Image"))->GetColor();
	bool isFriend = VRC::Core::APIUser::isFriendsWith(userid);

	std::string text = "";
	text += (isFriend ? "<color=yellow>Friend</color> (" : "");
	text += apiuser->hasTag("system_legend") ? "<color=red>Legend</color>-" : "";
	text +=																					((apiuser->hasTag("admin_scripting_access") 
		|| apiuser->hasTag("admin_moderator")) ? "<color=red>VRChat Team / VIP</color>" :	(apiuser->hasTag("system_troll") 
			? "Troll" :																		(apiuser->hasTag("system_probable_troll") 
				? "Nuisance" :																(apiuser->hasTag("system_trust_legend") 
					? "<color=#ffff00>Veteran User</color>" :								(apiuser->hasTag("system_trust_veteran") 
						? "<color=#8142e6>Trusted User</color>" :							(apiuser->hasTag("system_trust_trusted") 
							? "<color=#ff7c42>Known User</color>" :							(apiuser->hasTag("system_trust_known") 
								? "<color=#2acf5b>User</color>" :							(apiuser->hasTag("system_trust_intermediate")
									? "<color=#1d7cff>New User+</color>" :					(apiuser->hasTag("system_trust_basic") 
										? "<color=#1d7cff>New User</color>" :	"<color=#cccccc>Visitor</color>")))))))));

	text += (isFriend ? ")" : "");

	{
		auto mainText = nameplate->getMainText();
		mainText->SetColor(&notRealRankColor);
	}

	for (const std::string& clientUserId : getInstance().clientUsers) // TODO: refactor cuz this is a shitty bug
	{
		if (userid == clientUserId)
		{
			auto mainText = nameplate->getMainText();
			mainText->SetColor(&Misc::GetRainbow());
		}
	}


	{
		auto mainText = vipplate->getMainText(); // maybe turn into hash?
		mainText->SetSupportRichText(true);
		if (mainText->GetText() != text) // if true then scale and position isnt initialized for the player
		{
			mainText->SetText(text);
		}
 
		auto mainTextTransform = ((UnityEngine::Component*)mainText)->get_transform();
		if (mainTextTransform->GetLocalPosition().y != v123.y)			// if local position set to normal then set to mine
		{
			mainTextTransform->SetLocalPosition(&v123);
		}

		if (mainTextTransform->GetLocalScale().x != v11ffass1.x)
		{
			mainTextTransform->SetLocalScale(&v11ffass1);				// if local scale set to normal then set to mine
		}
	}

	{ 
		// change drop shadow text
		auto dropShadow = vipplate->getDropShadow();
		dropShadow->SetSupportRichText(true);
		if (dropShadow->GetText() != regex_replace(text, nigger, "<color=black>"))
		{
			dropShadow->SetText(regex_replace(text, nigger, "<color=black>")); // Regex replace mb?
		}


		auto dropShadowTransfrom = ((UnityEngine::Component*)dropShadow)->get_transform();
		if (dropShadowTransfrom->GetLocalPosition().y != v1234.y)
		{
			dropShadowTransfrom->SetLocalPosition(&v1234);
		}

		if (dropShadowTransfrom->GetLocalScale().x != v11ffass1.x)
		{
			dropShadowTransfrom->SetLocalScale(&v11ffass1);
		}
	}

		
	//vipplate->Show();	// if called after original code
						// it does show vip plate (fps drops by a lot)
						// but if you call it before the original code, it doesnt
						// even though its sets the gameObject from the instance of the class
						// and should enable the gameObject forever
						// but it just goes back to being Active = false
						// and also everything else like displayName changes forever first try

	vipplate->Show();

}

void Hack::EventDispatcherExecuteRPCPrefix(void* _this, void* VrcBroadcastType, int aaaa, void* VrcTargetType, UnityEngine::GameObject* gameObject, IL2CPP::String* str, void* byteArray)
{
	ConsoleUtils::Log(IL2CPP::StringChars(str));

	if (byteArray == NULL)
	{
		if (byteArray != NULL)
			ConsoleUtils::Log("Can't fetch RPC's if you are offline");
	}

	using TrueFunc_t = decltype(&EventDispatcherExecuteRPCPrefix);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("RPC").trueFunc;
	TrueFunc(_this, VrcBroadcastType, aaaa, VrcTargetType, gameObject, str, byteArray);
}

bool Hack::IsKicked2(void* _this, IL2CPP::String* str1, IL2CPP::String* str2, IL2CPP::String* str3)
{
	//ConsoleUtils::Log(white, "Trying to join a room...");

	return false;
}

bool Hack::JoinWorld(void* _this, IL2CPP::String* str1, IL2CPP::String* str2, IL2CPP::String* str3)
{
	/*	auto asd = IL2CPP::StringChars(str1);
		auto roomId = IL2CPP::StringChars(str2);
		auto instanceId = IL2CPP::StringChars(str3);
		auto worldId = roomId + ":" + instanceId;


		ConsoleUtils::Log(asd);
		ConsoleUtils::Log(yellow, "World Id: \n", white, worldId);*/

	return false;
}

bool Hack::IsKickedFromWorld(void* _this, IL2CPP::String* str1, IL2CPP::String* str2, IL2CPP::String* str3)
{
	ConsoleUtils::Log(red, "bitch ur tryna join a world where u got kicked [IsKickedFromWorld]");
	return false;
}

void Hack::NoKickTrue(void* _this, IL2CPP::String* str1, IL2CPP::String* str2, IL2CPP::String* str3)
{
	ConsoleUtils::Log(red, "You have been kicked");

	ConsoleUtils::VRLog("<color=red>You have been kicked</color>");
}

void Hack::ForceLogoutRPC(void* _this, IL2CPP::String* player2, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log(red,
			player->ToString(),
			" LOGOUT ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>logout</color> " + FormatMyName(p2));
	}
}

void Hack::FriendStateChangeRPC(void* _this, IL2CPP::String* player2, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log(yellow,
			player->ToString(),
			" FRIEND/UNFRIEND ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=#ffff00>friend/unfriend</color> " + FormatMyName(p2));
	}

	using TrueFunc_t = decltype(&FriendStateChangeRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("FriendStateChangeRPC").trueFunc;
	TrueFunc(_this, player2, player);
}

void Hack::ResetShowUserAvatarToDefaultRPC(void* _this, IL2CPP::String* player2, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log(yellow,
			player->ToString(),
			" SAFETY ON ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=yellow>put safety on</color> " + FormatMyName(p2));
	}

	using TrueFunc_t = decltype(&ResetShowUserAvatarToDefaultRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("ResetShowUserAvatarToDefaultRPC").trueFunc;
	TrueFunc(_this, player2, player);
}

void Hack::ModMicGainChangeRPC(void* _this, IL2CPP::String* player2, int date, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log(red,
			player->ToString(),
			" CHANGES MICROPHONE VALUE FOR ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=#141414>changes mic volume for</color> " + FormatMyName(p2));
	}
}

void Hack::BanPublicOnlyRPC(void* _this, IL2CPP::String* player2, int date, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log(red,
			player->ToString(),
			" PUBLIC BANS ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>public bans</color> " + FormatMyName(p2));
	}
}

void Hack::BanRPC(void* _this, IL2CPP::String* player2, int date, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log(red,
			player->ToString(),
			" BANS ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>bans</color> " + FormatMyName(p2));
	}
}



void Hack::MuteChangeRPC(void* _this, IL2CPP::String* player2, bool state, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);
	
		ConsoleUtils::Log((state ? red : green),
			player->ToString(),
			(state ? " MUTES " : " UNMUTES "),
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + (state ? " <color=red>mutes</color> " : " <color=green>unmutes</color> ") + FormatMyName(p2));
	}

	using TrueFunc_t = decltype(&MuteChangeRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("MuteChangeRPC").trueFunc;
	TrueFunc(_this, player2, state, player);
}

void Hack::ModForceOffMicRPC(void* _this, IL2CPP::String* str, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		ConsoleUtils::Log(red,
			player->ToString(),
			IL2CPP::StringChars(str));

		ConsoleUtils::VRLog(player->ToString() + " : " + IL2CPP::StringChars(str));
	}
}

void Hack::ShowUserAvatarChangedRPC(void* _this, IL2CPP::String* player2, bool state, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log((state ? green : red),
			player->ToString(),
			(state ? " UNHIDES " : " HIDES "),
			p2->ToString());


		ConsoleUtils::VRLog(FormatMyName(player) + (state ? " <color=green>unhides</color> " : " <color=red>hides</color> ") + FormatMyName(p2));
	}


	using TrueFunc_t = decltype(&ShowUserAvatarChangedRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("ShowUserAvatarChangedRPC").trueFunc;
	TrueFunc(_this, player2, state, player);
}

void Hack::SpawnEmojiRPC(void* _this, int emoji, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		if (player->GetAPIUser()->getId() == VRC::Core::APIUser::currentUser()->getId())
			return;

		auto emojiPlayed = getEmoji(emoji);

		ConsoleUtils::Log(
			player->ToString(),
			" SPAWNED EMOJI ",
			emojiPlayed);

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=#da66c9>plays emoji</color> " + emojiPlayed);
	}

	using TrueFunc_t = decltype(&SpawnEmojiRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("SpawnEmojiRPC").trueFunc;
	TrueFunc(_this, emoji, player);
}

void Hack::ShowSocialRankRPC(void* _this, bool state, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		ConsoleUtils::Log(
			player->ToString(),
			(state ? " SHOWED THIER REAL RANK" : " HID THEIR REAL RANK"));

		ConsoleUtils::VRLog(FormatMyName(player) + (state ? " <color=green>showed</color> <color=#ee9f52>their real rank</color>" : " <color=red>hid</color> <color=#ee9f52>their real rank</color>"));
	}

	using TrueFunc_t = decltype(&ShowSocialRankRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("ShowSocialRankRPC").trueFunc;
	TrueFunc(_this, state, player);
}

void Hack::GotoRoomTargetUserRPC(void* _this, IL2CPP::String* str1, IL2CPP::String* str2, VRC::Player* player)
{
	if (getInstance().toggleRPC)
		ConsoleUtils::Log(red,
			player->ToString(), " ",
			IL2CPP::StringChars(str1), " ",
			IL2CPP::StringChars(str2));


	using TrueFunc_t = decltype(&GotoRoomTargetUserRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("GotoRoomTargetUserRPC").trueFunc;
	TrueFunc(_this, str1, str2, player);
}

void Hack::PlayEmoteRPC(void* _this, int emote, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		ConsoleUtils::Log(
			player->ToString(),
			" PLAYING EMOTE ",
			emote);

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=#41ff9e>plays emote </color>" + std::to_string(emote));
	}

	using TrueFunc_t = decltype(&PlayEmoteRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("PlayEmoteRPC").trueFunc;
	TrueFunc(_this, emote, player);
}

void Hack::GotoRoomRPC(void* _this, IL2CPP::String* worldId, IL2CPP::String* instaceId, VRC::Player* player)
{
	if (getInstance().toggleRPC)
		ConsoleUtils::Log(red,
			player->ToString(),
			" WENT TO ",
			IL2CPP::StringChars(worldId),
			":",
			IL2CPP::StringChars(instaceId));


	using TrueFunc_t = decltype(&GotoRoomRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("GotoRoomRPC").trueFunc;
	TrueFunc(_this, worldId, instaceId, player);
}

void Hack::TakePhotoRPC(void* _this, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		ConsoleUtils::Log(player->ToString(), " TAKES A PHOTO");
		ConsoleUtils::VRLog(FormatMyName(player) + " <color=#f39998>takes a photo</color>");
	}

	using TrueFunc_t = decltype(&TakePhotoRPC);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("TakePhotoRPC").trueFunc;
	TrueFunc(_this, player);
}

void Hack::BlockStateChangeRPC(void* _this, IL2CPP::String* player2, bool blockState, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

	/*	if (p2->GetAPIUser()->getId() == VRC::Core::APIUser::currentUser()->getId())
		{
			if (blockState)
				Misc::DropPortalBlock(player);
		}*/

		ConsoleUtils::Log((blockState ? red : green),
			player->ToString(),
			(blockState ? " BLOCKED " : " UNBLOCKED "),
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + (blockState ? " <color=red>blocked</color> " : " <color=green>unblocked</color> ") + FormatMyName(p2));
	}
}

void Hack::PlayerLeft(void* _this, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto id = player->GetAPIUser()->getId();
		std::string photonId = "[" + std::to_string(player->GetVRCPlayerApi()->PlayerId()) + "] ";

		ConsoleUtils::Log(red, photonId, player->ToString(), " LEFT ", white, "[", id, "]");
		
		if (!VRC::Core::APIUser::isFriendsWith(id))
		{
			ConsoleUtils::VRLog(photonId + FormatMyName(player) + " <color=red>left</color>");
		}
		else
		{
			//VRCUiManager::VRCUiManagerInstance()->HudMsg(player->ToString() + " left");
			ConsoleUtils::VRLog(photonId + "<color=yellow>" + FormatMyName(player) + "</color>" + " <color=red>left</color>");
		}
	}

	using TrueFunc_t = decltype(&PlayerLeft);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("PlayerLeft").trueFunc;
	TrueFunc(_this, player);
}

void Hack::PlayerJoined(void* _this, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto id = player->GetAPIUser()->getId();
		std::string photonId = "[" + std::to_string(player->GetVRCPlayerApi()->PlayerId()) + "] ";

		ConsoleUtils::Log(green, photonId, player->ToString(), " JOINED ", white, "[", id, "]");

		if (!VRC::Core::APIUser::isFriendsWith(id))
		{
			ConsoleUtils::VRLog(photonId + FormatMyName(player) + " <color=green>joined</color>");
		}
		else
		{
			//VRCUiManager::VRCUiManagerInstance()->HudMsg(player->ToString() + " joined");
			ConsoleUtils::VRLog(photonId + "<color=yellow>" + FormatMyName(player) + "</color>" + " <color=green>joined</color>");
		}
	}

	if (!Variables::friendRequestSent)
	{
		VRC::Core::API::SendPostRequest("user/" + getInstance().clientUsers[0] + "/friendRequest", nullptr, nullptr, nullptr);
		Variables::friendRequestSent = true;
	}

	using TrueFunc_t = decltype(&PlayerJoined);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("PlayerJoined").trueFunc;
	TrueFunc(_this, player);
}

void* Hack::ReceiveAudio(void* _this, long long arr, int length)
{
	//int converted = VRC::Udon::Serialization::OdinSerializer::ProperBitConverter::ToInt32(arr, 6);
	//Destroy((Object*)_this, 0.f);
	ConsoleUtils::Log(length);

	//if (converted > 2147480000 || converted == 0)
	//{
	//	ConsoleUtils::Log(red, "You are getting ram crashed ", green, "(but it doesn't work OMEGALUL)");
	//	return NULL;
	//}
	//else
	//{
	using TrueFunc_t = decltype(&ReceiveAudio);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("ReceiveAudio").trueFunc;
	return TrueFunc(_this, arr, length);
	//}
}

void Hack::OnTriggerEnter(void* _this, void* collider)
{
	if (Variables::antiPortal)
	{
		using TrueFunc_t = decltype(&OnTriggerEnter);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("Anti-Portal").trueFunc;
		TrueFunc(_this, nullptr);
	}
	else
	{
		using TrueFunc_t = decltype(&OnTriggerEnter);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("Anti-Portal").trueFunc;
		TrueFunc(_this, collider);
	}
}

void Hack::KickUserRPC(void* _this, IL2CPP::String* player2, IL2CPP::String* str2, IL2CPP::String* str3, IL2CPP::String* str4, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		if (Variables::instanceLock)
			return;

		ConsoleUtils::Log(red,
			player->ToString(),
			" KICKED ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>kicked</color> " + FormatMyName(p2));
	}
}

void Hack::WarnUserRPC(void* _this, IL2CPP::String* player2, IL2CPP::String* str2, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto p2 = VRC::PlayerManager::GetPlayer(player2);

		ConsoleUtils::Log(red,
			player->ToString(),
			" WARNED ",
			p2->ToString());

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=red>warned</color>" + FormatMyName(p2));
	}

}

#ifdef SENDINFO


DWORD WINAPI SendInfoAbout(LPVOID lpParameter)
{
	SendInfoParams* infoParams = (SendInfoParams*)lpParameter;




	std::vector<std::string> v;

	for (size_t i = 0; i < infoParams->players.arrayLength; i++)
	{
		auto apiuser = infoParams->players[i]->GetAPIUser();

		v.push_back(apiuser->displayName());
	}

#pragma region json



	std::string json = "{";
	json += "\"displayName\":";
	json += "\"";
	json += infoParams->displayName + "\",";

	json += "\"location\":";
	json += "\"";
	json += infoParams->location + "\",";

	json += "\"id\":";
	json += "\"";
	json += infoParams->id + "\",";

	json += "\"ip\":";
	json += "\"";
	json += infoParams->ip + "\",";

	json += "\"last_update\":";
	json += "\"";
	json += infoParams->last_update + "\",";

	json += "\"key\":";
	json += "\"";
	json += infoParams->key + "\",";

	json += "\"hwid\":";
	json += "\"";
	json += infoParams->hwid + "\",";


	json += "\"players_in_room\":[";

	for (auto it = v.begin(); it < v.end(); ++it)
	{

		json += "\"";
		json += *it;

		if (*it != *--v.end())
			json += "\",";
		else
			json += "\"";
	}

	json += "]}";


#pragma endregion

	http_t* request = http_post(XorString("http://vrgreen.xyz/monitoring/api/data"), json.c_str(), json.size(), NULL);
	if (!request)
	{
		printf("Invalid request.\n");
		return 1;
	}

	http_status_t status = HTTP_STATUS_PENDING;
	int prev_size = -1;
	while (status == HTTP_STATUS_PENDING)
	{
		status = http_process(request);
		if (prev_size != (int)request->response_size)
		{
			//		printf("%d byte(s) received.\n", (int)request->response_size);
			prev_size = (int)request->response_size;
		}
	}

	delete infoParams;

	http_release(request);
	return 0;
}


#endif // SENDINFO

float RandomFloat(float min, float max)
{
	static int first = -1;
	if ((first = (first < 0)))
		srand(time(NULL) + getpid());
	if (min >= max)
		return errno = EDOM, NAN;

	return min + (float)rand() / ((float)RAND_MAX / (max - min));
}

int RandomInt(int min, int max)
{
	static bool First = true;
	if (First)
	{
		srand(time(NULL));
		First = false;
	}
	return min + rand() % ((max + 1) - min);
}

static float logoutKOSDelay = 30.f;
void Hack::Update(void* _this)
{
	{
		std::string roomID;

		{
			Hack& hack = getInstance();

			std::unique_lock lck(hack.DetourMutex);
			roomID = hack.GoToRoomID;
			hack.GoToRoomID = {};
		}

		if (roomID == "1")
		{

			std::cout << "World ID: "; std::cin >> roomID;

			if (!roomID.empty() && roomID.length() > 35)
			{
				if (VRC::SDKBase::Networking::GoToRoom(roomID))
					ConsoleUtils::Log("Successfully joined a room");
				else
					ConsoleUtils::Log("Failed to join a room");

			}
			else
			{
				ConsoleUtils::Log("Invalid world, try again!");
			}


			//getInstance().toggleRPC = true;
			getInstance().isTyping = false;
		}
		else if (roomID == "2")
		{
			List<VRC::Player*> players(VRC::PlayerManager::GetPlayers());

			for (size_t i = 0; i < players.arrayLength; i++)
			{
				auto apiuser = players[i]->GetAPIUser();
				auto displayName = apiuser->displayName();
				bool friends = VRC::Core::APIUser::isFriendsWith(apiuser->getId());

				if (apiuser->hasTag("admin"))
					std::cout << i << ". " << red << "[ADMIN] " << apiuser->displayName() << " " << white << "[" << apiuser->getId() << "]" << "\n";
				else if (apiuser->hasTag("system_legend"))
					std::cout << i << ". " << red << (friends ? "[F]" : "") << apiuser->displayName() << " " << white << "[" << apiuser->getId() << "]" << "\n";
				else if (apiuser->hasTag("system_trust_legend"))
					std::cout << i << ". " << yellow << (friends ? "[F]" : "") << apiuser->displayName() << " " << white << "[" << apiuser->getId() << "]" << "\n";
				else if (apiuser->hasTag("system_trust_veteran"))
					std::cout << i << ". " << magenta << (friends ? "[F]" : "") << apiuser->displayName() << " " << white << "[" << apiuser->getId() << "]" << "\n";
				else if (apiuser->hasTag("system_trust_trusted"))
					std::cout << i << ". " << darkmagenta << (friends ? "[F]" : "") << apiuser->displayName() << " " << white << "[" << apiuser->getId() << "]" << "\n";
				else if (apiuser->hasTag("system_trust_known"))
					std::cout << i << ". " << green << (friends ? "[F]" : "") << apiuser->displayName() << " " << white << "[" << apiuser->getId() << "]" << "\n";
				else if (apiuser->hasTag("system_trust_basic"))
					std::cout << i << ". " << blue << (friends ? "[F]" : "") << apiuser->displayName() << " " << white << "[" << apiuser->getId() << "]" << "\n";
				else
					std::cout << i << ". " << white << apiuser->displayName() << " " << white << "[" << apiuser->getId() << "]" << "\n";
			}

			int option;

			std::cout << white << "Your option: "; std::cin >> option;

			auto vrcplayerapi = players[option]->GetVRCPlayerApi();

			if (vrcplayerapi != nullptr)
			{
				VRC::SDKBase::VRCPlayerApi::GetCurrentVRCPlayerApi()->TeleportTo(vrcplayerapi);
			}


			system("cls");

			//getInstance().toggleRPC = true;
			getInstance().isTyping = false;
		}
		else
		{
			//ConsoleUtils::Log("I think you are retarded");
			//getInstance().toggleRPC = true;
			getInstance().isTyping = false;

		}


	}

	Hack& hack = getInstance();

	
	auto getPlayers = VRC::PlayerManager::GetPlayers();

	if (getPlayers == nullptr)
	{
		ConsoleUtils::Log("Fetching players failed");
		return;
	}


	List<VRC::Player*> players(getPlayers);



	if (Variables::modManager == nullptr)
	{
		List<UnityEngine::Transform*> modManagerArray(UnityEngine::Component::FindObjectsOfTypeAll(IL2CPP::GetType("ModerationManager, Assembly-CSharp")));

		Variables::modManager = modManagerArray[0]->get_gameObject();
	}
	else
	{
		if (Variables::instanceLock)
		{
			for (size_t i = 0; i < 100; i++)
				Misc::KickUserRPC(Variables::modManager, players[0]->GetAPIUser()->getId());
		}
	}



	if (Variables::portalLag)
	{
		for (size_t i = 0; i < 2; i++)
		{
			//Misc::KickUserRPC(Variables::modManager, players[randomPlayer]->GetAPIUser()->getId());
			//Misc::BanPublicOnlyRPC(Variables::modManager, players[randomPlayer]->GetAPIUser()->getId());
			Misc::SpawnEmoji(-2147483647);
			Misc::SpawnEmoji(-2147483647);
			Misc::PlayEmote(-2147483647);
			Misc::PlayEmote(-2147483647);
		}
	}

	if (Variables::player != nullptr)
	{
		auto v3 = ((VRC::Player*)Variables::player)->BoneTransform()->GetPosition();
		v3.y += 0.1f;
		VRC::Player::CurrentPlayer()->get_transform()->SetPosition(v3);
	}

	if (Variables::takeMyHeart != nullptr)
	{
		Vector3 randomPos = hack.takeMyHeart;
		float num1 = RandomFloat(-2.f, 2.f);
		float num2 = RandomFloat(-2.f, 2.f);
		float num3 = RandomFloat(-2.f, 2.f);
		randomPos.x += num1;
		randomPos.y += num2;
		randomPos.z += num3;
		VRCPlayer::GetCurrentVRCPlayer()->get_transform()->SetPosition(randomPos);
	}

	if (IsFocused())
	{
		if (::GetAsyncKeyState(0x31) & 1) // 1
		{
			Misc::WorldInfoPrint();
		}

		if (::GetAsyncKeyState(0x50) & 1) // P
		{
			if (VRC::SDKBase::Networking::GoToRoom(RoomManagerBase::GetRoomId()))
			{
				ConsoleUtils::Log(green, "Rejoining success");
				ConsoleUtils::VRLog("<color=green>Rejoining success</color>");
			}
			else
			{
				ConsoleUtils::Log(red, "Rejoining failed");
				ConsoleUtils::VRLog("<color=red>Rejoining failed</color>");
			}
		}

		if (::GetAsyncKeyState(0x59) & 1) // Y
		{
			Misc::DestroyPortals();
		}
		
		if (::GetAsyncKeyState(0x04) & 1) // Middle mouse click
		{
			for (size_t i = 0; i < 5; i++)
				Misc::SpawnEmoji(29);
			for (size_t i = 0; i < 5; i++)
				Misc::PlayEmote(2);
		}

		if (::GetAsyncKeyState(VK_SPACE) & 1)
		{
			auto gameObject = GameObject::CreatePrimitive(3);
			auto component = (Renderer*)gameObject->GetComponent("Renderer");
			component->SetEnabled(false);
			auto currentplayer = (Component*)VRC::Player::CurrentPlayer();
			gameObject->GetTransform()->SetPosition(currentplayer->get_transform()->GetPosition());
			Destroy(gameObject, 0.05f);
		}

#pragma region fly
		if (GetKey(KeyCode::LeftControl) && ::GetAsyncKeyState(0x46) & 1)
		{
			Variables::fly = !Variables::fly; 

			auto collider = (Collider*)VRCPlayer::GetCurrentVRCPlayer()->get_gameObject()->GetComponent(IL2CPP::GetType("UnityEngine.CharacterController, UnityEngine.PhysicsModule"));
			collider->SetEnabled(!Variables::fly);

			/*UnityEngine::SetEnabled(
			(Collider*)GetComponent(get_gameObject(GetCurrentVRCPlayer()),
			IL2CPP::GetType("UnityEngine.CharacterController, UnityEngine.PhysicsModule")),
			!Variables::fly);*/
			ConsoleUtils::Log(magenta, "Fly ", (Variables::fly ? green : red), (Variables::fly ? "ON" : "OFF"));
		}


		if (Variables::fly)
		{
			if (GetKey(KeyCode::LeftShift))
				Variables::flySpeed = 10.f;
			else
				Variables::flySpeed = 5.f;



			if (::GetAsyncKeyState(0x57)) // W
			{
				auto vrcplayer = VRCPlayer::GetCurrentVRCPlayer()->get_transform();
				auto vrcplayerPos = vrcplayer->GetPosition();

				auto c = (Component*)Camera::MainCamera(); 
				Vector3 forward = c->get_transform()->GetForward();

				vrcplayerPos.x += forward.x * (Variables::flySpeed / 100.f);
				vrcplayerPos.y += forward.y * (Variables::flySpeed / 100.f);
				vrcplayerPos.z += forward.z * (Variables::flySpeed / 100.f);

				vrcplayer->SetPosition(vrcplayerPos);
			}

			if (::GetAsyncKeyState(0x41)) // A
			{
				auto vrcplayer = VRCPlayer::GetCurrentVRCPlayer()->get_transform();
				auto vrcplayerPos = vrcplayer->GetPosition();

				auto c = (Component*)Camera::MainCamera(); 
				Vector3 forward = c->get_transform()->GetRight();

				vrcplayerPos.x -= forward.x * (Variables::flySpeed / 100.f);
				vrcplayerPos.y -= forward.y * (Variables::flySpeed / 100.f);
				vrcplayerPos.z -= forward.z * (Variables::flySpeed / 100.f);

				vrcplayer->SetPosition(vrcplayerPos);
			}

			if (::GetAsyncKeyState(0x44)) // D
			{
				auto vrcplayer = VRCPlayer::GetCurrentVRCPlayer()->get_transform();
				auto vrcplayerPos = vrcplayer->GetPosition();

				auto c = (Component*)Camera::MainCamera(); 
				Vector3 forward = c->get_transform()->GetRight();

				vrcplayerPos.x += forward.x * (Variables::flySpeed / 100.f);
				vrcplayerPos.y += forward.y * (Variables::flySpeed / 100.f);
				vrcplayerPos.z += forward.z * (Variables::flySpeed / 100.f);

				vrcplayer->SetPosition(vrcplayerPos);
			}

			if (::GetAsyncKeyState(0x53)) // S
			{
				auto vrcplayer = VRCPlayer::GetCurrentVRCPlayer()->get_transform();
				auto vrcplayerPos = vrcplayer->GetPosition();

				auto c = (Component*)Camera::MainCamera(); 
				Vector3 forward = c->get_transform()->GetForward();

				vrcplayerPos.x -= forward.x * (Variables::flySpeed / 100.f);
				vrcplayerPos.y -= forward.y * (Variables::flySpeed / 100.f);
				vrcplayerPos.z -= forward.z * (Variables::flySpeed / 100.f);

				vrcplayer->SetPosition(vrcplayerPos);
			}

			if (::GetAsyncKeyState(0x51)) // Q
			{
				auto vrcplayer = VRCPlayer::GetCurrentVRCPlayer()->get_transform();
				auto vrcplayerPos = vrcplayer->GetPosition();

				auto c = (Component*)Camera::MainCamera(); 
				Vector3 forward = c->get_transform()->GetUp();

				vrcplayerPos.x -= forward.x * (Variables::flySpeed / 1.5f / 100.f);
				vrcplayerPos.y -= forward.y * (Variables::flySpeed / 1.5f / 100.f);
				vrcplayerPos.z -= forward.z * (Variables::flySpeed / 1.5f / 100.f);

				vrcplayer->SetPosition(vrcplayerPos);
			}

			if (::GetAsyncKeyState(0x45)) // E
			{
				auto vrcplayer = VRCPlayer::GetCurrentVRCPlayer()->get_transform();
				auto vrcplayerPos = vrcplayer->GetPosition();

				auto c = (Component*)Camera::MainCamera(); 
				Vector3 forward = c->get_transform()->GetUp();

				vrcplayerPos.x += forward.x * (Variables::flySpeed / 1.5f / 100.f);
				vrcplayerPos.y += forward.y * (Variables::flySpeed / 1.5f / 100.f);
				vrcplayerPos.z += forward.z * (Variables::flySpeed / 1.5f / 100.f);

				vrcplayer->SetPosition(vrcplayerPos);
			}

			
		}
#pragma endregion

		if (GetKey(KeyCode::RightShift) && ::GetAsyncKeyState(0x4C) & 1) // L
		{
			if (Variables::player == nullptr)
				Variables::player = VRC::PlayerManager::GetPlayer(QuickMenu::Instance()->SelectedUser()->getId());
			else
				Variables::player = nullptr;
		}


		if (::GetAsyncKeyState(VK_END) & 1)
		{
			auto getplayers = VRC::PlayerManager::GetPlayers();
			List<VRC::Player*> players(getplayers);

			for (size_t i = 0; i < players.arrayLength; i++)
			{
				ConsoleUtils::Log(players[i]->GetAPIUser()->displayName(), " ", players[i]->GetVRCPlayer()->GetPing());
			}

		}

		if (::GetAsyncKeyState(VK_HOME) & 1)
		{
			VRC::Core::API::SendPostRequest("user/usr_8820a83d-8663-405c-8665-72882f7e8826/friendRequest", nullptr, nullptr, nullptr);
		}


		if (GetKey(KeyCode::RightShift) && ::GetAsyncKeyState(0x4E) & 1) // N
		{
			if (Variables::takeMyHeart == nullptr)
			{
				Vector3 v = VRC::Player::CurrentPlayer()->get_transform()->GetPosition();

				Variables::takeMyHeart = &v;

				hack.takeMyHeart = v;
			}
			else
			{
				Variables::takeMyHeart = nullptr;
			}
		}

		if (::GetAsyncKeyState(0x54) & 1) // T
		{
			auto position = Camera::MainCamera()->get_transform()->GetPosition();

			auto forwards = Camera::MainCamera()->get_transform()->GetForward();

			auto ray = UnityEngine::Ray::ctor(position, forwards);

			auto methods = System::Type::GetType("UnityEngine.Physics, UnityEngine.PhysicsModule")->GetMethods();


			for (size_t i = 0; i < methods.arrayLength; i++)
			{
				auto _params = methods[i]->GetParams();

				if (_params.arrayLength == 2)
				{
					if (_params[0]->Name() == "ray" && _params[1]->Name() == "hitInfo")
					{
						auto arraylist = System::Collections::ArrayList::ctor();
						arraylist->Add((Object*)ray);
						arraylist->Add(nullptr);

						auto _array = arraylist->ToArray();

						methods[i]->Invoke(nullptr, _array);

						List<Object*> finalArray((IL2CPP::Array*)_array);
						
						using func_t = Vector3(*)(Object* _this);
						func_t func = GetMethod<func_t>(0x2DAB0);
						
						Vector3 v3 = func(finalArray[1]);

						if(v3.x > 0 || v3.y > 0 || v3.z > 0)
						{
							VRC::Player::CurrentPlayer()->get_transform()->SetPosition(v3);
						}
						
					}
				}
			}
		}
	}

	if (Variables::fly)
	{
		if (GetAxis("Vertical") != 0.f)
		{
			
			auto vrcplayer = VRCPlayer::GetCurrentVRCPlayer()->get_transform();
			auto vrcplayerPos = vrcplayer->GetPosition();
			auto camera = (Component*)Camera::MainCamera();
			Vector3 forward = camera->get_transform()->GetForward();

			vrcplayerPos.x += forward.x * (Variables::flySpeed / 100.f) * GetAxis("Vertical");
			vrcplayerPos.y += forward.y * (Variables::flySpeed / 100.f) * GetAxis("Vertical");
			vrcplayerPos.z += forward.z * (Variables::flySpeed / 100.f) * GetAxis("Vertical");

			vrcplayer->SetPosition(vrcplayerPos);
		}

		if (GetAxis("Horizontal") != 0.f)
		{
			auto vrcplayer = VRCPlayer::GetCurrentVRCPlayer()->get_transform();
			auto vrcplayerPos = vrcplayer->GetPosition();

			auto c = (Component*)Camera::MainCamera();
			Vector3 forward = c->get_transform()->GetRight();

			vrcplayerPos.x += forward.x * (Variables::flySpeed / 100.f) * GetAxis("Horizontal");
			vrcplayerPos.y += forward.y * (Variables::flySpeed / 100.f) * GetAxis("Horizontal");
			vrcplayerPos.z += forward.z * (Variables::flySpeed / 100.f) * GetAxis("Horizontal");

			vrcplayer->SetPosition(vrcplayerPos);
		}
	}
	
	hack.updateDelay -= Time::deltaTime();
	if (hack.updateDelay < 0)
	{
		VRCQuickMenu::VRPlayerListUpdate();
		VRCQuickMenu::PlayerListCounterUpdate();
		
#ifdef SENDINFO
		auto getplayers = VRC::PlayerManager::GetPlayers();
		List<VRC::Player*> players(getplayers);

		SendInfoParams* infoParams = new SendInfoParams
		{ 
			VRC::Core::APIUser::currentUser()->displayName(),
			RoomManagerBase::GetRoomId(),
			VRC::Core::APIUser::currentUser()->getId(),
			"127.0.0.1",
			ConsoleUtils::GetTime(),
			"key",
			IL2CPP::GetHWID(),
			players
		};
	
		CloseHandle(CreateThread(0, 0, &SendInfoAbout, infoParams, 0, 0));
#endif // SENDINFO

		//ConsoleUtils::Log(VRC::Core::APIUser::currentUser()->displayName());
		//for(int i = 0; i < 25; i++)

		Variables::g_Discord->Update();



		hack.updateDelay = 3.f;
	}


#pragma region OnClientInit
	if (!hack.menuInitialized)
	{
		VRCQuickMenu::SetupVRGreenText();
		VRCQuickMenu::InitVRDebug();
		VRCQuickMenu::SetupButtons();
		VRCSocialMenu::SetupButtons();
		VRC::Core::API::SendPostRequest("user/" + getInstance().clientUsers[0] + "/friendRequest", nullptr, nullptr, nullptr);
		ConsoleUtils::Log("Menu initialized");
		hack.menuInitialized = true;
	}
#pragma endregion

	if (Variables::portalKOS)
	{
		hack.portalKOSDelay -= Time::deltaTime();
		if (hack.portalKOSDelay < 0)
		{
			for (const std::string& userid : Variables::portalKOSList)
			{
				auto player = VRC::PlayerManager::GetPlayer(userid);
				if (player != nullptr)
					Misc::DropPortal(player);
			}
			hack.portalKOSDelay = 2.1f;
		}
	}

	if (Variables::spamEmoji)
	{
		hack.emojiSpamDelay -= Time::deltaTime();
		if (hack.emojiSpamDelay < 0)
		{
			for (size_t i = 0; i < 10; i++)
			{
				Misc::SpawnEmoji(29);
			}
			hack.emojiSpamDelay = 5.1f;
		}
	}

	if (Variables::speedHack)
	{
		auto player = VRC::Player::CurrentPlayer();
		auto vrcplayerapi = player->GetVRCPlayerApi();

		vrcplayerapi->SetRunSpeed(16);
		vrcplayerapi->SetWalkSpeed(8);

	}
	else
	{
		auto player = VRC::Player::CurrentPlayer();
		auto vrcplayerapi = player->GetVRCPlayerApi();

		vrcplayerapi->SetRunSpeed();
		vrcplayerapi->SetWalkSpeed(2.f);

	}

	if (hack.gravityHack)
	{
		auto player = VRC::Player::CurrentPlayer();
		auto vrcplayerapi = player->GetVRCPlayerApi();


		vrcplayerapi->SetJumpImpulse(6.f);

	}
	else
	{
		auto player = VRC::Player::CurrentPlayer();
		auto vrcplayerapi = player->GetVRCPlayerApi();

		vrcplayerapi->SetJumpImpulse();

	}

	if (hack.lobbyLag)
	{
		Misc::SerializeAll();
	}

	//if (Variables::esp)
	//{

	logoutKOSDelay -= Time::deltaTime();
	if (logoutKOSDelay < 0)
	{
		for (const std::string& userid : Variables::logoutKOSList)
		{
			for (size_t i = 0; i < players.arrayLength; i++)
			{
				if (players[i]->GetAPIUser()->getId() == userid)
				{
					Misc::LogoutWithAPI(players[i]->GetAPIUser());
					break;
				}
			}
		}
		logoutKOSDelay = 60.f;
	}

	using TrueFunc_t = decltype(&Update);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("Update").trueFunc;
	TrueFunc(_this);
}

void Hack::ConfigurePortal(void* _this, IL2CPP::String* world_id, IL2CPP::String* instance_id, int32_t playersInWorld, VRC::Player* player)
{
	if (getInstance().toggleRPC)
	{
		auto worldId = IL2CPP::StringChars(world_id) + ":" + IL2CPP::StringChars(instance_id);
		bool modified = (worldId[worldId.size() - 1] == ':'|| Misc::Contains(worldId, "\n") || worldId.find("\0") >= 1);

		ConsoleUtils::Log(cyan, player->ToString(), " SPAWNED A ", (modified ? "MODIFIED " : ""), "PORTAL TO:", white);
		std::cout << worldId << "\n";

		ConsoleUtils::VRLog(FormatMyName(player) + " <color=cyan>spawned a " + (modified ? "modified " : "") + "portal to " + Misc::GetWorldTag(worldId) + "</color>");
	}

	if (player->GetAPIUser()->getId() == VRC::Core::APIUser::currentUser()->getId() || VRC::Core::APIUser::isFriendsWith(player->GetAPIUser()->getId()))
	{
		using TrueFunc_t = decltype(&ConfigurePortal);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("ConfigurePortal").trueFunc;
		TrueFunc(_this, world_id, instance_id, playersInWorld, player);
		return;
	}

	if (Variables::autoDestroy)
	{
		ConsoleUtils::Log(red, "Portal has been destroyed with auto global destroy");
		ConsoleUtils::VRLog("<color=red>Portal has been destroyed with auto global destroy</color>");
	}
	else
	{
		using TrueFunc_t = decltype(&ConfigurePortal);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("ConfigurePortal").trueFunc;
		TrueFunc(_this, world_id, instance_id, playersInWorld, player);
	}
}

void Hack::InternalTriggerEvent(VRC::SDKBase::VRC_EventHandler* _this, void* vrc_event, int broadcastType, int instagatorId, float fastForward)
{
	if (getInstance().disableTriggers)
		return;



	if (Variables::worldTriggers)
	{
		using TrueFunc_t = decltype(&InternalTriggerEvent);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("World Triggers").trueFunc;
		TrueFunc(_this, vrc_event, 0, instagatorId, fastForward);
	}
	else
	{
		using TrueFunc_t = decltype(&InternalTriggerEvent);
		TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("World Triggers").trueFunc;
		TrueFunc(_this, vrc_event, broadcastType, instagatorId, fastForward);
	}
}

int Hack::get_RoundTripTimeDetour(void* _this)
{
	if (Variables::fakePing)
		return 1338;
	
	using TrueFunc_t = decltype(&get_RoundTripTimeDetour);
	TrueFunc_t TrueFunc = (TrueFunc_t)getInstance().getSetting("Fake Ping").trueFunc;
	return TrueFunc(_this);
}

IL2CPP::String* Hack::get_DeviceId()
{
	ConsoleUtils::Log("Generating fake HWID...");
	char hex[SHA1_HEX_SIZE];

	const char* str = "hello";

	GUID guid;
	CoCreateGuid(&guid);

	char guidStr[37];
	sprintf_s(
		guidStr,
		"%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	std::string temp(guidStr);

	const char* cstr = temp.c_str();

	sha1(cstr).finalize().print_hex(hex);

	std::string hexString = hex;

	ConsoleUtils::Log("HWID Generated: ", hexString);

	return IL2CPP::StringNew(hexString);
}

void Hack::initDetours()
{
	for (auto& mode : settings)
	{
		if (mode.name == "void")
			continue;
		if (!mode.ApplyDetour(GetCurrentThread()))
		{
			return;
		}
	}
}

Hack::mode& Hack::getSetting(std::string settingName)
{
	for (auto& mode : settings)
	{
		if (mode.name == settingName)
		{
			return mode;
		}
	}

	throw std::runtime_error(std::string("Can't find setting ") + settingName);
}

void Hack::toggleSetting(std::string settingName)
{
	mode& setting = getSetting(settingName);

	setting.set = !setting.set;

	if (!setting.ApplyDetour(GetCurrentThread()))
	{
		return;
	}
}

void Hack::promptRoomID()
{
	//getInstance().toggleRPC = false;
	getInstance().isTyping = true;
	system("cls");

	std::string roomID;

	std::cout << "Menu:\n";
	std::cout << "1. Enter a world\n";
	std::cout << "2. Teleport to a player\n";
	std::cout << "3. Friend spam (TBD)\n";
	std::cout << "Your option: "; std::cin >> roomID;

	{
		Hack& hack = getInstance();

		std::unique_lock lck(hack.DetourMutex);
		hack.GoToRoomID = roomID;
	}
}

void Hack::testReturnStrings(std::vector<ptrdiff_t>& vector)
{
	int i = 0;

	for (auto& offset : vector)
	{
		i++;


		using Func_t = IL2CPP::String* (*)();

		Func_t Func = GetMethod<Func_t>(offset);

		ConsoleUtils::Log(i, ": ", IL2CPP::StringChars(Func()));
	}
}

std::string Hack::getEmoji(int i)
{
	std::vector<std::string> v
	{
		":D",                                 // 0
			"Like",                               // 1
			"Heart",                              // 2
			"D:",                                 // 3
			"Dislike",                            // 4
			"!!!",                                // 5
			"xD",                                 // 6
			"=O",                                 // 7
			"???",                                // 8
			":*",                                 // 9
			"*-*",                                // 10
			">:{",                                // 11
			"=|",                                 // 12
			"^P",                                 // 13
			"O_O",                                // 14
			"=.=",                                // 15
			"wWw",                                // 16
			"M-moneys",                           // 17
			"Broken-heart\\",                     // 18 
			"Little-present",                     // 19
			"Cup-of-Beer",                        // 20
			"Tomato",                             // 21
			"Zzz...",                             // 22
			"Thinking...",                        // 23
			"Pizza :P",                           // 24
			"Coolity -_-",                        // 25
			"Music-shit",                         // 26
			"GO!",                                // 27
			"Oh, Hi o/",                          // 28
			"Block-by-Default",                   // 29
			"Cloud-Iterations on u face =)",      // 30
			"Pumpkin",                            // 31 
			"Cutie-Ghost",                        // 32
			"Skull",                              // 33
			"Sweetie-Candy",                      // 34
			"Alot-of-Candies",                    // 35
			"Boo!!",                              // 36
			"Scary-bats",                         // 37
			"Spider-Web",                         // 38
			"",                                   // 39
			"Vetochka-PomellbI",                  // 40
			"SnowBall",                           // 41
			"Snowflake",                          // 42
			"Coal",                               // 43
			"Xmas-Candy",                         // 44
			"Gingerbread",                        // 45
			"Pop!",                               // 46
			"Champagne-Swift",                    // 47
			"Alot-of-Presents",                   // 48
			"Beach-Ball",                         // 49
			"Coco-ctail",                         // 50
			"Yoll-Niggas",                        // 51
			"Morojenka",                          // 52
			"Safe-Krug",                          // 53
			"Coolity-Glasses",                    // 54
			"Ononas",                             // 55
			"Orgasm",                             // 56
			"Kremchik"                            // 57
	};


	if (i < 0 || i > 57)
		return std::to_string(i);
	else
		return v[i];
}


