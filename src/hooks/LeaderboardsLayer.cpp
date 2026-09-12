#include "../IntegratedPointercrate.hpp"
#include <Geode/modify/LeaderboardsLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/binding/GJUserScore.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <Geode/binding/SimplePlayer.hpp>
#include <Geode/binding/UserInfoDelegate.hpp>
#include <Geode/ui/ListView.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/utils/string.hpp>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace geode::prelude;

namespace {
    geode::Function<void(GJUserScore*)> s_onUserInfoFinished;
    geode::CopyableFunction<void(int)> s_onUserInfoFailed;

    class GlobalUserInfoDelegate : public UserInfoDelegate {
    public:
        void getUserInfoFinished(GJUserScore* score) override {
            if (s_onUserInfoFinished) s_onUserInfoFinished(score);
        }
        void getUserInfoFailed(int id) override {
            if (s_onUserInfoFailed) s_onUserInfoFailed(id);
        }
    };
    GlobalUserInfoDelegate g_userInfoDelegate;
}

class PCPRow : public cocos2d::CCNode {
public:
    static PCPRow* create(int rank, std::string name, double score, bool isYou, float width, GJUserScore* profile = nullptr) {
        auto ret = new PCPRow();
        if (ret->init(rank, std::move(name), score, isYou, width, profile)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
protected:
    bool init(int rank, std::string name, double score, bool isYou, float width, GJUserScore* profile) {
        if (!CCNode::init()) return false;

        this->setContentSize({ width, 64.0f });

        auto rankLabel = CCLabelBMFont::create(fmt::format("{}", rank).c_str(), "bigFont.fnt");
        rankLabel->setAnchorPoint({ 0.0f, 0.5f });
        rankLabel->setPosition({ 12.0f, 47.0f });
        rankLabel->setScale(0.5f);
        if (rank <= 3) rankLabel->setColor({ 255, 220, 0 });
        addChild(rankLabel);

        if (profile) {
            makeIcon(profile);
            name = std::string(profile->m_userName);
        }
        else {
            auto placeholder = CCLabelBMFont::create("...", "chatFont.fnt");
            placeholder->setPosition({ 44.0f, 21.0f });
            placeholder->setScale(0.6f);
            placeholder->setColor({ 120, 120, 120 });
            addChild(placeholder);
        }

        auto nameLabel = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
        nameLabel->setAnchorPoint({ 0.0f, 0.5f });
        nameLabel->setPosition({ 82.0f, 45.0f });
        nameLabel->setScale(0.5f);
        nameLabel->limitLabelWidth(std::max(50.0f, width - 170.0f), 0.5f, 0.0f);
        if (isYou) {
            nameLabel->setColor({ 255, 255, 0 });
            nameLabel->setString(fmt::format("{} (You)", name).c_str());
            nameLabel->limitLabelWidth(std::max(50.0f, width - 170.0f), 0.5f, 0.0f);
        }
        addChild(nameLabel);

        auto scoreLabel = CCLabelBMFont::create(formatScore(score).c_str(), "goldFont.fnt");
        scoreLabel->setAnchorPoint({ 1.0f, 0.5f });
        scoreLabel->setPosition({ width - 12.0f, 47.0f });
        scoreLabel->setScale(0.55f);
        addChild(scoreLabel);

        if (profile) {
            auto statsLabel = CCLabelBMFont::create(
                fmt::format("{} stars   {} demons", profile->m_stars, profile->m_demons).c_str(), "chatFont.fnt"
            );
            statsLabel->setAnchorPoint({ 0.0f, 0.5f });
            statsLabel->setPosition({ 82.0f, 18.0f });
            statsLabel->setScale(0.42f);
            statsLabel->setColor({ 170, 170, 170 });
            addChild(statsLabel);
        }

        return true;
    }
private:
    void makeIcon(GJUserScore* profile) {
        auto gm = GameManager::get();
        int cube = profile->m_playerCube;
        if (cube <= 0) cube = 1;
        int color1 = profile->m_color1; if (color1 <= 0) color1 = 1;
        int color2 = profile->m_color2; if (color2 <= 0) color2 = 1;
        auto player = SimplePlayer::create(cube);
        player->setScale(0.7f);
        player->setFlipX(true);
        player->setPosition({ 44.0f, 21.0f });
        player->setColors(gm->colorForIdx(color1), gm->colorForIdx(color2));
        if (profile->m_glowEnabled && profile->m_color3 > 0) {
            auto glow = gm->colorForIdx(std::max(1, profile->m_color3));
            player->setGlowOutline(glow);
            player->enableCustomGlowColor(glow);
        }
        else {
            player->disableGlowOutline();
        }
        addChild(player);
    }

    static std::string formatScore(double score) {
        int whole = static_cast<int>(std::floor(score));
        int frac = static_cast<int>(std::lround((score - whole) * 10.0));
        if (frac >= 10) {
            whole += 1;
            frac = 0;
        }
        auto digits = fmt::format("{}", whole);
        std::string out;
        int count = 0;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
            out.push_back(*it);
            if (++count % 3 == 0 && it + 1 != digits.rend()) out.push_back(',');
        }
        std::reverse(out.begin(), out.end());
        if (frac != 0) out += fmt::format(".{}", frac);
        return out;
    }
};

static std::string toTaglessLower(std::string const& name) {
    auto s = string::toLower(name);
    auto lb = s.find('[');
    auto rb = s.find(']');
    if (lb == 0 && rb != std::string::npos && rb > lb) s = s.substr(rb + 1);
    while (!s.empty() && s.front() == ' ') s.erase(s.begin());
    return s;
}

class $modify(PCPLeaderboards, LeaderboardsLayer) {
    struct Fields {
        CCMenuItemToggler* m_pcToggle = nullptr;
        GJListLayer* m_pcList = nullptr;
        LoadingCircle* m_loadingCircle = nullptr;
        cocos2d::CCLabelBMFont* m_yourRankLabel = nullptr;
        cocos2d::CCLabelBMFont* m_pageLabel = nullptr;
        cocos2d::CCMenu* m_pageMenu = nullptr;
        CCMenuItemSpriteExtra* m_prevButton = nullptr;
        CCMenuItemSpriteExtra* m_nextButton = nullptr;
        geode::async::TaskHolder<web::WebResponse> m_listener;
        geode::async::TaskHolder<web::WebResponse> m_rankListener;
        std::unordered_map<int, Ref<GJUserScore>> m_iconCache;
        std::unordered_set<int> m_failedIcons;
        std::vector<int> m_iconQueue;
        bool m_iconRequestPending = false;
        LeaderboardType m_lastType = LeaderboardType::Default;
        bool m_pcEnabled = false;
        int m_ownRankValue = 0;
        bool m_ownRankChecked = false;
        int m_page = 0;
    };

    bool init(LeaderboardType type, LeaderboardStat stat) {
        if (!LeaderboardsLayer::init(type, stat)) return false;

        CCNode* menu = getChildByID("right-side-menu");
        if (!menu && m_modeButtons && m_modeButtons->count()) {
            if (auto btn = m_modeButtons->objectAtIndex(0)) {
                menu = static_cast<CCNode*>(btn)->getParent();
            }
        }

        if (menu) {
            auto onSprite = ButtonSprite::create("Pointercrate", "goldFont.fnt", "GJ_button_01.png", 1.0f);
            onSprite->setScale(0.55f);
            auto offSprite = ButtonSprite::create("Pointercrate", "bigFont.fnt", "GJ_button_02.png", 1.0f);
            offSprite->setScale(0.55f);
            m_fields->m_pcToggle = CCMenuItemToggler::create(offSprite, onSprite, this, menu_selector(PCPLeaderboards::onPointercrate));
            m_fields->m_pcToggle->setID("pointercrate-toggle"_spr);
            m_fields->m_pcToggle->toggle(false);
            menu->addChild(m_fields->m_pcToggle);
            menu->updateLayout();
        }

        return true;
    }

    void selectLeaderboard(LeaderboardType type, LeaderboardStat stat) {
        if (m_fields->m_pcEnabled) {
            disarmPointercrate();
        }
        LeaderboardsLayer::selectLeaderboard(type, stat);
    }

    void keyBackClicked() {
        if (m_fields->m_pcEnabled) {
            disarmPointercrate();
        }
        LeaderboardsLayer::keyBackClicked();
    }

    void onPointercrate(CCObject* sender) {
        if (m_fields->m_pcEnabled) {
            disarmPointercrate();
            selectLeaderboard(m_fields->m_lastType, m_stat);
        }
        else {
            m_fields->m_pcEnabled = true;
            m_fields->m_lastType = m_type;
            m_fields->m_page = 0;
            m_fields->m_ownRankValue = 0;
            m_fields->m_ownRankChecked = false;
            enablePointercrate();
        }
    }

    void disarmPointercrate() {
        m_fields->m_pcEnabled = false;
        if (m_fields->m_pcToggle) m_fields->m_pcToggle->toggle(false);
        if (m_fields->m_pcList) {
            m_fields->m_pcList->removeFromParent();
            m_fields->m_pcList = nullptr;
        }
        if (m_fields->m_loadingCircle) m_fields->m_loadingCircle->setVisible(false);
        if (m_fields->m_yourRankLabel) m_fields->m_yourRankLabel->setVisible(false);
        if (m_fields->m_pageMenu) m_fields->m_pageMenu->setVisible(false);
        s_onUserInfoFinished = nullptr;
        s_onUserInfoFailed = nullptr;
        m_fields->m_iconCache.clear();
        m_fields->m_failedIcons.clear();
        m_fields->m_iconQueue.clear();
        m_fields->m_iconRequestPending = false;
        m_list->setVisible(true);
    }

    void enablePointercrate() {
        auto size = m_list->getContentSize();
        auto pos = m_list->getPosition();

        m_list->setVisible(false);

        m_fields->m_pcList = GJListLayer::create(nullptr, "Pointercrate Ranking", { 0, 0, 0, 180 }, size.width, size.height, 0);
        m_fields->m_pcList->setPosition(pos);
        m_fields->m_pcList->setID("pointercrate-list"_spr);
        addChild(m_fields->m_pcList);

        if (!m_fields->m_loadingCircle) {
            m_fields->m_loadingCircle = LoadingCircle::create();
        }
        m_fields->m_loadingCircle->setParentLayer(this);
        m_fields->m_loadingCircle->show();

        if (!m_fields->m_yourRankLabel) {
            m_fields->m_yourRankLabel = CCLabelBMFont::create("", "goldFont.fnt");
            m_fields->m_yourRankLabel->setScale(0.6f);
            m_fields->m_yourRankLabel->setPosition({ pos.x + size.width / 2.0f, pos.y + size.height / 2.0f + 15.0f });
            m_fields->m_yourRankLabel->setID("your-rank-label"_spr);
            addChild(m_fields->m_yourRankLabel);
        }
        m_fields->m_yourRankLabel->setVisible(false);

        if (!m_fields->m_pageMenu) {
            m_fields->m_pageMenu = CCMenu::create();
            m_fields->m_pageMenu->setID("pc-page-menu"_spr);

            auto prevSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
            auto prevBtn = CCMenuItemSpriteExtra::create(prevSpr, this, menu_selector(PCPLeaderboards::onPrevPage));
            prevBtn->setID("pc-prev-page"_spr);
            prevBtn->setPosition({ -55.0f, 0.0f });
            m_fields->m_pageMenu->addChild(prevBtn);

            auto nextSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
            nextSpr->setFlipX(true);
            auto nextBtn = CCMenuItemSpriteExtra::create(nextSpr, this, menu_selector(PCPLeaderboards::onNextPage));
            nextBtn->setID("pc-next-page"_spr);
            nextBtn->setPosition({ 55.0f, 0.0f });
            m_fields->m_pageMenu->addChild(nextBtn);

            m_fields->m_pageLabel = CCLabelBMFont::create("", "bigFont.fnt");
            m_fields->m_pageLabel->setScale(0.5f);
            m_fields->m_pageMenu->addChild(m_fields->m_pageLabel);

            m_fields->m_prevButton = prevBtn;
            m_fields->m_nextButton = nextBtn;
        }
        m_fields->m_pageMenu->setPosition({ pos.x + size.width / 2.0f, pos.y - size.height / 2.0f - 15.0f });
        m_fields->m_pageMenu->setVisible(true);
        if (m_fields->m_pageLabel) m_fields->m_pageLabel->setString("");

        s_onUserInfoFinished = [this](GJUserScore* score) {
            if (!m_fields->m_pcEnabled) return;
            m_fields->m_iconCache[score->m_accountID] = Ref<GJUserScore>(score);
            m_fields->m_failedIcons.erase(score->m_accountID);
            m_fields->m_iconRequestPending = false;
            if (m_fields->m_pcList) buildRankingList();
            requestNextIcon();
        };
        s_onUserInfoFailed = [this](int id) {
            if (!m_fields->m_pcEnabled) return;
            m_fields->m_failedIcons.insert(id);
            m_fields->m_iconRequestPending = false;
            if (m_fields->m_pcList) buildRankingList();
            requestNextIcon();
        };
        GameLevelManager::get()->m_userInfoDelegate = &g_userInfoDelegate;

        IntegratedPointercrate::loadPlayers(m_fields->m_listener, [this] {
            if (!m_fields->m_pcEnabled) return;
            buildRankingList();
        }, [this](int code) {
            onPointercrateLoadFailed(code);
        });

        auto ownName = GameManager::get()->m_playerName;
        if (!ownName.empty()) {
            IntegratedPointercrate::loadPlayerRankByName(m_fields->m_rankListener, ownName, [this](PCRankedPlayer player) {
                m_fields->m_ownRankValue = player.rank;
                m_fields->m_ownRankChecked = true;
                updateYourRankLabel();
            }, [this](int) {
                m_fields->m_ownRankChecked = true;
                updateYourRankLabel();
            });
        }
        else {
            m_fields->m_ownRankChecked = true;
        }
    }

    void buildRankingList() {
        if (!m_fields->m_pcEnabled || !m_fields->m_pcList) return;
        if (m_fields->m_loadingCircle) m_fields->m_loadingCircle->setVisible(false);

        auto const& players = IntegratedPointercrate::players;
        auto totalPages = std::max(1, static_cast<int>((players.size() + 9) / 10));
        m_fields->m_page = std::clamp(m_fields->m_page, 0, totalPages - 1);
        auto beginIdx = static_cast<size_t>(m_fields->m_page) * 10;
        auto endIdx = std::min(players.size(), beginIdx + 10);

        auto cells = CCArray::create();
        auto own = toTaglessLower(std::string(GameManager::get()->m_playerName));
        auto size = m_fields->m_pcList->getContentSize();
        for (size_t i = beginIdx; i < endIdx; ++i) {
            auto& player = players[i];
            bool isYou = !own.empty() && toTaglessLower(player.name) == own;
            auto it = m_fields->m_iconCache.find(player.id);
            cells->addObject(PCPRow::create(player.rank, player.name, player.score, isYou, size.width,
                it != m_fields->m_iconCache.end() ? it->second : nullptr));
        }

        if (auto listView = m_fields->m_pcList->m_listView) {
            listView->removeFromParent();
            listView->release();
        }
        auto listView = ListView::create(cells, 64.0f, size.width, size.height - 30.0f);
        listView->retain();
        m_fields->m_pcList->addChild(listView, 6, 9);
        m_fields->m_pcList->m_listView = listView;

        requestIconsForPage();
        updatePageUI();
        updateYourRankLabel();
    }

    void requestIconsForPage() {
        if (!m_fields->m_pcEnabled) return;
        auto const& players = IntegratedPointercrate::players;
        auto beginIdx = static_cast<size_t>(m_fields->m_page) * 10;
        auto endIdx = std::min(players.size(), beginIdx + 10);
        m_fields->m_iconQueue.clear();
        for (size_t i = beginIdx; i < endIdx; ++i) {
            int aid = players[i].id;
            if (aid <= 0) continue;
            if (m_fields->m_iconCache.count(aid) || m_fields->m_failedIcons.count(aid)) continue;
            m_fields->m_iconQueue.push_back(aid);
        }
        requestNextIcon();
    }

    void requestNextIcon() {
        if (!m_fields->m_pcEnabled) return;
        if (m_fields->m_iconRequestPending || m_fields->m_iconQueue.empty()) return;
        auto glm = GameLevelManager::get();
        glm->m_userInfoDelegate = &g_userInfoDelegate;
        auto aid = m_fields->m_iconQueue.front();
        m_fields->m_iconQueue.erase(m_fields->m_iconQueue.begin());
        if (m_fields->m_iconCache.count(aid) || m_fields->m_failedIcons.count(aid)) {
            requestNextIcon();
            return;
        }
        m_fields->m_iconRequestPending = true;
        glm->getGJUserInfo(aid);
    }

    void updatePageUI() {
        if (!m_fields->m_pcEnabled) return;
        auto const& players = IntegratedPointercrate::players;
        auto totalPages = std::max(1, static_cast<int>((players.size() + 9) / 10));
        if (m_fields->m_pageLabel) {
            m_fields->m_pageLabel->setString(fmt::format("{} / {}", m_fields->m_page + 1, totalPages).c_str());
            m_fields->m_pageLabel->setVisible(totalPages > 1);
        }
        if (m_fields->m_prevButton) m_fields->m_prevButton->setVisible(m_fields->m_page > 0);
        if (m_fields->m_nextButton) m_fields->m_nextButton->setVisible(m_fields->m_page < totalPages - 1);
    }

    void onPrevPage(CCObject* sender) {
        if (!m_fields->m_pcEnabled || m_fields->m_page <= 0) return;
        m_fields->m_page--;
        buildRankingList();
    }

    void onNextPage(CCObject* sender) {
        if (!m_fields->m_pcEnabled) return;
        auto totalPages = std::max(1, static_cast<int>((IntegratedPointercrate::players.size() + 9) / 10));
        if (m_fields->m_page >= totalPages - 1) return;
        m_fields->m_page++;
        buildRankingList();
    }

    void updateYourRankLabel() {
        if (!m_fields->m_yourRankLabel || !m_fields->m_pcEnabled) return;
        if (m_fields->m_ownRankValue > 0) {
            m_fields->m_yourRankLabel->setString(fmt::format("Your Rank: #{}", m_fields->m_ownRankValue).c_str());
            m_fields->m_yourRankLabel->setVisible(true);
        }
        else if (m_fields->m_ownRankChecked) {
            m_fields->m_yourRankLabel->setString("You're not ranked on Pointercrate.");
            m_fields->m_yourRankLabel->setVisible(true);
        }
    }

    void onPointercrateLoadFailed(int code) {
        FLAlertLayer::create(fmt::format("Load Failed ({})", code).c_str(), "Failed to load the Pointercrate ranking. Please try again later.", "OK")->show();
        disarmPointercrate();
        LeaderboardsLayer::selectLeaderboard(m_fields->m_lastType, m_stat);
    }
};