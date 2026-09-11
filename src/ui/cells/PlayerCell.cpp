#include "PlayerCell.hpp"

using namespace geode::prelude;

bool PlayerCell::init(pointercrate::RankedPlayer const& player, float width, bool highlight) {
    auto bg = highlight ? ccc4(255, 255, 255, 40) : ccc4(0, 0, 0, 0);
    if (!CCLayerColor::initWithColor(bg, width, 36.f)) return false;

    auto rankLabel = CCLabelBMFont::create(fmt::format("#{}", player.rank).c_str(), "bigFont.fnt");
    rankLabel->setScale(0.4f);
    rankLabel->setAnchorPoint({ 0.f, 0.5f });
    rankLabel->setPosition({ 8.f, 18.f });
    addChild(rankLabel);

    auto nameLabel = CCLabelBMFont::create(player.name.c_str(), "bigFont.fnt");
    nameLabel->setScale(0.45f);
    nameLabel->setAnchorPoint({ 0.f, 0.5f });
    nameLabel->setPosition({ 50.f, 18.f });
    if (nameLabel->getScaledContentSize().width > 150.f) {
        nameLabel->setScale(150.f / nameLabel->getContentSize().width);
    }
    addChild(nameLabel);

    std::string nationText = player.nationality.has ? player.nationality.nation : "-";
    auto natLabel = CCLabelBMFont::create(nationText.c_str(), "chatFont.fnt");
    natLabel->setScale(0.4f);
    natLabel->setAnchorPoint({ 1.f, 0.5f });
    natLabel->setPosition({ width - 90.f, 18.f });
    addChild(natLabel);

    auto scoreLabel = CCLabelBMFont::create(fmt::format("{:.2f}", player.score).c_str(), "bigFont.fnt");
    scoreLabel->setScale(0.4f);
    scoreLabel->setAnchorPoint({ 1.f, 0.5f });
    scoreLabel->setColor({ 255, 220, 100 });
    scoreLabel->setPosition({ width - 8.f, 18.f });
    addChild(scoreLabel);

    return true;
}

PlayerCell* PlayerCell::create(pointercrate::RankedPlayer const& player, float width, bool highlight) {
    auto ret = new PlayerCell();
    if (ret->init(player, width, highlight)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
