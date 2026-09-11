#include "DemonCell.hpp"

using namespace geode::prelude;

bool DemonCell::init(pointercrate::Demon const& demon, float width) {
    if (!CCLayerColor::initWithColor({ 0, 0, 0, 0 }, width, 40.f)) return false;

    m_demon = demon;

    auto posLabel = CCLabelBMFont::create(fmt::format("#{}", demon.position).c_str(), "bigFont.fnt");
    posLabel->setScale(0.45f);
    posLabel->setAnchorPoint({ 0.f, 0.5f });
    posLabel->setPosition({ 8.f, 20.f });
    addChild(posLabel);

    auto nameLabel = CCLabelBMFont::create(demon.name.c_str(), "bigFont.fnt");
    nameLabel->setScale(0.5f);
    nameLabel->setAnchorPoint({ 0.f, 0.5f });
    nameLabel->setPosition({ 0.f, 0.f });
    if (nameLabel->getScaledContentSize().width > 160.f) {
        nameLabel->setScale(160.f / nameLabel->getContentSize().width);
    }

    if (demon.levelId != 0) {
        auto nameBtn = CCMenuItemSpriteExtra::create(nameLabel, this, menu_selector(DemonCell::onName));
        m_nameBtn = nameBtn;
        auto nameMenu = CCMenu::createWithItem(nameBtn);
        nameMenu->setAnchorPoint({ 0.f, 0.5f });
        nameMenu->setPosition({ 45.f, 25.f });
        addChild(nameMenu);
    } else {
        nameLabel->setPosition({ 45.f, 25.f });
        addChild(nameLabel);
    }

    auto infoText = fmt::format(
        "Verifier: {}",
        demon.verifier.name.empty() ? "-" : demon.verifier.name
    );
    auto infoLabel = CCLabelBMFont::create(infoText.c_str(), "chatFont.fnt");
    infoLabel->setScale(0.45f);
    infoLabel->setAnchorPoint({ 0.f, 0.5f });
    infoLabel->setPosition({ 45.f, 10.f });
    addChild(infoLabel);

    if (!demon.video.empty()) {
        auto infoBtnSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        infoBtnSpr->setScale(0.7f);
        auto infoBtn = CCMenuItemSpriteExtra::create(infoBtnSpr, this, menu_selector(DemonCell::onInfo));
        auto menu = CCMenu::createWithItem(infoBtn);
        menu->setPosition({ width - 25.f, 20.f });
        addChild(menu);
    }

    return true;
}

void DemonCell::onInfo(CCObject*) {
    if (!m_demon.video.empty()) {
        web::openLinkInBrowser(m_demon.video);
    }
}

void DemonCell::onName(CCObject*) {
    if (m_demon.levelId == 0) return;

    if (m_nameBtn) m_nameBtn->setEnabled(false);

    if (!m_loadingCircle) {
        m_loadingCircle = LoadingCircle::create();
        m_loadingCircle->setParentLayer(this);
        m_loadingCircle->setPosition({ 25.f, 25.f });
        m_loadingCircle->show();
    }

    GameLevelManager::sharedState()->m_levelDownloadDelegate = this;
    GameLevelManager::sharedState()->downloadLevel(m_demon.levelId, false);
}

void DemonCell::levelDownloadFinished(GJGameLevel* level) {
    GameLevelManager::sharedState()->m_levelDownloadDelegate = nullptr;

    if (m_loadingCircle) {
        m_loadingCircle->fadeAndRemove();
        m_loadingCircle = nullptr;
    }

    if (m_nameBtn) m_nameBtn->setEnabled(true);

    CCDirector::sharedDirector()->pushScene(
        CCTransitionFade::create(0.5f, LevelInfoLayer::scene(level, false))
    );
}

void DemonCell::levelDownloadFailed(int) {
    GameLevelManager::sharedState()->m_levelDownloadDelegate = nullptr;

    if (m_nameBtn) m_nameBtn->setEnabled(true);

    if (m_loadingCircle) {
        m_loadingCircle->fadeAndRemove();
        m_loadingCircle = nullptr;
    }
}

DemonCell* DemonCell::create(pointercrate::Demon const& demon, float width) {
    auto ret = new DemonCell();
    if (ret->init(demon, width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
