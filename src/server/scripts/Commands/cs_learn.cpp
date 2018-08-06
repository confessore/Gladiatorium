/*
 * Copyright (C) 2008-2018 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: learn_commandscript
%Complete: 100
Comment: All learn related commands
Category: commandscripts
EndScriptData */

#include "ScriptMgr.h"
#include "Chat.h"
#include "DBCStores.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "RBAC.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "WorldSession.h"

class learn_commandscript : public CommandScript
{
public:
    learn_commandscript() : CommandScript("learn_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> learnAllMyCommandTable =
        {
            { "class",      rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_CLASS,      false, &HandleLearnAllMyClassCommand,      "" },
            { "pettalents", rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_PETTALENTS, false, &HandleLearnAllMyPetTalentsCommand, "" },
            { "spells",     rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_SPELLS,     false, &HandleLearnAllMySpellsCommand,     "" },
            { "talents",    rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_TALENTS,    false, &HandleLearnAllMyTalentsCommand,    "" },
        };

        static std::vector<ChatCommand> learnAllCommandTable =
        {
            { "my",      rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY,      false, nullptr,                          "", learnAllMyCommandTable },
            { "gm",      rbac::RBAC_PERM_COMMAND_LEARN_ALL_GM,      false, &HandleLearnAllGMCommand,      "" },
            { "crafts",  rbac::RBAC_PERM_COMMAND_LEARN_ALL_CRAFTS,  false, &HandleLearnAllCraftsCommand,  "" },
            { "default", rbac::RBAC_PERM_COMMAND_LEARN_ALL_DEFAULT, false, &HandleLearnAllDefaultCommand, "" },
            { "lang",    rbac::RBAC_PERM_COMMAND_LEARN_ALL_LANG,    false, &HandleLearnAllLangCommand,    "" },
            { "recipes", rbac::RBAC_PERM_COMMAND_LEARN_ALL_RECIPES, false, &HandleLearnAllRecipesCommand, "" },
        };

        static std::vector<ChatCommand> learnCommandTable =
        {
            { "all", rbac::RBAC_PERM_COMMAND_LEARN_ALL, false, nullptr,                "", learnAllCommandTable },
            { "",    rbac::RBAC_PERM_COMMAND_LEARN,     false, &HandleLearnCommand, "" },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "learn",   rbac::RBAC_PERM_COMMAND_LEARN,   false, nullptr,                  "", learnCommandTable },
            { "unlearn", rbac::RBAC_PERM_COMMAND_UNLEARN, false, &HandleUnLearnCommand, "" },
        };
        return commandTable;
    }

    static bool HandleLearnCommand(ChatHandler* handler, char const* args)
    {
        Player* targetPlayer = handler->getSelectedPlayerOrSelf();

        if (!targetPlayer)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spell = handler->extractSpellIdFromLink((char*)args);
        if (!spell || !sSpellMgr->GetSpellInfo(spell))
            return false;

        char const* all = strtok(nullptr, " ");
        bool allRanks = all ? (strncmp(all, "all", strlen(all)) == 0) : false;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell);
        if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer()))
        {
            handler->PSendSysMessage(LANG_COMMAND_SPELL_BROKEN, spell);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!allRanks && targetPlayer->HasSpell(spell))
        {
            if (targetPlayer == handler->GetSession()->GetPlayer())
                handler->SendSysMessage(LANG_YOU_KNOWN_SPELL);
            else
                handler->PSendSysMessage(LANG_TARGET_KNOWN_SPELL, handler->GetNameLink(targetPlayer).c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (allRanks)
            targetPlayer->LearnSpellHighestRank(spell);
        else
            targetPlayer->LearnSpell(spell, false);

        if (GetTalentSpellCost(spellInfo->GetFirstRankSpell()->Id))
            targetPlayer->SendTalentsInfoData(false);

        return true;
    }

    static bool HandleLearnAllGMCommand(ChatHandler* handler, char const* /*args*/)
    {
        for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(i);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer(), false))
                continue;

            if (!spellInfo->IsAbilityOfSkillType(SKILL_INTERNAL))
                continue;

            handler->GetSession()->GetPlayer()->LearnSpell(i, false);
        }

        handler->SendSysMessage(LANG_LEARNING_GM_SKILLS);
        return true;
    }

    static bool HandleLearnAllMyClassCommand(ChatHandler* handler, char const* /*args*/)
    {
        HandleLearnAllMySpellsCommand(handler, "");
        HandleLearnAllMyTalentsCommand(handler, "");
        return true;
    }

    static bool HandleLearnAllMySpellsCommand(ChatHandler* handler, char const* /*args*/)
    {
        std::vector<int> ignore =
        {
            // warrior
            47994, 30223, 30219, 30213, 29842, 29841, 64380, 23885, 23880, 20647, 12970, 12969, 12968, 12967, 12966,

            // paladin
            64891, 48823, 48822, 48821, 48820, 33074, 33073, 27176, 27175, 25914, 25913, 25912, 25911, 25903, 25902,
            20185, 20187, 20186, 20184, 20154,  25997, 31803, 20467, 20425, 53726, 31804, 67, 26017, 53742, 31898,
            53733, 34767, 34769, 66906, 66907, 

            // hunter
            55754, 55753, 55752, 55751, 55750, 55749, 52399, 52398, 52397, 52396, 52395, 50433, 52474, 52473, 27050,
            17261, 17260, 17259, 17258, 17257, 17256, 17255, 17253, 52472, 52471, 27049, 3009, 3010, 16832, 16831,
            16830, 16829, 16828, 16827, 1742, 55487, 27051, 24579, 24578, 24577, 24423, 50285, 55485, 55484, 55483,
            55482, 35323, 34889, 55492, 55491, 55490, 55489, 55488, 54644, 64495, 64494, 64493, 64492, 64491, 24604,
            35295, 35294, 35293, 35292, 35291, 35290, 61676, 27047, 14921, 14920, 14919, 14918, 14917, 14916, 2649,
            58611, 58610, 58609, 58608, 58607, 58604, 25012, 25011, 25010, 25009, 25008, 24844, 55499, 55498, 55497,
            55496, 55495, 54680, 53589, 53588, 53587, 53586, 53584, 50479, 53548, 53547, 53546, 53545, 53544, 50245,
            55557, 55556, 55555, 35392, 35389, 35387, 24453, 24452, 24450, 26090, 59886, 59885, 59884, 59883, 59882,
            59881, 53562, 53561, 53560, 53559, 53558, 50518, 53582, 53581, 53580, 53579, 53578, 50498, 55728, 27060,
            24587, 24586, 24583, 24640, 52016, 52015, 52014, 52013, 52012, 50318, 26064, 52476, 52475, 49974, 49973,
            49972, 49971, 49970, 49969, 49968, 49967, 49966, 53543, 53542, 53540, 53538, 53537, 50541, 53568, 53567,
            53566, 53565, 53564, 50519, 61198, 61197, 61196, 61195, 61194, 61193, 53598, 53597, 53596, 53594, 53593,
            50274, 57393, 57392, 57391, 57390, 57389, 57386, 56631, 56630, 56629, 56628, 56627, 56626, 53533, 53532,
            53529, 53528, 53526, 50256, 53575, 53574, 53573, 53572, 53571, 50271, 55509, 55508, 55507, 55506, 55505,
            54706, 35346, 4167, 24406, 34471, 58433, 58432, 42234, 42245, 42244, 42243, 53254, 49065, 49064, 27026,
            14315, 14314, 13812, 60210, 60202, 49054, 49053, 27024, 14301, 14300, 14299, 14298, 13797, 49010, 49009,
            27069, 24135, 24134, 24131, 25329, 27805, 27804, 

            // rogue
            27099, 27097, 27096, 27095, 24224, 57933,

            // priest
            32841, 56131, 56160, 53003, 53002, 53001, 53000, 52999, 52998, 52988, 52987, 52986, 52985, 52984, 52983,
            47758, 47757, 47750, 47666, 33619, 64844, 56161, 48153, 34754, 48076, 48075, 27803, 23459, 23458, 23455,
            64904, 48085, 48084, 28276, 27874, 27873, 7001, 33110, 53022, 49821,

            // death knight
            52374, 57532, 49142, 52372, 62904, 62903, 62902, 62901, 62900, 52375, 47633, 47632, 45470, 45469, 52373,
            50536,

            // shaman
            49269, 49268, 45302, 45301, 45300, 45299, 45298, 45297, 61654, 61650, 25537, 25535, 11307, 11306, 8503,
            8502, 8349, 63685, 49240, 49239, 45296, 45295, 45294, 45293, 45292, 45291, 45290, 45289, 45288, 45287,
            45286, 45285, 45284, 30708, 43339, 49279, 49278, 26372, 26371, 26370, 26369, 26367, 26366, 26365, 26364,
            26363, 32176, 32175, 35886, 379, 21169, 36591, 18848,

            // mage
            42845, 42844, 38703, 38700, 27076, 25346, 10274, 10273, 8418, 8419, 7270, 7269, 7268, 12536, 61316,
            61024, 54648, 61780, 61721, 61305, 61025, 28272, 28271, 31643, 44450, 24530, 12654, 55362, 55361, 44461,
            42938, 42937, 42198, 42213, 42212, 42211, 42210, 42209, 42208, 12486, 12485, 12484, 12494, 70909, 43044,
            43043, 34913,

            // warlock
            47993, 33700, 33699, 33698, 47982, 27268, 11767, 11766, 7805, 7804, 6307, 47988, 47987, 27272, 17854,
            17853, 17852, 17851, 17850, 17767, 48011, 27277, 27276, 19736, 19734, 19731, 19505, 57567, 57566, 57565,
            57564, 54424, 47983, 27269, 11771, 11770, 8317, 8316, 2947, 47964, 27267, 11763, 11762, 7802, 7801,
            7800, 7799, 3110, 47996, 30198, 30194, 30151, 47992, 47991, 27274, 11780, 11779, 11778, 7816, 7815,
            7814, 7870, 47986, 47985, 27273, 19443, 19442, 19441, 19440, 19438, 7812, 27275, 11785, 11784, 7813,
            6360, 19647, 19244, 47990, 47989, 33701, 27271, 17752, 17751, 17750, 17735, 47984, 27270, 11775, 11774,
            7811, 7810, 7809, 3716, 54053, 54052, 54051, 54050, 54049, 6358, 47206, 1010, 47834, 47833, 27285, 53756,
            31117, 34936, 47822, 27214, 11682, 11681, 5857, 47818, 47817, 42218, 42226, 42225, 42224, 42223,

            // druid
            48466, 42230, 42233, 42232, 42231, 24907, 24905, 53227, 61387, 61388, 61390, 61391, 5421, 1178, 9635,
            49376, 16979, 48566, 48565, 33983, 33982, 33876, 48564, 48563, 33987, 33986, 33878, 40120, 33943, 64801,
            48445, 48444, 44208, 44207, 44206, 44205, 44203, 33891
        };

        std::vector<int> except =
        {
            // hunter
            1515, 6991, 2641, 883, 6197, 1462, 19885,

            // rogue
            2836,

            // death knight
            3714, 53341, 53343, 54447, 53342, 53331, 54446, 43323, 53344, 70164, 62158,

            // shaman
            6196, 66842, 66843, 66844,

            // warlock
            5500, 29858,

            // druid
            5225, 20719, 62600,

            // passives
            3127, 750, 674, 8737, 198, 199, 264, 227, 2567, 5011, 200, 266, 196, 197, 201, 202, 1180
        };

        ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(handler->GetSession()->GetPlayer()->getClass());
        if (!classEntry)
            return true;
        uint32 family = classEntry->spellfamily;

        for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
        {
            SkillLineAbilityEntry const* entry = sSkillLineAbilityStore.LookupEntry(i);
            if (!entry)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(entry->spellId);
            if (!spellInfo)
                continue;

            if (std::find(ignore.begin(), ignore.end(), spellInfo->Id) != ignore.end())
                continue;
            else if (std::find(except.begin(), except.end(), spellInfo->Id) != except.end())
            {
                if (!handler->GetSession()->GetPlayer()->IsSpellFitByClassAndRace(spellInfo->Id))
                    continue;

                if (spellInfo->Id == 3714)
                {
                    if (handler->GetSession()->GetPlayer()->getClass() == CLASS_DEATH_KNIGHT)
                        handler->GetSession()->GetPlayer()->LearnSpell(spellInfo->Id, false);

                    continue;
                }

                if (spellInfo->Id == 66842 || spellInfo->Id == 66843 || spellInfo->Id == 66844)
                {
                    if (handler->GetSession()->GetPlayer()->getClass() == CLASS_SHAMAN)
                        handler->GetSession()->GetPlayer()->LearnSpell(spellInfo->Id, false);

                    continue;
                }

                handler->GetSession()->GetPlayer()->LearnSpell(spellInfo->Id, false);
            }
            else
            {
                // skip server-side/triggered spells
                if (spellInfo->SpellLevel == 0)
                    continue;

                // skip wrong class/race skills
                if (!handler->GetSession()->GetPlayer()->IsSpellFitByClassAndRace(spellInfo->Id))
                    continue;

                // skip other spell families
                if (spellInfo->SpellFamilyName != family)
                    continue;

                // skip spells with first rank learned as talent (and all talents then also)
                if (GetTalentSpellCost(spellInfo->GetFirstRankSpell()->Id) > 0)
                    continue;

                // skip broken spells
                if (!SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer(), false))
                    continue;

                handler->GetSession()->GetPlayer()->LearnSpell(spellInfo->Id, false);
            }
        }

        handler->SendSysMessage(LANG_COMMAND_LEARN_CLASS_SPELLS);
        return true;
    }

    static bool HandleLearnAllMyTalentsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint32 classMask = player->getClassMask();

        for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
            if (!talentInfo)
                continue;

            TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
            if (!talentTabInfo)
                continue;

            if ((classMask & talentTabInfo->ClassMask) == 0)
                continue;

            // search highest talent rank
            uint32 spellId = 0;
            for (int8 rank = MAX_TALENT_RANK - 1; rank >= 0; --rank)
            {
                if (talentInfo->RankID[rank] != 0)
                {
                    spellId = talentInfo->RankID[rank];
                    break;
                }
            }

            if (!spellId)                                        // ??? none spells in talent
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer(), false))
                continue;

            // learn highest rank of talent and learn all non-talent spell ranks (recursive by tree)
            player->LearnSpellHighestRank(spellId);
            player->AddTalent(spellId, player->GetActiveSpec(), true);
        }

        player->SetFreeTalentPoints(0);

        handler->SendSysMessage(LANG_COMMAND_LEARN_CLASS_TALENTS);
        return true;
    }

    static bool HandleLearnAllMyPetTalentsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        Pet* pet = player->GetPet();
        if (!pet)
        {
            handler->SendSysMessage(LANG_NO_PET_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        CreatureTemplate const* creatureInfo = pet->GetCreatureTemplate();
        if (!creatureInfo)
        {
            handler->SendSysMessage(LANG_WRONG_PET_TYPE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        CreatureFamilyEntry const* petFamily = sCreatureFamilyStore.LookupEntry(creatureInfo->family);
        if (!petFamily)
        {
            handler->SendSysMessage(LANG_WRONG_PET_TYPE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (petFamily->petTalentType < 0)                       // not hunter pet
        {
            handler->SendSysMessage(LANG_WRONG_PET_TYPE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
            if (!talentInfo)
                continue;

            TalentTabEntry const* talentTabInfo = sTalentTabStore.LookupEntry(talentInfo->TalentTab);
            if (!talentTabInfo)
                continue;

            // prevent learn talent for different family (cheating)
            if (((1 << petFamily->petTalentType) & talentTabInfo->petTalentMask) == 0)
                continue;

            // search highest talent rank
            uint32 spellId = 0;

            for (int8 rank = MAX_TALENT_RANK-1; rank >= 0; --rank)
            {
                if (talentInfo->RankID[rank] != 0)
                {
                    spellId = talentInfo->RankID[rank];
                    break;
                }
            }

            if (!spellId)                                        // ??? none spells in talent
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer(), false))
                continue;

            // learn highest rank of talent and learn all non-talent spell ranks (recursive by tree)
            pet->learnSpellHighRank(spellId);
        }

        pet->SetFreeTalentPoints(0);

        handler->SendSysMessage(LANG_COMMAND_LEARN_PET_TALENTS);
        return true;
    }

    static bool HandleLearnAllLangCommand(ChatHandler* handler, char const* /*args*/)
    {
        for (LanguageDesc const& langDesc : lang_description)
            if (uint32 langSpellId = langDesc.spell_id)
                handler->GetSession()->GetPlayer()->LearnSpell(langSpellId, false);

        handler->SendSysMessage(LANG_COMMAND_LEARN_ALL_LANG);
        return true;
    }

    static bool HandleLearnAllDefaultCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        target->LearnDefaultSkills();
        target->LearnCustomSpells();
        target->LearnQuestRewardedSpells();

        handler->PSendSysMessage(LANG_COMMAND_LEARN_ALL_DEFAULT_AND_QUEST, handler->GetNameLink(target).c_str());
        return true;
    }

    static bool HandleLearnAllCraftsCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        for (uint32 i = 0; i < sSkillLineStore.GetNumRows(); ++i)
        {
            SkillLineEntry const* skillInfo = sSkillLineStore.LookupEntry(i);
            if (!skillInfo)
                continue;

            if ((skillInfo->categoryId == SKILL_CATEGORY_PROFESSION || skillInfo->categoryId == SKILL_CATEGORY_SECONDARY) &&
                skillInfo->canLink)                             // only prof. with recipes have
            {
                HandleLearnSkillRecipesHelper(target, skillInfo->id);
            }
        }

        handler->SendSysMessage(LANG_COMMAND_LEARN_ALL_CRAFT);
        return true;
    }

    static bool HandleLearnAllRecipesCommand(ChatHandler* handler, char const* args)
    {
        //  Learns all recipes of specified profession and sets skill to max
        //  Example: .learn all_recipes enchanting

        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        if (!*args)
            return false;

        std::wstring namePart;

        if (!Utf8toWStr(args, namePart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(namePart);

        std::string name;

        SkillLineEntry const* targetSkillInfo = nullptr;
        for (uint32 i = 1; i < sSkillLineStore.GetNumRows(); ++i)
        {
            SkillLineEntry const* skillInfo = sSkillLineStore.LookupEntry(i);
            if (!skillInfo)
                continue;

            if ((skillInfo->categoryId != SKILL_CATEGORY_PROFESSION &&
                skillInfo->categoryId != SKILL_CATEGORY_SECONDARY) ||
                !skillInfo->canLink)                            // only prof with recipes have set
                continue;

            int locale = handler->GetSessionDbcLocale();
            name = skillInfo->name[locale];
            if (name.empty())
                continue;

            if (!Utf8FitTo(name, namePart))
            {
                locale = 0;
                for (; locale < TOTAL_LOCALES; ++locale)
                {
                    if (locale == handler->GetSessionDbcLocale())
                        continue;

                    name = skillInfo->name[locale];
                    if (name.empty())
                        continue;

                    if (Utf8FitTo(name, namePart))
                        break;
                }
            }

            if (locale < TOTAL_LOCALES)
            {
                targetSkillInfo = skillInfo;
                break;
            }
        }

        if (!targetSkillInfo)
            return false;

        HandleLearnSkillRecipesHelper(target, targetSkillInfo->id);

        uint16 maxLevel = target->GetPureMaxSkillValue(targetSkillInfo->id);
        target->SetSkill(targetSkillInfo->id, target->GetSkillStep(targetSkillInfo->id), maxLevel, maxLevel);
        handler->PSendSysMessage(LANG_COMMAND_LEARN_ALL_RECIPES, name.c_str());
        return true;
    }

    static void HandleLearnSkillRecipesHelper(Player* player, uint32 skillId)
    {
        uint32 classmask = player->getClassMask();

        for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
        {
            SkillLineAbilityEntry const* skillLine = sSkillLineAbilityStore.LookupEntry(j);
            if (!skillLine)
                continue;

            // wrong skill
            if (skillLine->skillId != skillId)
                continue;

            // not high rank
            if (skillLine->forward_spellid)
                continue;

            // skip racial skills
            if (skillLine->racemask != 0)
                continue;

            // skip wrong class skills
            if (skillLine->classmask && (skillLine->classmask & classmask) == 0)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(skillLine->spellId);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, player, false))
                continue;

            player->LearnSpell(skillLine->spellId, false);
        }
    }

    static bool HandleUnLearnCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId)
            return false;

        char const* allStr = strtok(nullptr, " ");
        bool allRanks = allStr ? (strncmp(allStr, "all", strlen(allStr)) == 0) : false;

        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (allRanks)
            spellId = sSpellMgr->GetFirstSpellInChain(spellId);

        if (target->HasSpell(spellId))
            target->RemoveSpell(spellId, false, !allRanks);
        else
            handler->SendSysMessage(LANG_FORGET_SPELL);

        if (GetTalentSpellCost(spellId))
            target->SendTalentsInfoData(false);

        return true;
    }
};

void AddSC_learn_commandscript()
{
    new learn_commandscript();
}
