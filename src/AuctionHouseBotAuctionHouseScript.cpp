/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>,
 * released under GNU AGPL v3 license:
 * https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE
 */

#include "AuctionHouseMgr.h"
#include "GameTime.h"

#include "AuctionHouseBot.h"
#include "AuctionHouseBotCommon.h"
#include "AuctionHouseBotAuctionHouseScript.h"

AHBot_AuctionHouseScript::AHBot_AuctionHouseScript() : AuctionHouseScript("AHBot_AuctionHouseScript")
{

}

void AHBot_AuctionHouseScript::OnBeforeAuctionHouseMgrSendAuctionSuccessfulMail(
    AuctionHouseMgr*,                /*auctionHouseMgr*/
    AuctionEntry*,                   /*auction*/
    Player* owner,
    uint32&,                         /*owner_accId*/
    uint32&,                         /*profit*/
    bool& sendNotification,
    bool& updateAchievementCriteria,
    bool&                            /*sendMail*/)
{
    if (owner && gBotsId.find(owner->GetGUID().GetCounter()) != gBotsId.end())
    {
        sendNotification          = false;
        updateAchievementCriteria = false;
    }
}

void AHBot_AuctionHouseScript::OnBeforeAuctionHouseMgrSendAuctionExpiredMail(
    AuctionHouseMgr*,       /* auctionHouseMgr */
    AuctionEntry*,          /* auction */
    Player* owner,
    uint32&,                /* owner_accId */
    bool& sendNotification,
    bool&                   /* sendMail */)
{
    if (owner && gBotsId.find(owner->GetGUID().GetCounter()) != gBotsId.end())
    {
        sendNotification = false;
    }
}

void AHBot_AuctionHouseScript::OnBeforeAuctionHouseMgrSendAuctionOutbiddedMail(
    AuctionHouseMgr*,      /* auctionHouseMgr */
    AuctionEntry* auction,
    Player* oldBidder,
    uint32&,               /* oldBidder_accId */
    Player* newBidder,
    uint32& newPrice,
    bool&,                 /* sendNotification */
    bool&                  /* sendMail */)
{
    if (oldBidder && !newBidder)
    {
        if (gBotsId.size() > 0)
        {
            //
            // Use a random bot id
            //
            uint32 randBot = urand(0, gBotsId.size() - 1);
            std::set<uint32>::iterator it = gBotsId.begin();
            std::advance(it, randBot);

            oldBidder->GetSession()->SendAuctionBidderNotification(
                (uint32)auction->GetHouseId(),
                auction->Id,
                ObjectGuid::Create<HighGuid::Player>(*it),
                newPrice,
                auction->GetAuctionOutBid(),
                auction->item_template);
        }
    }
}

void AHBot_AuctionHouseScript::OnAuctionAdd(AuctionHouseObject* /*ah*/, AuctionEntry* auction)
{
    //
    // The configuration for the auction house
    //
    AuctionHouseEntry const* ahEntry = sAuctionMgr->GetAuctionHouseEntryFromHouse(auction->GetHouseId());
    AHBConfig*               config  = gNeutralConfig;

    if (ahEntry)
    {
        // Changed here
        if (AuctionHouses(ahEntry->houseId) == AUCTIONHOUSE_ALLIANCE)
        {
            config = gAllianceConfig;
        }
        // Changed here
        else if (AuctionHouses(ahEntry->houseId) == AUCTIONHOUSE_HORDE)
        {
            config = gHordeConfig;
        }
    }

    //
    // Consider only auctions NOT owned by a bot if configured that way
    //
    if (config->ConsiderOnlyBotAuctions)
    {
        if (gBotsId.find(auction->owner.GetCounter()) != gBotsId.end())
        {
            return;
        }
    }

    //
    // Verify if we can operate on the item
    //
    Item* pItem = sAuctionMgr->GetAItem(auction->item_guid);
    if (!pItem)
    {
        if (config->DebugOut)
        {
            LOG_ERROR("module", "AHBot: Item {} doesn't exist, perhaps bought already?", auction->item_guid.ToString());
        }
        return;
    }

    //
    // Keep updated the amount of items in the auction
    //
    ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(auction->item_template);

    if (config->DebugOut)
    {
        LOG_INFO("module", "AHBot: ah={}, item={}, count={}",
                 auction->GetHouseId(), auction->item_template, config->GetItemCounts(prototype->Quality));
    }

    config->IncItemCounts(prototype->Class, prototype->Quality);
}

void AHBot_AuctionHouseScript::OnAuctionRemove(AuctionHouseObject* /*ah*/, AuctionEntry* auction)
{
    //
    // Get the configuration for the auction house
    //
    AuctionHouseEntry const* ahEntry = sAuctionMgr->GetAuctionHouseEntryFromHouse(auction->GetHouseId());
    AHBConfig*               config  = gNeutralConfig;

    if (ahEntry)
    {
        // Changed here
        if (AuctionHouses(ahEntry->houseId) == AUCTIONHOUSE_ALLIANCE)
        {
            config = gAllianceConfig;
        }
        // Changed here
        else if (AuctionHouses(ahEntry->houseId) == AUCTIONHOUSE_HORDE)
        {
            config = gHordeConfig;
        }
    }

    //
    // Consider only auctions NOT owned by a bot if configured that way
    //
    if (config->ConsiderOnlyBotAuctions)
    {
        if (gBotsId.find(auction->owner.GetCounter()) != gBotsId.end())
        {
            return;
        }
    }

    //
    // Verify if we can operate on the item
    //
    Item* pItem = sAuctionMgr->GetAItem(auction->item_guid);
    if (!pItem)
    {
        if (config->DebugOut)
        {
            LOG_ERROR("module", "AHBot: Item {} doesn't exist, perhaps bought already?", auction->item_guid.ToString());
        }
        return;
    }

    //
    // Decrement the item count
    //
    ItemTemplate const* prototype = sObjectMgr->GetItemTemplate(auction->item_template);

    if (config->DebugOut)
    {
        LOG_INFO("module", "AHBot: ah={}, item={}, count={}",
                 auction->GetHouseId(), auction->item_template, config->GetItemCounts(prototype->Quality));
    }

    config->DecItemCounts(prototype->Class, prototype->Quality);
}

void AHBot_AuctionHouseScript::OnAuctionSuccessful(AuctionHouseObject* /*ah*/, AuctionEntry* auction)
{
    //
    // Get the configuration for the auction house
    //
    AuctionHouseEntry const* ahEntry = sAuctionMgr->GetAuctionHouseEntryFromHouse(auction->GetHouseId());
    AHBConfig*               config  = gNeutralConfig;

    if (ahEntry)
    {
        // Changed here
        if (AuctionHouses(ahEntry->houseId) == AUCTIONHOUSE_ALLIANCE)
        {
            config = gAllianceConfig;
        }
        // Changed here
        else if (AuctionHouses(ahEntry->houseId) == AUCTIONHOUSE_HORDE)
        {
            config = gHordeConfig;
        }
    }

    //
    // If the auction has been won, it means it was accepted by the market.
    // We'll use the buyout as a reference since the price for the bid is downgraded during selling.
    //
    config->UpdateItemStats(auction->item_template, auction->itemCount, auction->buyout);
}

void AHBot_AuctionHouseScript::OnAuctionExpire(AuctionHouseObject* /*ah*/, AuctionEntry* auction)
{
    //
    // Get the configuration for the auction house
    //
    AuctionHouseEntry const* ahEntry = sAuctionMgr->GetAuctionHouseEntryFromHouse(auction->GetHouseId());
    AHBConfig*               config  = gNeutralConfig;

    if (ahEntry)
    {
        // Changed here
        if (AuctionHouses(ahEntry->houseId) == AUCTIONHOUSE_ALLIANCE)
        {
            config = gAllianceConfig;
        }
        // Changed here
        else if (AuctionHouses(ahEntry->houseId) == AUCTIONHOUSE_HORDE)
        {
            config = gHordeConfig;
        }
    }

    //
    // If the auction expired, the bid was unwanted by the market.
    // We use the final bid as a reference to lower the item price next time.
    //
    config->UpdateItemStats(auction->item_template, auction->itemCount, auction->bid);
}

void AHBot_AuctionHouseScript::OnBeforeAuctionHouseMgrUpdate()
{
    //
    // For every registered bot, perform an update
    //
    for (AuctionHouseBot* bot: gBots)
    {
        bot->Update();
    }
}
