#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>
#include "api/PointercrateAPI.hpp"

class DemonListLayer : public geode::Popup {
protected:
    geode::async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    cocos2d::extension::CCScrollView* m_scrollView = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCMenuItemSpriteExtra* m_nextBtn = nullptr;
    cocos2d::CCMenuItemSpriteExtra* m_prevBtn = nullptr;
    pointercrate::PageLinks m_links;
    cocos2d::CCSize m_listSize;
    cocos2d::CCPoint m_listPos;

    bool init(float width, float height);
    void fetch(std::string const& url);
    void onFetchFinished(geode::utils::web::WebResponse res);
    void populate(std::vector<pointercrate::Demon> const& demons);
    void onNext(cocos2d::CCObject*);
    void onPrev(cocos2d::CCObject*);
    void onReload(cocos2d::CCObject*);

public:
    static DemonListLayer* create();
};
