#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/web.hpp>
#include "api/PointercrateAPI.hpp"

class DemonListLayer : public geode::Popup<> {
protected:
    geode::EventListener<geode::utils::web::WebTask> m_listener;
    cocos2d::extension::CCScrollView* m_scrollView = nullptr;
    cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
    cocos2d::CCMenuItemSpriteExtra* m_nextBtn = nullptr;
    cocos2d::CCMenuItemSpriteExtra* m_prevBtn = nullptr;
    pointercrate::PageLinks m_links;
    cocos2d::CCSize m_listSize;
    cocos2d::CCPoint m_listPos;

    bool setup() override;
    void fetch(std::string const& url);
    void onFetchFinished(geode::utils::web::WebTask::Event* event);
    void populate(std::vector<pointercrate::Demon> const& demons);
    void onNext(cocos2d::CCObject*);
    void onPrev(cocos2d::CCObject*);
    void onReload(cocos2d::CCObject*);

public:
    static DemonListLayer* create();
};
