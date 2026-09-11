#include "PlayerRankingLayer.hpp"
#include "ui/cells/PlayerCell.hpp"

using namespace geode::prelude;
using namespace pointercrate;

static bool sameNameIgnoreCase(std::string const& a, std::string const& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    }
    return true;
}

bool PlayerRankingLayer::init(float width, float height) {
    if (!Popup::init(width, height)) return false;

    this->setTitle("Pointercrate Player Ranking");

    auto winSize = m_mainLayer->getContentSize();

    auto inputBG = CCScale9Sprite::create("square02_small.png");
    inputBG->setContentSize({ 200.f, 26.f });
    inputBG->setOpacity(120);
    inputBG->setPosition({ winSize.width / 2.f - 45.f, winSize.height - 45.f });
    m_mainLayer->addChild(inputBG);

    m_searchInput = TextInput::create(190.f, "Player name");
    m_searchInput->setPosition({ winSize.width / 2.f - 45.f, winSize.height - 45.f });
    m_mainLayer->addChild(m_searchInput);

    auto searchSpr = CCSprite::createWithSpriteFrameName("GJ_searchBtn_001.png");
    searchSpr->setScale(0.6f);
    auto searchBtn = CCMenuItemSpriteExtra::create(searchSpr, this, menu_selector(PlayerRankingLayer::onSearchOwn));
    auto searchMenu = CCMenu::createWithItem(searchBtn);
    searchMenu->setPosition({ winSize.width / 2.f + 75.f, winSize.height - 45.f });
    m_mainLayer->addChild(searchMenu);

    m_ownRankLabel = CCLabelBMFont::create("Search your name to see your rank", "chatFont.fnt");
    m_ownRankLabel->setScale(0.5f);
    m_ownRankLabel->setPosition({ winSize.width / 2.f, winSize.height - 65.f });
    m_ownRankLabel->setColor({ 255, 220, 100 });
    m_mainLayer->addChild(m_ownRankLabel);

    float listWidth = winSize.width - 40.f;
    float listHeight = winSize.height - 140.f;

    auto scrollBG = CCScale9Sprite::create("square02_small.png");
    scrollBG->setContentSize({ listWidth, listHeight });
    scrollBG->setColor({ 0, 0, 0 });
    scrollBG->setOpacity(90);
    scrollBG->setPosition({ winSize.width / 2.f, winSize.height / 2.f - 20.f });
    m_mainLayer->addChild(scrollBG);

    m_listSize = { listWidth - 10.f, listHeight - 10.f };
    m_listPos = {
        winSize.width / 2.f - m_listSize.width / 2.f,
        winSize.height / 2.f - 20.f - m_listSize.height / 2.f
    };

    m_statusLabel = CCLabelBMFont::create("Loading...", "bigFont.fnt");
    m_statusLabel->setScale(0.5f);
    m_statusLabel->setPosition({ winSize.width / 2.f, winSize.height / 2.f - 20.f });
    m_mainLayer->addChild(m_statusLabel);

    auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    prevSpr->setFlipX(true);
    m_prevBtn = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(PlayerRankingLayer::onPrev));
    m_prevBtn->setEnabled(false);
    m_prevBtn->setOpacity(100);

    auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    m_nextBtn = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(PlayerRankingLayer::onNext));
    m_nextBtn->setEnabled(false);
    m_nextBtn->setOpacity(100);

    auto reloadSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    reloadSpr->setScale(0.8f);
    auto reloadBtn = CCMenuItemSpriteExtra::create(reloadSpr, this, menu_selector(PlayerRankingLayer::onReload));

    auto navMenu = CCMenu::create();
    navMenu->setLayout(RowLayout::create()->setGap(30.f));
    navMenu->addChild(m_prevBtn);
    navMenu->addChild(reloadBtn);
    navMenu->addChild(m_nextBtn);
    navMenu->updateLayout();
    navMenu->setPosition({ winSize.width / 2.f, 25.f });
    m_mainLayer->addChild(navMenu);

    fetchList(playersRankingUrl());

    return true;
}

void PlayerRankingLayer::fetchList(std::string const& url) {
    m_statusLabel->setVisible(true);
    m_statusLabel->setString("Loading...");
    if (m_scrollView) m_scrollView->setVisible(false);

    auto req = web::WebRequest();
    req.userAgent("IntegratedPointercrate (Geode Mod)");
    m_listListener.spawn(req.get(url), [this](web::WebResponse res) {
        onListFetchFinished(res);
    });
}

void PlayerRankingLayer::onListFetchFinished(web::WebResponse res) {
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
    std::vector<RankedPlayer> players;
    if (json.isArray()) {
        for (auto& item : json) {
            players.push_back(RankedPlayer::fromJson(item));
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

    populate(players);
}

void PlayerRankingLayer::populate(std::vector<RankedPlayer> const& players) {
    m_statusLabel->setVisible(false);

    if (m_scrollView) {
        m_scrollView->removeFromParent();
        m_scrollView = nullptr;
    }

    float cellHeight = 36.f;
    float totalHeight = std::max(m_listSize.height, cellHeight * players.size());

    auto container = CCLayer::create();
    container->setContentSize({ m_listSize.width, totalHeight });

    if (players.empty()) {
        auto emptyLabel = CCLabelBMFont::create("No players found", "bigFont.fnt");
        emptyLabel->setScale(0.5f);
        emptyLabel->setPosition({ m_listSize.width / 2.f, totalHeight / 2.f });
        container->addChild(emptyLabel);
    }

    for (size_t i = 0; i < players.size(); i++) {
        bool highlight = !m_highlightedName.empty() && sameNameIgnoreCase(players[i].name, m_highlightedName);
        auto cell = PlayerCell::create(players[i], m_listSize.width, highlight);
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

void PlayerRankingLayer::fetchOwnRank(std::string const& name) {
    m_ownRankLabel->setString("Searching...");
    m_highlightedName = name;

    auto req = web::WebRequest();
    req.userAgent("IntegratedPointercrate (Geode Mod)");
    m_ownListener.spawn(req.get(playerByNameUrl(name)), [this](web::WebResponse res) {
        onOwnFetchFinished(res);
    });
}

void PlayerRankingLayer::onOwnFetchFinished(web::WebResponse res) {
    if (!res.ok()) {
        m_ownRankLabel->setString("Could not find that player");
        return;
    }

    auto jsonResult = res.json();
    if (jsonResult.isErr()) {
        m_ownRankLabel->setString("Could not find that player");
        return;
    }

    auto json = jsonResult.unwrap();
    if (!json.isArray() || json.size() == 0) {
        m_ownRankLabel->setString("Could not find that player");
        return;
    }

    auto player = RankedPlayer::fromJson(json[0]);
    m_ownRankLabel->setString(fmt::format(
        "Your rank: #{}  |  Score: {:.2f}", player.rank, player.score
    ).c_str());
}

void PlayerRankingLayer::onNext(CCObject*) {
    if (m_links.next) fetchList(m_links.next.value());
}

void PlayerRankingLayer::onPrev(CCObject*) {
    if (m_links.prev) fetchList(m_links.prev.value());
}

void PlayerRankingLayer::onReload(CCObject*) {
    fetchList(playersRankingUrl());
}

void PlayerRankingLayer::onSearchOwn(CCObject*) {
    auto name = m_searchInput->getString();
    if (!name.empty()) fetchOwnRank(name);
}

PlayerRankingLayer* PlayerRankingLayer::create() {
    auto ret = new PlayerRankingLayer();
    if (ret->init(400.f, 280.f)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
