#pragma once

#include "Options.h"

#define GFX_SX 1920
#define GFX_SY 1080

#define NUM_VOTING_BUTTONS 4

OPTION_DECLARE(int, VOTING_CHAR_WIDTH, 80);
OPTION_DECLARE(int, VOTING_CHAR_HEIGHT, 200);
OPTION_DECLARE(float, VOTING_CHAR_SCALE, 1.f);

OPTION_DECLARE(int, VOTING_CHAR1_X, 0);
OPTION_DECLARE(int, VOTING_CHAR1_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR2_X, 150);
OPTION_DECLARE(int, VOTING_CHAR2_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR3_X, 300);
OPTION_DECLARE(int, VOTING_CHAR3_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR4_X, 450);
OPTION_DECLARE(int, VOTING_CHAR4_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR5_X, 600);
OPTION_DECLARE(int, VOTING_CHAR5_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR6_X, 750);
OPTION_DECLARE(int, VOTING_CHAR6_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR7_X, 900);
OPTION_DECLARE(int, VOTING_CHAR7_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR8_X, 1050);
OPTION_DECLARE(int, VOTING_CHAR8_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR9_X, 1200);
OPTION_DECLARE(int, VOTING_CHAR9_Y, 0);

OPTION_DECLARE(int, VOTING_CHAR10_X, 1350);
OPTION_DECLARE(int, VOTING_CHAR10_Y, 0);

//

OPTION_DECLARE(int, VOTING_AGENDA_TEXTBOX_X, 70);
OPTION_DECLARE(int, VOTING_AGENDA_TEXTBOX_Y, 150);
OPTION_DECLARE(int, VOTING_AGENDA_TEXTBOX_WIDTH, 750);
OPTION_DECLARE(int, VOTING_AGENDA_TEXTBOX_HEIGHT, 280);
OPTION_DECLARE(int, VOTING_REQ_X, 110);
OPTION_DECLARE(int, VOTING_REQ_Y, 400);

//

OPTION_DECLARE(int, VOTING_ABSTAIN_X, 1620);
OPTION_DECLARE(int, VOTING_ABSTAIN_Y, 1000);
OPTION_DECLARE(int, VOTING_ABSTAIN_WIDTH, 275);
OPTION_DECLARE(int, VOTING_ABSTAIN_HEIGHT, 85);

//

OPTION_DECLARE(int, VOTING_CHAR_X, 1500);
OPTION_DECLARE(int, VOTING_CHAR_Y, 25);
OPTION_DECLARE(float, VOTING_OPTION_CHAR_SCALE, 1.f);

OPTION_DECLARE(int, VOTING_GOAL_X, 1150);
OPTION_DECLARE(int, VOTING_GOAL_Y, 280);

OPTION_DECLARE(int, VOTING_STAT_FOOD_X, 1740);
OPTION_DECLARE(int, VOTING_STAT_FOOD_Y, 420);
OPTION_DECLARE(int, VOTING_STAT_WEALTH_X, 1480);
OPTION_DECLARE(int, VOTING_STAT_WEALTH_Y, 420);
OPTION_DECLARE(int, VOTING_STAT_TECH_X, 1480);
OPTION_DECLARE(int, VOTING_STAT_TECH_Y, 560);
OPTION_DECLARE(int, VOTING_STAT_HAPPINESS_X, 1740);
OPTION_DECLARE(int, VOTING_STAT_HAPPINESS_Y, 560);

//

OPTION_DECLARE(int, VOTING_OPTION_X, 80);
OPTION_DECLARE(int, VOTING_OPTION_Y, 510);
OPTION_DECLARE(int, VOTING_OPTION_TEXT_X, 210);
OPTION_DECLARE(int, VOTING_OPTION_TEXT_Y, 510);
OPTION_DECLARE(int, VOTING_OPTION_DY, 120);
OPTION_DECLARE(int, VOTING_OPTION_BUTTON_WIDTH, 500);
OPTION_DECLARE(int, VOTING_OPTION_BUTTON_HEIGHT, 110);

OPTION_DECLARE(int, VOTING_OPTION_COST_1_X, 1012);
OPTION_DECLARE(int, VOTING_OPTION_COST_2_X, 1050);
OPTION_DECLARE(int, VOTING_OPTION_COST_Y, 556);
OPTION_DECLARE(int, VOTING_OPTION_COSTBOX_1_X, 918);
OPTION_DECLARE(int, VOTING_OPTION_COSTBOX_2_X, 1140);
OPTION_DECLARE(int, VOTING_OPTION_COSTBOX_Y, 540);

//

OPTION_DECLARE(int, COUNCIL_OPTION_X, 80);
OPTION_DECLARE(int, COUNCIL_OPTION_Y, 510);
OPTION_DECLARE(int, COUNCIL_OPTION_TEXT_X, 210);
OPTION_DECLARE(int, COUNCIL_OPTION_TEXT_Y, 510);
OPTION_DECLARE(int, COUNCIL_OPTION_DY, 120);
OPTION_DECLARE(int, COUNCIL_OPTION_BUTTON_WIDTH, 500);
OPTION_DECLARE(int, COUNCIL_OPTION_BUTTON_HEIGHT, 110);

OPTION_DECLARE(int, COUNCIL_OPTION_COST_1_X, 1012);
OPTION_DECLARE(int, COUNCIL_OPTION_COST_2_X, 1050);
OPTION_DECLARE(int, COUNCIL_OPTION_COST_Y, 556);
OPTION_DECLARE(int, COUNCIL_OPTION_COSTBOX_1_X, 918);
OPTION_DECLARE(int, COUNCIL_OPTION_COSTBOX_2_X, 1140);
OPTION_DECLARE(int, COUNCIL_OPTION_COSTBOX_Y, 540);

//

OPTION_DECLARE(int, VOTING_TIMER_X, 1590);
OPTION_DECLARE(int, VOTING_TIMER_Y, 700);
OPTION_DECLARE(int, VOTING_TIMER_SIZE, 180);

//

OPTION_DECLARE(int, VOTING_TARGET_CHAR1_X, 0);
OPTION_DECLARE(int, VOTING_TARGET_CHAR1_Y, 100);

OPTION_DECLARE(int, VOTING_TARGET_CHAR2_X, 300);
OPTION_DECLARE(int, VOTING_TARGET_CHAR2_Y, 100);

OPTION_DECLARE(int, VOTING_TARGET_CHAR3_X, 600);
OPTION_DECLARE(int, VOTING_TARGET_CHAR3_Y, 100);

OPTION_DECLARE(int, VOTING_TARGET_CHAR4_X, 900);
OPTION_DECLARE(int, VOTING_TARGET_CHAR4_Y, 100);

OPTION_DECLARE(int, VOTING_TARGET_CHAR5_X, 1200);
OPTION_DECLARE(int, VOTING_TARGET_CHAR5_Y, 100);

OPTION_DECLARE(int, VOTING_TARGET_CHAR6_X, 0);
OPTION_DECLARE(int, VOTING_TARGET_CHAR6_Y, 500);

OPTION_DECLARE(int, VOTING_TARGET_CHAR7_X, 300);
OPTION_DECLARE(int, VOTING_TARGET_CHAR7_Y, 500);

OPTION_DECLARE(int, VOTING_TARGET_CHAR8_X, 600);
OPTION_DECLARE(int, VOTING_TARGET_CHAR8_Y, 500);

OPTION_DECLARE(int, VOTING_TARGET_CHAR9_X, 900);
OPTION_DECLARE(int, VOTING_TARGET_CHAR9_Y, 500);

OPTION_DECLARE(int, VOTING_TARGET_CHAR10_X, 1200);
OPTION_DECLARE(int, VOTING_TARGET_CHAR10_Y, 500);
