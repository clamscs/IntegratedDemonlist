#include "../IntegratedPointercrate.hpp"
#include <Geode/modify/LeaderboardsLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <Geode/ui/ListView.hpp>
#include <Geode/utils/string.hpp>
#include <algorithm>
#include <cmath>

using namespace geode::prelude;

class PCPRow : public cocos2d::CCNode {
public:
    static PCPRow* create(int rank, std::string const& name, double score, bool isYou, float width) {
        auto ret = new PCPRow();
        if (ret->init(rank, name, score, isYou, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
protected:
    bool init(int rank, std::string const& name, double score, bool isYou, float width) {
        if (!CCNode::init()) return false;

        this->setContentSize({ width, 64.0f });

        auto rankLabel = CCLabelBMFont::create(fmt::format("#{}", rank).c_str(), "bigFont.fnt");
        rankLabel->setAnchorPoint({ 0.0f, 0.5f });
        rankLabel->setPosition({ 12.0f, 32.0f });
        rankLabel->setScale(0.55f);
        rankLabel->setColor({ 255, 255, 255 });
        if (rank <= 3) rankLabel->setColor({ 255, 220, 0 });
        addChild(rankLabel);

        auto nameLabel = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
        nameLabel->setAnchorPoint({ 0.0f, 0.5f });
        nameLabel->setPosition({ width * 0.22f, 32.0f });
        nameLabel->setScale(0.5f);
        nameLabel->limitLabelWidth(width - 220.0f, 0.5f, 0.0f);
        if (isYou) {
            nameLabel->setColor({ 255, 255, 0 });
            nameLabel->setString(fmt::format("{}  (you)", name).c_str());
            nameLabel->limitLabelWidth(width - 220.0f, 0.5f, 0.0f);
        }
        addChild(nameLabel);

        auto scoreLabel = CCLabelBMFont::create(formatScore(score).c_str(), "goldFont.fnt");
        scoreLabel->setAnchorPoint({ 1.0f, 0.5f });
        scoreLabel->setPosition({ width - 12.0f, 32.0f });
        scoreLabel->setScale(0.55f);
        addChild(scoreLabel);

        return true;
    }
private:
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
        geode::async::TaskHolder<web::WebResponse> m_listener;
        geode::async::TaskHolder<web::WebResponse> m_rankListener;
        LeaderboardType m_lastType = LeaderboardType::Default;
        bool m_pcEnabled = false;
        int m_ownRankValue = 0;
        bool m_ownRankChecked = false;
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

    void onPointercrate(CCObject* sender) {
        if (m_fields->m_pcEnabled) {
            disarmPointercrate();
            selectLeaderboard(m_fields->m_lastType, m_stat);
        }
        else {
            m_fields->m_pcEnabled = true;
            m_fields->m_lastType = m_type;
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
        if (m_fields->m_yourRankLabel) {
            m_fields->m_yourRankLabel->setVisible(false);
        }
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
            m_fields->m_yourRankLabel->setPosition({
                pos.x,
                pos.y - size.height / 2.0f - 15.0f
            });
            m_fields->m_yourRankLabel->setID("your-rank-label"_spr);
            addChild(m_fields->m_yourRankLabel);
        }
        m_fields->m_yourRankLabel->setVisible(false);

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
        if (!m_fields->m_pcList) return;
        if (m_fields->m_loadingCircle) m_fields->m_loadingCircle->setVisible(false);

        auto cells = CCArray::create();

        auto own = toTaglessLower(std::string(GameManager::get()->m_playerName));
        auto size = m_fields->m_pcList->getContentSize();
        for (auto& player : IntegratedPointercrate::players) {
            bool isYou = !own.empty() && toTaglessLower(player.name) == own;
            cells->addObject(PCPRow::create(player.rank, player.name, player.score, isYou, size.width));
        }

        if (auto listView = m_fields->m_pcList->m_listView) {
            listView->removeFromParent();
            listView->release();
        }
        auto listView = geode::ui::ListView::create(cells, 64.0f, size.width, size.height - 30.0f);
        listView->retain();
        m_fields->m_pcList->addChild(listView, 6, 9);
        m_fields->m_pcList->m_listView = listView;

        updateYourRankLabel();
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