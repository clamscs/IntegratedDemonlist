#include "../IntegratedDemonlist.hpp"
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/ProfilePage.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/StringBuffer.hpp>
#include <algorithm>
#include <cstdlib>
#include <set>
#include <jasmine/hook.hpp>
#include <jasmine/setting.hpp>
#include <jasmine/web.hpp>

using namespace geode::prelude;

std::set<int> loadedDemons;

static std::string stripClanTag(const std::string& name) {
    if (name.starts_with('[')) {
        if (auto end = name.find(']'); end != std::string::npos) {
            auto start = end + 1;
            while (start < name.size() && name[start] == ' ') ++start;
            return name.substr(start);
        }
    }
    return name;
}

class $modify(IDLevelCell, LevelCell) {
    struct Fields {
        TaskHolder<web::WebResponse> m_listener;
        TaskHolder<web::WebResponse> m_profileListener;
        std::string m_verifier;
    };

    static void onModify(ModifyBase<ModifyDerive<IDLevelCell, LevelCell>>& self) {
        (void)self.setHookPriorityAfterPost("LevelCell::loadFromLevel", "hiimjustin000.level_size");
        jasmine::hook::modify(self.m_hooks, "LevelCell::loadFromLevel", "enable-rank");
    }

    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);

        m_mainLayer->removeChildByID("level-rank-label"_spr);
        m_mainLayer->removeChildByID("level-rank-icon"_spr);
        m_mainLayer->removeChildByID("level-rank-menu"_spr);

        auto platformer = level->isPlatformer();
        auto difficulty = level->m_demonDifficulty;
        if (level->m_levelType == GJLevelType::Editor || level->m_demon.value() <= 0 ||
            (!platformer && difficulty < 6) || (platformer && difficulty != 0 && difficulty < 5)) return;

        auto levelID = level->m_levelID.value();
        std::vector<int> positions;
        std::string verifier;
        for (auto& demon : platformer ? IntegratedDemonlist::pemonlist : IntegratedDemonlist::pointercrate) {
            if (demon.id == levelID) {
                positions.push_back(demon.position);
                if (!platformer && verifier.empty()) verifier = demon.verifier;
            }
        }
        if (!positions.empty()) {
            m_fields->m_verifier = verifier;
            return addRank(positions, verifier);
        }

        if (loadedDemons.contains(levelID)) return;
        loadedDemons.insert(levelID);

        m_fields->m_listener.spawn(
            web::WebRequest().get(platformer
                ? fmt::format("https://pemonlist.com/api/level/{}?version=2", levelID)
                : fmt::format("https://pointercrate.com/api/v2/demons/?level_id={}", levelID)),
            [this, levelID, levelName = std::string(level->m_levelName), platformer](
                web::WebResponse res
            ) mutable {
                if (!res.ok()) return;

                std::vector<int> positions;
                std::string verifier;
                if (platformer) {
                    auto json = res.json();
                    if (!json.isOk()) return;

                    auto position = json.unwrap().get<int>("placement");
                    if (!position.isOk()) return;

                    auto position1 = position.unwrap();
                    if (position1 > 150) return;
                    positions.push_back(position1);
                }
                else {
                    for (auto& demon : jasmine::web::getArray(res)) {
                        auto position = demon.get<int>("position");
                        if (!position.isOk()) continue;

                        positions.push_back(position.unwrap());
                        if (verifier.empty()) {
                            if (auto verifierRes = demon.get("verifier"); verifierRes.isOk()) {
                                if (auto verifierName = verifierRes.unwrap().get<std::string>("name"); verifierName.isOk()) {
                                    verifier = std::move(verifierName).unwrap();
                                }
                            }
                        }
                    }
                    if (positions.empty()) return;
                }

                auto& list = platformer ? IntegratedDemonlist::pemonlist : IntegratedDemonlist::pointercrate;
                for (auto position : positions) {
                    IDListDemon demon(levelID, position, levelName, verifier);
                    if (!std::ranges::contains(list, demon)) {
                        list.push_back(std::move(demon));
                    }
                }

                m_fields->m_verifier = verifier;
                addRank(positions, verifier);
            }
        );
    }

    void addRank(const std::vector<int>& positions, const std::string& verifier) {
        if (m_mainLayer->getChildByID("level-rank-label"_spr)) return;

        auto dailyLevel = m_level->m_dailyID.value() > 0;
        auto isWhite = dailyLevel || jasmine::setting::getValue<bool>("white-rank");
        auto platformer = m_level->isPlatformer();

        StringBuffer positionsStr;
        for (auto it = positions.begin(); it != positions.end(); ++it) {
            if (it != positions.begin()) positionsStr.append('/');
            positionsStr.append("#{}", *it);
        }
        if (platformer) positionsStr.append(" Pemonlist");
        else positionsStr.append(" Pointercrate");

        auto rlc = Loader::get()->getLoadedMod("raydeeux.revisedlevelcells");
        auto styleLabel = [&](CCLabelBMFont* label) {
            if (rlc && rlc->getSettingValue<bool>("enabled") && rlc->getSettingValue<bool>("blendingText")) {
                label->setBlendFunc({ GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA });
            }
            else if (isWhite) {
                label->setOpacity(152);
            }
            else {
                label->setColor({ 51, 51, 51 });
                label->setOpacity(200);
            }
        };

        auto scale = m_compactView ? 0.4f : 0.55f;
        auto rightX = 346.0f;
        auto bottomY = dailyLevel ? 6.0f : 1.0f;

        auto rankTextNode = CCLabelBMFont::create(positionsStr.c_str(), "chatFont.fnt");
        rankTextNode->setAnchorPoint({ 1.0f, 0.0f });
        rankTextNode->setScale(scale);
        rankTextNode->setPosition({ rightX, bottomY });
        rankTextNode->setID("level-rank-label"_spr);
        styleLabel(rankTextNode);
        m_mainLayer->addChild(rankTextNode);

        if (auto icon = CCSprite::createWithSpriteFrameName(platformer ? "difficulty_10_2_btn_001.png" : "GJ_starsIcon_001.png")) {
            icon->setAnchorPoint({ 1.0f, 0.0f });
            icon->setScale(m_compactView ? 0.5f : 0.7f);
            icon->setPosition({ rightX - rankTextNode->getScaledContentWidth() - 2.0f, bottomY });
            icon->setID("level-rank-icon"_spr);
            m_mainLayer->addChild(icon);
        }

        if (!platformer && !verifier.empty()) {
            m_fields->m_verifier = verifier;

            StringBuffer verifierStr;
            verifierStr.append("by {}", stripClanTag(verifier));
            auto verifierLabel = CCLabelBMFont::create(verifierStr.c_str(), "chatFont.fnt");
            verifierLabel->setScale(scale * 0.8f);
            styleLabel(verifierLabel);

            auto verifierButton = CCMenuItemSpriteExtra::create(verifierLabel, this, menu_selector(IDLevelCell::onViewVerifier));
            verifierButton->setID("level-rank-verifier"_spr);

            auto verifierMenu = CCMenu::create();
            verifierMenu->setID("level-rank-menu"_spr);
            verifierMenu->addChild(verifierButton);
            m_mainLayer->addChild(verifierMenu);

            verifierButton->setPosition({ rightX - verifierLabel->getScaledContentWidth() / 2.0f, bottomY + 15.0f });
        }

        if (auto levelSizeLabel = m_mainLayer->getChildByID("hiimjustin000.level_size/size-label")) {
            auto rankWidth = rankTextNode->getScaledContentWidth() + 2.0f;
            if (auto icon = dynamic_cast<CCSprite*>(m_mainLayer->getChildByID("level-rank-icon"_spr))) {
                rankWidth += icon->getScaledContentWidth();
            }
            levelSizeLabel->setPosition({
                m_compactView ? 343.0f - rankWidth : 346.0f,
                m_compactView ? 1.0f : 12.0f
            });
        }
    }

    void onViewVerifier(CCObject* sender) {
        if (m_fields->m_verifier.empty()) return;

        auto gdName = stripClanTag(m_fields->m_verifier);
        m_fields->m_profileListener.spawn(
            web::WebRequest().get(fmt::format("https://gdbrowser.com/api/profile/{}", gdName)),
            [this, gdName](web::WebResponse res) mutable {
                if (!res.ok()) return showVerifierNotFound(gdName);

                auto json = res.json();
                if (!json.isOk()) return showVerifierNotFound(gdName);

                auto value = json.unwrap();
                int accountID = 0;
                if (auto id = value.get<int>("accountID"); id.isOk()) {
                    accountID = id.unwrap();
                }
                else if (auto id = value.get<std::string>("accountID"); id.isOk()) {
                    accountID = std::atoi(id.unwrap().c_str());
                }

                if (accountID <= 0) return showVerifierNotFound(gdName);

                ProfilePage::create(accountID, false)->show();
            }
        );
    }

    void showVerifierNotFound(const std::string& name) {
        FLAlertLayer::create(
            "Couldn't Find Player",
            fmt::format("Couldn't find the GD profile of \"{}\" in the search results.", name).c_str(),
            "OK"
        )->show();
    }
};