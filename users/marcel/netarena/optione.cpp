#include "framework.h"
#include "gamedefs.h"
#include "optione.h"
#include "uicommon.h"

#include "main.h" // fixme : remove. needed for escape = quit hack for now

OptioneMenu::OptioneMenu()
	: m_back(0)
	, m_audio(0)
	, m_display(0)
	, m_video(0)
	, m_menuNav(0)
	, m_buttonLegend(0)
{
	m_back = new Button(0, 0, "mainmenu-button.png", 0, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_audio = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-button.png", "menu-audio", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_display = new Button(GFX_SX/2, GFX_SY/3 + 150, "mainmenu-button.png", "menu-display", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_video = new Button(GFX_SX/2, GFX_SY/3 + 300, "mainmenu-button.png", "menu-video", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

	m_menuNav = new MenuNav();
	m_menuNav->addElem(m_audio);
	m_menuNav->addElem(m_display);
	m_menuNav->addElem(m_video);

	m_buttonLegend = new ButtonLegend();
	m_buttonLegend->addElem(ButtonLegend::kButtonId_B, m_back, "ui-legend-back");
	m_buttonLegend->addElem(ButtonLegend::kButtonId_ESCAPE, m_back, "ui-legend-back");
}

OptioneMenu::~OptioneMenu()
{
	delete m_buttonLegend;

	delete m_menuNav;

	delete m_back;
	delete m_audio;
	delete m_display;
	delete m_video;
}

void OptioneMenu::onEnter()
{
	inputLockAcquire(); // fixme : remove. needed for escape = quit hack for now
}

void OptioneMenu::onExit()
{
	inputLockRelease(); // fixme : remove. needed for escape = quit hack for now
}

bool OptioneMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	m_buttonLegend->tick(dt, UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	//

	if (m_audio && m_audio->isClicked())
	{
	}
	else if (m_display && m_display->isClicked())
	{
	}
	else if (m_video && m_video->isClicked())
	{
	}
	else if (m_back->isClicked() || gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		g_app->playSound("ui/sounds/menu-back.ogg");
		return true;
	}

	return false;
}

void OptioneMenu::draw()
{
	m_buttonLegend->draw(UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	if (m_audio)
		m_audio->draw();
	if (m_display)
		m_display->draw();
	if (m_video)
		m_video->draw();
}
