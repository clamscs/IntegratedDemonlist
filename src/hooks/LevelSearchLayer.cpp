#include "../classes/PCListLayer.hpp"
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

using namespace geode::prelude;

class $modify(PCDemonListLevelSearch, LevelSearchLayer) {
    bool init(int searchType) {
        if (!LevelSearchLayer::init(searchType)) return false;

        auto demonlistButtonSprite = CircleButtonSprite::createWithSprite("ID_demonBtn_001.png"_spr);
        demonlistButtonSprite->getTopNode()->setScale(1.0f);
        demonlistButtonSprite->setScale(0.8f);
        auto demonlistButton = CCMenuItemSpriteExtra::create(demonlistButtonSprite, this, menu_selector(PCDemonListLevelSearch::onDemonlist));
        demonlistButton->setID("pointercrate-demonlist-button"_spr);
        if (auto menu = getChildByID("other-filter-menu")) {
            menu->addChild(demonlistButton);
            menu->updateLayout();
        }

        return true;
    }

    void onDemonlist(CCObject* sender) {
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, PCListLayer::scene()));
    }
};