#include "DemonListLayer.hpp"
#include "ui/cells/DemonCell.hpp"

using namespace geode::prelude;
using namespace pointercrate;

bool DemonListLayer::init(float width, float height) {
    if (!Popup::init(width, height)) return false;

    this->setTitle("Pointercrate Demon List");
    m_noElasticity = true;

    auto winSize = m_mainLayer->getContentSize();
    float listWidth = winSize.width - 40.f;
    float listHeight = winSize.height - 90.f;

    auto scrollBG = CCScale9Sprite::create("square02_small.png");
    scrollBG->setContentSize({ listWidth, listHeight });
    scrollBG->setColor({ 0, 0, 0 });
    scrollBG->setOpacity(90);
    scrollBG->setPosition({ winSize.width / 2.f, winSize.height / 2.f + 5.f });
    m_mainLayer->addChild(scrollBG);

    m_listSize = { listWidth - 10.f, listHeight - 10.f };
    m_listPos = {
        winSize.width / 2.f - m_listSize.width / 2.f,
        winSize.height / 2.f + 5.f - m_listSize.height / 2.f
    };

    m_statusLabel = CCLabelBMFont::create("Loading...", "bigFont.fnt");
    m_statusLabel->setScale(0.5f);
    m_statusLabel->setPosition({ winSize.width / 2.f, winSize.height / 2.f + 5.f });
    m_mainLayer->addChild(m_statusLabel);

    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    prevSpr->setFlipX(true);
    m_prevBtn = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(DemonListLayer::onPrev));
    m_prevBtn->setEnabled(false);
    m_prevBtn->setOpacity(100);

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    m_nextBtn = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(DemonListLayer::onNext));
    m_nextBtn->setEnabled(false);
    m_nextBtn->setOpacity(100);

    auto reloadSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    reloadSpr->setScale(0.8f);
    auto reloadBtn = CCMenuItemSpriteExtra::create(reloadSpr, this, menu_selector(DemonListLayer::onReload));

    auto navMenu = CCMenu::create();
    navMenu->setLayout(RowLayout::create()->setGap(30.f));
    navMenu->addChild(m_prevBtn);
    navMenu->addChild(reloadBtn);
    navMenu->addChild(m_nextBtn);
    navMenu->updateLayout();
    navMenu->setPosition({ winSize.width / 2.f, 25.f });
    m_mainLayer->addChild(navMenu);

    fetch(demonsListedUrl());

    return true;
}

void DemonListLayer::fetch(std::string const& url) {
    m_statusLabel->setVisible(true);
    m_statusLabel->setString("Loading...");
    if (m_scrollView) m_scrollView->setVisible(false);

    auto req = web::WebRequest();
    req.userAgent("IntegratedPointercrate (Geode Mod)");
    m_listener.spawn(req.get(url), [this](web::WebResponse res) {
        onFetchFinished(res);
    });
}

void DemonListLayer::onFetchFinished(web::WebResponse res) {
    if (!res.ok()) {
        m_statusLabel->setString(fmt::format("Failed to load (HTTP {})", res.code()).c_str());
        return;
    }

    auto jsonResult = res.json();
    if (jsonResult.isErr()) {
        m_statusLabel->setString("Failed to parse response");
        return;
    }

    auto json = jsonResult.unwrap();
    std::vector<Demon> demons;
    if (json.isArray()) {
        for (auto& item : json) {
            demons.push_back(Demon::fromJson(item));
        }
    }

    std::optional<std::string> linkHeader;
    if (auto header = res.header("Link")) {
        linkHeader = std::string(*header);
    }
    m_links = PageLinks::fromHeader(linkHeader);
    m_prevBtn->setEnabled(m_links.prev.has_value());
    m_prevBtn->setOpacity(m_links.prev.has_value() ? 255 : 100);
    m_nextBtn->setEnabled(m_links.next.has_value());
    m_nextBtn->setOpacity(m_links.next.has_value() ? 255 : 100);

    populate(demons);
}

void DemonListLayer::populate(std::vector<Demon> const& demons) {
    m_statusLabel->setVisible(false);

    if (m_scrollView) {
        m_scrollView->removeFromParent();
        m_scrollView = nullptr;
    }

    float cellHeight = 40.f;
    float totalHeight = std::max(m_listSize.height, cellHeight * demons.size());

    auto container = CCLayer::create();
    container->setContentSize({ m_listSize.width, totalHeight });

    if (demons.empty()) {
        auto emptyLabel = CCLabelBMFont::create("No demons found", "bigFont.fnt");
        emptyLabel->setScale(0.5f);
        emptyLabel->setPosition({ m_listSize.width / 2.f, totalHeight / 2.f });
        container->addChild(emptyLabel);
    }

    for (size_t i = 0; i < demons.size(); i++) {
        auto cell = DemonCell::create(demons[i], m_listSize.width);
        cell->setPosition({ 0.f, totalHeight - cellHeight * (i + 1) });
        container->addChild(cell);
    }

    m_scrollView = CCScrollView::create(m_listSize, container);
    m_scrollView->setDirection(kCCScrollViewDirectionVertical);
    m_scrollView->setTouchEnabled(true);
    m_scrollView->setPosition(m_listPos);
    m_scrollView->setContentOffset({ 0.f, m_listSize.height - totalHeight });
    m_mainLayer->addChild(m_scrollView);
}

void DemonListLayer::onNext(CCObject*) {
    if (m_links.next) fetch(m_links.next.value());
}

void DemonListLayer::onPrev(CCObject*) {
    if (m_links.prev) fetch(m_links.prev.value());
}

void DemonListLayer::onReload(CCObject*) {
    fetch(demonsListedUrl());
}

DemonListLayer* DemonListLayer::create() {
    auto ret = new DemonListLayer();
    if (ret->init(360.f, 260.f)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
