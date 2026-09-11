#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>
#include "api/PointercrateAPI.hpp"

class PlayerRankingLayer : public geode::Popup {
protected:
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_listListener;
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_ownListener;
    cocos2d::extension::CCScrollView* m_scrollView = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCLabelBMFont* m_ownRankLabel = nullptr;
    CCMenuItemSpriteExtra* m_nextBtn = nullptr;
    CCMenuItemSpriteExtra* m_prevBtn = nullptr;
    geode::TextInput* m_searchInput = nullptr;
    pointercrate::PageLinks m_links;
    cocos2d::CCSize m_listSize;
    cocos2d::CCPoint m_listPos;
    std::string m_highlightedName;

    bool init(float width, float height);
    void fetchList(std::string const& url);
    void onListFetchFinished(geode::utils::web::WebResponse res);
    void populate(std::vector<pointercrate::RankedPlayer> const& players);
    void fetchOwnRank(std::string const& name);
    void onOwnFetchFinished(geode::utils::web::WebResponse res);
    void onNext(cocos2d::CCObject*);
    void onPrev(cocos2d::CCObject*);
    void onReload(cocos2d::CCObject*);
    void onSearchOwn(cocos2d::CCObject*);

public:
    static PlayerRankingLayer* create();
};
