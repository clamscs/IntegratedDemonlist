#include "../IntegratedPointercrate.hpp"
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/binding/ProfilePage.hpp>
#include <Geode/binding/GJUserScore.hpp>
#include <Geode/utils/cocos.hpp>
#include <string>

using namespace geode::prelude;

class $modify(PCPProfilePage, ProfilePage) {
    struct Fields {
        geode::async::TaskHolder<web::WebResponse> m_listener;
        cocos2d::CCLabelBMFont* m_rankLabel = nullptr;
        std::string m_lastRequested;
    };

    void loadPageFromUserInfo(GJUserScore* score) {
        ProfilePage::loadPageFromUserInfo(score);

        if (!score) return;
        auto name = std::string(score->m_userName);
        if (name.empty() || name == m_fields->m_lastRequested) return;
        m_fields->m_lastRequested = name;

        if (!m_fields->m_rankLabel) {
            auto label = CCLabelBMFont::create("", "goldFont.fnt");
            label->setScale(0.5f);
            if (auto uname = m_usernameLabel) {
                label->setPosition({ uname->getPositionX(), uname->getPositionY() - 42.0f });
            }
            else {
                auto winSize = CCDirector::get()->getWinSize();
                label->setPosition({ winSize.width / 2.0f, winSize.height - 140.0f });
            }
            label->setVisible(false);
            label->setID("pc-rank-label"_spr);
            m_fields->m_rankLabel = label;
            addChild(label);
        }

        IntegratedPointercrate::loadPlayerRankByName(m_fields->m_listener, name,
            [this, self = Ref<PCPProfilePage>(this)](PCRankedPlayer player) {
                if (player.rank > 0 && m_fields->m_rankLabel) {
                    m_fields->m_rankLabel->setString(fmt::format("Pointercrate Rank: #{}", player.rank).c_str());
                    m_fields->m_rankLabel->setVisible(true);
                }
            },
            [this, self = Ref<PCPProfilePage>(this)](int) {
                if (m_fields->m_rankLabel) m_fields->m_rankLabel->setVisible(false);
            });
    }
};