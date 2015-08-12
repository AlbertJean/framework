#include "Calc.h"
#include "FileStream.h"
#include "framework.h"
#include "StreamReader.h"
#include "uicommon.h"

#pragma optimize("", off)

MenuNavElem::MenuNavElem()
	: m_hasFocus(false)
	, m_next(0)
{
}

void MenuNavElem::getPosition(int & x, int & y) const
{
	x = 0;
	y = 0;
}

void MenuNavElem::onFocusChange(bool hasFocus, bool isAutomaticSelection)
{
	m_hasFocus = hasFocus;
}

void MenuNavElem::onSelect()
{
}

//

MenuNav::MenuNav()
	: m_first(0)
	, m_selection(0)
{
}

void MenuNav::tick(float dt)
{
	// fixme : don't hard code gamepad index. select main controller at start or let all navigate?

	// todo : check if mouse is used. if true, do mouse hover collision test and change selection

	const int gamepadIndex = 0;

	if (gamepad[gamepadIndex].wentDown(DPAD_UP) || keyboard.wentDown(SDLK_UP, true))
		moveSelection(0, -1);
	if (gamepad[gamepadIndex].wentDown(DPAD_DOWN) || keyboard.wentDown(SDLK_DOWN, true))
		moveSelection(0, +1);
	if (gamepad[gamepadIndex].wentDown(DPAD_LEFT) || keyboard.wentDown(SDLK_LEFT, true))
		moveSelection(-1, 0);
	if (gamepad[gamepadIndex].wentDown(DPAD_RIGHT) || keyboard.wentDown(SDLK_RIGHT, true))
		moveSelection(+1, 0);
	if (gamepad[gamepadIndex].wentDown(GAMEPAD_A) || keyboard.wentDown(SDLK_RETURN, false))
		handleSelect();
}

void MenuNav::addElem(MenuNavElem * elem)
{
	elem->m_next = m_first;
	m_first = elem;

	if (!m_selection)
		setSelection(elem, true);
}

void MenuNav::moveSelection(int dx, int dy)
{
	MenuNavElem * newSelection = 0;

	int minDistance = 0;

	if (!m_selection)
	{
		newSelection = m_first;
	}
	else
	{
		int oldX, oldY;
		m_selection->getPosition(oldX, oldY);

		for (MenuNavElem * elem = m_first; elem != 0; elem = elem->m_next)
		{
			if (elem != m_selection)
			{
				int newX, newY;
				elem->getPosition(newX, newY);

				int distX = newX - oldX;
				int distY = newY - oldY;

				if (dx == 0)
					distX = 0;
				else if (Calc::Sign(dx) != Calc::Sign(distX))
					continue;

				if (dy == 0)
					distY = 0;
				else if (Calc::Sign(dy) != Calc::Sign(distY))
					continue;

				const int distance = distX * distX + distY * distY;

				if (distance < minDistance || minDistance == 0)
				{
					minDistance = distance;

					newSelection = elem;
				}
			}
		}
	}

	if (newSelection)
		setSelection(newSelection, false);
}

void MenuNav::setSelection(MenuNavElem * elem, bool isAutomaticSelection)
{
	if (m_selection)
		m_selection->onFocusChange(false, isAutomaticSelection);

	m_selection = elem;

	if (m_selection)
		m_selection->onFocusChange(true, isAutomaticSelection);
}

void MenuNav::handleSelect()
{
	if (m_selection)
		m_selection->onSelect();
}

//

Button::Button(int x, int y, const char * filename, const char * localString, int textX, int textY, int textSize)
	: m_sprite(new Sprite(filename))
	, m_isMouseDown(false)
	, m_x(0)
	, m_y(0)
	, m_hasBeenSelected(false)
	, m_localString(localString)
	, m_textX(textX)
	, m_textY(textY)
	, m_textSize(textSize)
{
	setPosition(x, y);
}

Button::~Button()
{
	delete m_sprite;
	m_sprite = 0;
}

void Button::setPosition(int x, int y)
{
	m_x = x - m_sprite->getWidth() / 2;
	m_y = y - m_sprite->getHeight() / 2;
}

bool Button::isClicked()
{
	bool result = false;

	if (m_hasBeenSelected)
	{
		m_hasBeenSelected = false;
		result = true;
	}
	else
	{
		const bool isInside =
			mouse.x >= m_x &&
			mouse.y >= m_y &&
			mouse.x < m_x + m_sprite->getWidth() &&
			mouse.y < m_y + m_sprite->getHeight();
		const bool isDown = isInside && mouse.isDown(BUTTON_LEFT);
		result = isInside && !isDown && m_isMouseDown;

		if (mouse.wentDown(BUTTON_LEFT))
			m_isMouseDown = isInside;
		if (!mouse.isDown(BUTTON_LEFT))
			m_isMouseDown = false;
	}

	if (result)
		Sound("ui/button/select.ogg").play();

	return result;
}

void Button::draw()
{
	if (m_hasFocus)
		setColor(colorWhite);
	else
		setColor(255, 255, 255, 255, 63);
	m_sprite->drawEx(m_x, m_y);

	if (m_localString)
	{
		// todo : translate local string, etc

		drawText(m_x + m_textX, m_y + m_textY, m_textSize, +1.f, +1.f, "%s", getLocalString(m_localString));
	}
}

void Button::getPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

void Button::onFocusChange(bool hasFocus, bool isAutomaticSelection)
{
	MenuNavElem::onFocusChange(hasFocus, isAutomaticSelection);

	if (hasFocus && !isAutomaticSelection)
		Sound("ui/button/focus.ogg").play();
}

void Button::onSelect()
{
	m_hasBeenSelected = true;
}

//

std::map<std::string, std::string> s_localStrings;

void setLocal(const char * local)
{
	s_localStrings.clear();

	try
	{
		const std::string filename = std::string("local-") + local + ".txt";

		FileStream stream;
		stream.Open(filename.c_str(), (OpenMode)(OpenMode_Read | OpenMode_Text));
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();
		for (auto & line : lines)
		{
			auto p = line.find_first_of(':');
			if (p == std::string::npos)
				logError("invalid local string definition: %s", line.c_str());
			else
			{
				std::string key = line.substr(0, p);
				std::string value = line.substr(p + 1);

				if (s_localStrings.count(key) != 0)
					logError("duplicate local string definition: %s", line.c_str());
				else
					s_localStrings[key] = value;
			}
		}
	}
	catch (std::exception & e)
	{
		logError(e.what());
	}
}

const char * getLocalString(const char * localString)
{
	auto i = s_localStrings.find(localString);
	if (i == s_localStrings.end())
		return localString;
	else
		return i->second.c_str();
}
