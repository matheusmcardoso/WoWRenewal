#include "ScriptMgr.h"
#include "MapManager.h"

class renewal_hearthstone : ItemScript
{
public:
	renewal_hearthstone() : ItemScript("renewal_hearthstone") {}

	// may return null_ptr
	const MapEntry* GetMapEntryFromMapId(uint32 mapId) {
		return sMapStore.LookupEntry(mapId);
	}

	const std::string GetMapNameFromMapId(uint32 mapId, LocaleConstant locale = LOCALE_enUS) {
		auto entry = GetMapEntryFromMapId(mapId);
		if (!entry)
			return "UNKNOWN_MAPID_" + std::to_string(mapId);
		return entry->name[locale];
	}

	// may return null_ptr
	const AreaTableEntry* GetAreaTableEntryFromAreaId(uint32 areaId) {
		return sAreaTableStore.LookupEntry(areaId);
	}

	// may return null_ptr
	const uint32 GetAreaIdFromWorldLocation(WorldLocation w) {
		return MapManager::instance()->GetAreaId(w.m_mapId, w.m_positionX, w.m_positionY, w.m_positionZ);
	}

	// may return null_ptr
	const AreaTableEntry* GetAreaTableEntryFromWorldLocation(WorldLocation w) {
		auto areaId = GetAreaIdFromWorldLocation(w);
		return GetAreaTableEntryFromAreaId(areaId);
	}

	const std::string GetAreaNameFromAreaId(uint32 areaId, LocaleConstant locale = LOCALE_enUS) {
		auto entry = GetAreaTableEntryFromAreaId(areaId);
		if (!entry)
			return "UNKNOWN_AREAID_" + std::to_string(areaId);
		return std::string(entry->area_name[locale]);
	}

	const std::string GetAreaNameFromWorldLocation(WorldLocation w, LocaleConstant locale = LOCALE_enUS) {
		auto areaId = GetAreaIdFromWorldLocation(w);
		return GetAreaNameFromAreaId(areaId, locale);
	}

	std::vector<WorldLocation> GetPlayerHomes(Player* player) {

		auto homes = std::vector<WorldLocation>();

		std::string query = "SELECT mapId, areaId, x, y, z FROM renewal_hearthstone_homes WHERE character_guid = ";
		query += std::to_string(player->GetSession()->GetGUIDLow()) + " ORDER BY areaId;";

		auto res = CharacterDatabase.Query(query.c_str());
		
		if (res) {
			do
			{
				Field* field = res->Fetch();

				uint16 mapId = field[0].GetUInt16();

				if (!GetMapEntryFromMapId(mapId))
					continue;

				uint16 areaId = field[1].GetUInt16();

				if (!GetAreaTableEntryFromAreaId(areaId))
					continue;

				float x = field[2].GetFloat();
				float y = field[3].GetFloat();
				float z = field[4].GetFloat();

				auto location = WorldLocation(mapId, x, y, z, 0.0);

				if (!GetAreaTableEntryFromWorldLocation(location))
					continue;

				homes.push_back(location);

			} while (res->NextRow());
		}

		return homes;
	}

	bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) {

		auto locale = player->GetSession()->GetSessionDbLocaleIndex();
		auto homes = GetPlayerHomes(player);

		ClearGossipMenuFor(player);

		auto i = 1;
		for (auto h : homes) {

			auto mapName = GetMapNameFromMapId(h.m_mapId, locale);
			auto areaName = GetAreaNameFromWorldLocation(h, locale);

			AddGossipItemFor(player, 0, areaName + " (" + mapName + ")",
				GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + i);

			i++;
		}

		SendGossipMenuFor(player, 135555, item->GetGUID());

		return true;
	}

	void OnGossipSelect(Player* player, Item* item, uint32 /*sender*/, uint32 action) {

		ClearGossipMenuFor(player);

		auto homes = GetPlayerHomes(player);
		auto choice = action - GOSSIP_ACTION_INFO_DEF - 1;
		auto home = homes[choice];

		player->SetHomebind(home, GetAreaIdFromWorldLocation(home));
		player->CastStop();
		player->CastItemUseSpell(item, SpellCastTargets(), 1, 0);

		CloseGossipMenuFor(player);
	}

	bool OnRemove(Player* /*player*/, Item* /*item*/) {
		return true;
	}
};

void AddSC_renewal_hearthstone()
{
	new renewal_hearthstone();
}