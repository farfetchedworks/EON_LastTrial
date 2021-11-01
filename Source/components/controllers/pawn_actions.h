#pragma once

enum EAction
{
    // Keep this order for attacks
    ATTACK_REGULAR,
    ATTACK_REGULAR_COMBO,
    ATTACK_REGULAR_SPRINT,
    ATTACK_HEAVY,
    ATTACK_HEAVY_CHARGE,
    ATTACK_HEAVY_SPRINT,
    // ***************************
    MOVE,
    RECEIVE_DMG,
    SPRINT,
    DASH,
    DASH_STRIKE,
    PARRY,
    AREA_DELAY,
    TIME_REVERSAL,
    HEAL,
    PRAY,
    FALLING,
    DEATH,
    NUM_ACTIONS
};