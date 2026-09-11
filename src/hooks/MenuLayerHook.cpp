#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "ui/popups/DemonListLayer.hpp"
#include "ui/popups/PlayerRankingLayer.hpp"

using namespace geode::prelude;

class $modify(PointercrateMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto listSpr = CircleButtonSprite::createWithSpriteFrameName(
            "GJ_listIcon_001.png", 1.f, CircleBaseColor::Green, CircleBaseSize::Small
        );
        auto listBtn = CCMenuItemSpriteExtra::create(listSpr, this, menu_selector(PointercrateMenuLayer::onDemonList));

        auto rankSpr = CircleButtonSprite::createWithSpriteFrameName(
            "GJ_leaderboardIcon_001.png", 1.f, CircleBaseColor::Blue, CircleBaseSize::Small
        );
        auto rankBtn = CCMenuItemSpriteExtra::create(rankSpr, this, menu_selector(PointercrateMenuLayer::onPlayerRanking));

        auto menu = CCMenu::create();
        menu->setLayout(ColumnLayout::create()->setGap(8.f)->setAxisReverse(true));
        menu->addChild(listBtn);
        menu->addChild(rankBtn);
        menu->updateLayout();

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        menu->setPosition({ winSize.width - 30.f, winSize.height - 110.f });
        menu->setID("integrated-pointercrate-menu"_spr);
        this->addChild(menu);

        return true;
    }

    void onDemonList(CCObject*) {
        DemonListLayer::create()->show();
    }

    void onPlayerRanking(CCObject*) {
        PlayerRankingLayer::create()->show();
    }
};
