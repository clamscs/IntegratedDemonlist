#pragma once

#include <Geode/Geode.hpp>
#include "api/PointercrateAPI.hpp"

class PlayerCell : public cocos2d::CCLayerColor {
protected:
    bool init(pointercrate::RankedPlayer const& player, float width, bool highlight);

public:
    static PlayerCell* create(pointercrate::RankedPlayer const& player, float width, bool highlight = false);
};
