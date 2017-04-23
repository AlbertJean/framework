#include "colorwheel.h"
#include "framework.h"
#include "particle.h"
#include "StringEx.h"
#include "textfield.h"
#include "ui.h"

struct UiElem;
struct UiMenu;

// ui design
static const int kMenuWidth = 100;
static const int kFontSize = 12;
static const int kTextBoxHeight = 20;
static const int kTextBoxTextOffset = 150;
static const int kCheckBoxHeight = 20;
static const int kEnumHeight = 20;
static const int kEnumSelectOffset = 150;
static const int kCurveHeight = 20;
static const int kColorHeight = 20;
static const int kColorCurveHeight = 20;
static const Color kBackgroundFocusColor(0.f, 0.f, 1.f, .5f);
static const Color kBackgroundColor(0.f, 0.f, 0.f, .7f);

// ui draw state
bool g_doActions;
bool g_doDraw;
int g_drawX;
int g_drawY;

//

UiState * g_uiState = nullptr;

static GLuint checkersTexture = 0;

//

struct UiMenuStates
{
	std::map<std::string, UiMenu> menus;
};

static const int kMaxUiElemStoreDepth = 4;

static std::string g_menuStack[kMaxUiElemStoreDepth];
static int g_menuStackSize = -1;
static UiMenu * g_menu = nullptr;

//

UiState::UiState()
	: activeElem(nullptr)
	, activeColor(nullptr)
	, colorWheel(nullptr)
	, menuStates(nullptr)
{
	colorWheel = new ColorWheel();
	menuStates = new UiMenuStates();
}
	
UiState::~UiState()
{
	delete colorWheel;
	colorWheel = nullptr;
	
	delete menuStates;
	menuStates = nullptr;
}

void UiState::reset()
{
	activeElem = nullptr;
	activeColor = nullptr;
	
	menuStates->menus.clear();
}

//

void initUi()
{
	// create a checkered texture
	fassert(checkersTexture == 0);
	const uint8_t v1 = 31;
	const uint8_t v2 = 63;
	uint32_t rgba[4];
	uint32_t c1; uint32_t c2;
	uint8_t * rgba1 = (uint8_t*)&c1; uint8_t * rgba2 = (uint8_t*)&c2;
	rgba1[0] = v1; rgba1[1] = v1; rgba1[2] = v1; rgba1[3] = 255;
	rgba2[0] = v2; rgba2[1] = v2; rgba2[2] = v2; rgba2[3] = 255;
	rgba[0] = c1; rgba[1] = c2; rgba[2] = c2; rgba[3] = c1;
	checkersTexture = createTextureFromRGBA8(rgba, 2, 2, false, false);
}

void shutUi()
{
	glDeleteTextures(1, &checkersTexture);
}

void drawUiRectCheckered(float x1, float y1, float x2, float y2, float scale)
{
	gxSetTexture(checkersTexture);
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
		gxBegin(GL_QUADS);
		{
			gxTexCoord2f(0.f,               (y2 - y1) / scale); gxVertex2f(x1, y1);
			gxTexCoord2f((x2 - x1) / scale, (y2 - y1) / scale); gxVertex2f(x2, y1);
			gxTexCoord2f((x2 - x1) / scale, 0.f              ); gxVertex2f(x2, y2);
			gxTexCoord2f(0.f,               0.f              ); gxVertex2f(x1, y2);
		}
		gxEnd();
	}
	gxSetTexture(0);
}

void drawUiCircle(const float x, const float y, const float radius, const float r, const float g, const float b, const float a)
{
	hqBegin(HQ_STROKED_CIRCLES);
	{
		setColorf(0.f, 0.f, 0.f, a);
		hqStrokeCircle(x, y, radius, 4.f);
		setColorf(r, g, b, a);
		hqStrokeCircle(x, y, radius, 3.f);
	}
	hqEnd();
}

void hlsToRGB(float hue, float lum, float sat, float & r, float & g, float & b)
{
	float m2 = (lum <= .5f) ? (lum + (lum * sat)) : (lum + sat - lum * sat);
	float m1 = lum + lum - m2;

	hue = fmod(hue, 1.f) * 6.f;

	if (hue < 0.f)
	{
		hue += 6.f;
	}

	if (hue < 3.0f)
	{
		if (hue < 2.0f)
		{
			if(hue < 1.0f)
			{
				r = m2;
				g = m1 + (m2 - m1) * hue;
				b = m1;
			}
			else
			{
				r = (m1 + (m2 - m1) * (2.f - hue));
				g = m2;
				b = m1;
			}
		}
		else
		{
			r = m1;
			g = m2;
			b = (m1 + (m2 - m1) * (hue - 2.f));
		}
	}
	else
	{
		if (hue < 5.0f)
		{
			if (hue < 4.0f)
			{
				r = m1;
				g = (m1 + (m2 - m1) * (4.f - hue));
				b = m2;
			}
			else
			{
				r = (m1 + (m2 - m1) * (hue - 4.f));
				g = m1;
				b = m2;
			}
		}
		else
		{
			r = m2;
			g = m1;
			b = (m1 + (m2 - m1) * (6.f - hue));
		}
	}
}

void rgbToHSL(float r, float g, float b, float & hue, float & lum, float & sat)
{
	float max = std::max(r, std::max(g, b));
	float min = std::min(r, std::min(g, b));

	lum = (max + min) / 2.0f;

	float delta = max - min;

	if (delta < FLT_EPSILON)
	{
		sat = 0.f;
		hue = 0.f;
	}
	else
	{
		sat = (lum <= .5f) ? (delta / (max + min)) : (delta / (2.f - (max + min)));

		if (r == max)
			hue = (g - b) / delta;
		else if (g == max)
			hue = 2.f + (b - r) / delta;
		else
			hue = 4.f + (r - g) / delta;

		if (hue < 0.f)
			hue += 6.0f;

		hue /= 6.f;
	}
}

static const float kSrgbToLinear = 2.2f;
static const float kLinearToSrgb = 1.f / kSrgbToLinear;

void srgbToLinear(float r, float g, float b, float & out_r, float & out_g, float & out_b)
{
	out_r = std::powf(r, kSrgbToLinear);
	out_g = std::powf(g, kSrgbToLinear);
	out_b = std::powf(b, kSrgbToLinear);
}

void linearToSrgb(float r, float g, float b, float & out_r, float & out_g, float & out_b)
{
	out_r = std::powf(r, kLinearToSrgb);
	out_g = std::powf(g, kLinearToSrgb);
	out_b = std::powf(b, kLinearToSrgb);
}

//

struct UiElem
{
	static const int kMaxVars = 10;
	
	bool hasFocus;
	bool isActive;
	bool clicked;
	
	struct Var
	{
		union
		{
			bool boolValue;
			int intValue;
			float floatValue;
			void * pointerValue;
		};
	};
	
	Var vars[kMaxVars];
	int varMask;
	
	EditorTextField * textField; // ugh
	
	UiElem()
		: hasFocus(false)
		, isActive(false)
		, clicked(false)
		, vars()
		, varMask(0)
		, textField(nullptr)
	{
	}
	
	~UiElem()
	{
		delete textField;
		textField = nullptr;
	}
	
	void tick(const int x1, const int y1, const int x2, const int y2);
	
	void resetVars()
	{
		varMask = 0;
	}
	
	bool & getBool(const int index, const bool defaultValue)
	{
		fassert(index >= 0 && index < kMaxVars);
		if ((varMask & (1 << index)) == 0)
		{
			vars[index].boolValue = defaultValue;
			varMask |= (1 << index);
		}
		return vars[index].boolValue;
	}
	
	int & getInt(const int index, const int defaultValue)
	{
		fassert(index >= 0 && index < kMaxVars);
		if ((varMask & (1 << index)) == 0)
		{
			vars[index].intValue = defaultValue;
			varMask |= (1 << index);
		}
		return vars[index].intValue;
	}
	
	float & getFloat(const int index, const float defaultValue)
	{
		fassert(index >= 0 && index < kMaxVars);
		if ((varMask & (1 << index)) == 0)
		{
			vars[index].floatValue = defaultValue;
			varMask |= (1 << index);
		}
		return vars[index].floatValue;
	}
	
	template <typename T>
	T & getPointer(const int index, const T defaultValue)
	{
		fassert(index >= 0 && index < kMaxVars);
		if ((varMask & (1 << index)) == 0)
		{
			vars[index].pointerValue = defaultValue;
			varMask |= (1 << index);
		}
		return (T&)vars[index].pointerValue;
	}
	
	EditorTextField & getTextField()
	{
		if (textField == nullptr)
			textField = new EditorTextField();
		
		return *textField;
	}
};

//

struct UiMenu
{
	int sx;
	
	std::map<std::string, UiElem> elems;
	
	UiElem & getElem(const char * name)
	{
		return elems[name];
	}
};

//

void UiElem::tick(const int x1, const int y1, const int x2, const int y2)
{
	clicked = false;
	
	hasFocus = mouse.x >= x1 && mouse.x <= x2 && mouse.y >= y1 && mouse.y <= y2;
	if (hasFocus && mouse.wentDown(BUTTON_LEFT))
	{
		g_uiState->activeElem = this;
		clicked = true;
	}
	isActive = (g_uiState->activeElem == this);
}

//

void makeActive(UiState * state)
{
	fassert(g_menu == nullptr);
	fassert(g_menuStackSize == 0);
	
	g_uiState = state;
}

//

void pushMenu(const char * name, const int width)
{
	std::string childName;
	
	if (g_menuStackSize == -1)
	{
		childName = name;
	}
	else
	{
		const std::string & parentName = g_menuStack[g_menuStackSize];
		
		childName = parentName + "." + name;
	}
	
	g_menuStackSize++;
	
	g_menuStack[g_menuStackSize] = childName;
	
	const int previousSx = g_menu == nullptr ? kMenuWidth : g_menu->sx;
	
	g_menu = &g_uiState->menuStates->menus[g_menuStack[g_menuStackSize]];
	g_menu->sx = width ? width : previousSx;
}

void popMenu()
{
	g_menuStackSize--;
	
	if (g_menuStackSize == -1)
	{
		g_menu = nullptr;
	}
	else
	{
		g_menu = &g_uiState->menuStates->menus[g_menuStack[g_menuStackSize]];
	}
}

//

bool doButton(const char * name, const float xOffset, const float xScale, const bool lineBreak)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;

	const int x1 = g_drawX + g_menu->sx * xOffset;
	const int x2 = g_drawX + g_menu->sx * xOffset + g_menu->sx * xScale;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCheckBoxHeight;

	if (lineBreak)
		g_drawY += kCheckBoxHeight;

	bool result = false;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (elem.isActive && elem.hasFocus && mouse.wentUp(BUTTON_LEFT))
		{
			result = true;
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);
	}

	return result;
}

static bool valueToString(const std::string & src, char * dst, const int dstSize)
{
	sprintf_s(dst, dstSize, "%s", src.c_str());
	return true;
}

static bool valueToString(int src, char * dst, const int dstSize)
{
	sprintf_s(dst, dstSize, "%d", src);
	return true;
}

static bool valueToString(float src, char * dst, const int dstSize)
{
	sprintf_s(dst, dstSize, "%g", src);
	return true;
}

static bool stringToValue(const char * src, std::string & dst)
{
	dst = src;
	return true;
}

static bool stringToValue(const char * src, int & dst)
{
	dst = atoi(src);
	return true;
}

static bool stringToValue(const char * src, float & dst)
{
	dst = (float)atof(src);
	return true;
}

static void increment(char * dst, int dstSize, int direction, int & value)
{
	value += direction;
	
	valueToString(value, dst, dstSize);
}

static int countDecimals(const char * text)
{
	int numDecimals = -1;
	
	for (int i = 0; text[i]; ++i)
	{
		if (text[i] == '.')
			numDecimals = 0;
		else if (numDecimals >= 0)
			numDecimals++;
	}
	
	if (numDecimals < 0)
		numDecimals = 0;
	
	return numDecimals;
}

static void increment(char * dst, int dstSize, int direction, float & _value)
{
	const int numDecimals = countDecimals(dst);
	
	double value = std::stod(dst);
	
	value += direction / double(std::pow(10, numDecimals));
	
	const std::string s = std::to_string(value);
	
	sprintf_s(dst, dstSize, "%s", s.c_str());
	
	const int numDecimalsAfterConversion = countDecimals(dst);
	
	if (numDecimalsAfterConversion < numDecimals)
	{
		int length = strlen(dst);
		
		if (numDecimalsAfterConversion == 0)
		{
			if (length + 1 < dstSize)
			{
				dst[length++] = '.';
				dst[length] = 0;
			}
		}
		
		for (int i = 0; i < numDecimals - numDecimalsAfterConversion; ++i)
		{
			if (length + 1 < dstSize)
			{
				dst[length++] = '0';
				dst[length] = 0;
			}
		}
		
		fassert(length >= 0 && length < dstSize);
	}
	else if (numDecimalsAfterConversion > numDecimals)
	{
		int length = strlen(dst);
		
		for (int i = 0; i < numDecimalsAfterConversion - numDecimals; ++i)
			dst[--length] = 0;
		
		if (numDecimals == 0)
			dst[--length] = 0;
		
		fassert(length >= 0 && length < dstSize);
	}
}

static void increment(char * dst, int dstSize, int direction, std::string & value)
{
}

template <typename T>
static void doTextBoxImpl(T & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt)
{
	UiElem & elem = g_menu->getElem(name);
	
	enum Vars
	{
		kVar_TextfieldIsInit
	};
	
	bool & textFieldIsInit = elem.getBool(kVar_TextfieldIsInit, false);
	EditorTextField & textField = elem.getTextField();
	
	const int kPadding = 5;

	const int x1 = g_drawX + g_menu->sx * xOffset;
	const int x2 = g_drawX + g_menu->sx * xOffset + g_menu->sx * xScale;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kTextBoxHeight;

	if (lineBreak)
		g_drawY += kTextBoxHeight;

	if (g_doActions)
	{
		if (!textFieldIsInit)
		{
			textFieldIsInit = true;

			textField.open(32, false, false);

			char temp[32];
			valueToString(value, temp, sizeof(temp));
			textField.setText(temp);

			textField.close();
		}
		
		elem.tick(x1, y1, x2, y2);

		if (elem.clicked)
		{
			textField.open(32, false, false);

			char temp[32];
			valueToString(value, temp, sizeof(temp));
			textField.setText(temp);
			
			textField.setTextIsSelected(true);
		}
		
		if (!elem.isActive)
		{
			if (textField.isActive())
				textField.close();
		}

		if (textField.isActive())
		{
			textField.tick(dt);
			
			stringToValue(textField.getText(), value);
			
			int incrementDirection = 0;
			
			if (keyboard.wentDown(SDLK_DOWN, true))
				incrementDirection--;
			if (keyboard.wentDown(SDLK_UP, true))
				incrementDirection++;
			
			if (incrementDirection != 0)
			{
				char temp[1024];
				sprintf_s(temp, sizeof(temp), "%s", textField.getText());
				
				increment(temp, sizeof(temp), incrementDirection, value);
				
				textField.setText(temp);
			}
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);

		textField.draw(x1 + kPadding + kTextBoxTextOffset * xScale, y1, y2 - y1, kFontSize);
	}
}

void doTextBox(int & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt)
{
	doTextBoxImpl(value, name, xOffset, xScale, lineBreak, dt);
}

void doTextBox(float & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt)
{
	doTextBoxImpl(value, name, xOffset, xScale, lineBreak, dt);
}

void doTextBox(std::string & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt)
{
	doTextBoxImpl(value, name, xOffset, xScale, lineBreak, dt);
}

template <typename T>
static void doTextBoxImpl(T & value, const char * name, const float dt)
{
	doTextBox(value, name, 0.f, 1.f, true, dt);
}

void doTextBox(int & value, const char * name, const float dt)
{
	doTextBoxImpl(value, name, dt);
}

void doTextBox(float & value, const char * name, const float dt)
{
	doTextBoxImpl(value, name, dt);
}

void doTextBox(std::string & value, const char * name, const float dt)
{
	doTextBoxImpl(value, name, dt);
}

bool doCheckBox(bool & value, const char * name, const bool isCollapsable)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCheckBoxHeight;

	g_drawY += kCheckBoxHeight;

	elem.tick(x1, y1, x2, y2);

	const int cx1 = x1 + kPadding;
	const int cx2 = x1 + kPadding + kCheckButtonSize;
	const int cy1 = y1 + kPadding;
	const int cy2 = y2 - kPadding;

	if (g_doActions)
	{
		if (elem.isActive && elem.hasFocus && mouse.wentUp(BUTTON_LEFT))
		{
			value = !value;
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorGreen);
		if (value)
			drawRect(cx1, cy1, cx2, cy2);
		setColor(colorWhite);
		drawRectLine(cx1, cy1, cx2, cy2);

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);

		if (isCollapsable)
		{
			setColor(colorWhite);
			drawLine(
				x2 - kPadding - kCheckButtonSize,
				(y1 + y2) / 2,
				x2 - kPadding,
				(y1 + y2) / 2);

			if (!value)
			{
				drawLine(
					x2 - kPadding - kCheckButtonSize / 2,
					y1 + kPadding,
					x2 - kPadding - kCheckButtonSize / 2,
					y2 - kPadding);
			}
		}
	}

	return value;
}

void doEnumImpl(int & value, const char * name, const std::vector<EnumValue> & enumValues)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kEnumHeight;

	g_drawY += kEnumHeight;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (elem.isActive && elem.hasFocus)
		{
			// cycle prev/next

			int direction = 0;

			if (mouse.wentDown(BUTTON_LEFT))
				direction = +1;
			if (mouse.wentDown(BUTTON_RIGHT))
				direction = -1;

			if (direction != 0)
			{
				int oldIndex = -1;
				for (size_t i = 0; i < enumValues.size(); ++i)
					if (enumValues[i].value == value)
						oldIndex = i;
				if (oldIndex != -1)
					value = enumValues[(oldIndex + enumValues.size() + direction) % enumValues.size()].value;
			}
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);

		int index = -1;
		for (size_t i = 0; i < enumValues.size(); ++i)
			if (enumValues[i].value == value)
				index = i;
		if (index != -1)
			drawText(x1 + kPadding + kEnumSelectOffset, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", enumValues[index].name.c_str());
	}
}

void doParticleCurve(ParticleCurve & curve, const char * name)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCurveHeight;

	g_drawY += kCurveHeight;
    
	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		// todo : curve type selection
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);
	}
}

void doParticleColor(ParticleColor & color, const char * name)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kColorHeight;

	g_drawY += kColorHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		const bool wasActive = elem.isActive;

		elem.tick(x1, y1, x2, y2);

		if (elem.isActive && elem.hasFocus && mouse.wentDown(BUTTON_LEFT))
		{
			g_uiState->activeColor = &color;

			if (!wasActive)
			{
				g_uiState->colorWheel->fromColor(
					g_uiState->activeColor->rgba[0],
					g_uiState->activeColor->rgba[1],
					g_uiState->activeColor->rgba[2],
					g_uiState->activeColor->rgba[3]);
			}
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawUiRectCheckered(cx1, cy1, cx2, cy2, 4.f);
        setColorf(color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3]);
        drawRect(cx1, cy1, cx2, cy2);

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);
	}
}

static float screenToCurve(const int x1, const int x2, const int x, const float offset)
{
	return saturate((x - x1) / float(x2 - x1) + offset);
}

static float curveToScreen(int x1, int x2, float t)
{
	return x1 + (x2 - x1) * t;
}

static ParticleColorCurve::Key * findNearestKey(ParticleColorCurve & curve, const float t, const float maxDeviation)
{
	ParticleColorCurve::Key * nearestKey = 0;
	float nearestDistance = 0.f;

	for (int i = 0; i < curve.numKeys; ++i)
	{
		const float dt = curve.keys[i].t - t;
		const float distance = std::sqrtf(dt * dt);

		if (distance < maxDeviation && (distance < nearestDistance || nearestKey == 0))
		{
			nearestKey = &curve.keys[i];
			nearestDistance = distance;
		}
	}

	return nearestKey;
}

void doParticleColorCurve(ParticleColorCurve & curve, const char * name)
{
	pushMenu(name);
	
	UiElem & elem = g_menu->getElem("curve");
	
	enum Vars
	{
		kVar_SelectedKey,
		kVar_IsDragging,
		kVar_DragOffset
	};
	
	ParticleColorCurve::Key *& selectedKey = elem.getPointer<ParticleColorCurve::Key*>(kVar_SelectedKey, nullptr);
	bool & isDragging = elem.getBool(kVar_IsDragging, false);
	float & dragOffset = elem.getFloat(kVar_DragOffset, 0.f);
	
	const float kMaxSelectionDeviation = 5 / float(g_menu->sx);

	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kColorCurveHeight;

	g_drawY += kColorCurveHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (!elem.isActive)
			selectedKey = nullptr;
		else if (elem.hasFocus)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				// select or insert key

				const float t = screenToCurve(x1, x2, mouse.x, 0.f);
				auto key = findNearestKey(curve, t, kMaxSelectionDeviation);
				
				if (key)
				{
					g_uiState->colorWheel->fromColor(
						key->color.rgba[0],
						key->color.rgba[1],
						key->color.rgba[2],
						key->color.rgba[3]);
				}
				else
				{
					// insert a new key

					ParticleColor color;
					curve.sample(t, curve.useLinearColorSpace, color);

					if (curve.allocKey(key))
					{
						key->color = color;
						key->t = t;

						g_uiState->colorWheel->fromColor(
							key->color.rgba[0],
							key->color.rgba[1],
							key->color.rgba[2],
							key->color.rgba[3]);

						key = curve.sortKeys(key);
					}
				}

				selectedKey = key;

				if (selectedKey != nullptr)
				{
					isDragging = true;
					dragOffset = key->t - t;
				}
			}
			
			if (!mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.wentDown(BUTTON_RIGHT) || keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
				{
					selectedKey = nullptr;

					// erase key

					const float t = screenToCurve(x1, x2, mouse.x, 0.f);
					auto * key = findNearestKey(curve, t, kMaxSelectionDeviation);

					if (key)
					{
						curve.freeKey(key);
					}
				}
			}
		}

		if (isDragging)
		{
			if (elem.isActive && selectedKey != nullptr && mouse.isDown(BUTTON_LEFT))
			{
				// move selected key around

				const float t = screenToCurve(x1, x2, mouse.x, dragOffset);

				selectedKey->t = t;
				selectedKey = curve.sortKeys(selectedKey);

				fassert(selectedKey->t == t);
			}
			else
			{
				isDragging = false;
			}
		}

		if (elem.isActive)
		{
			g_uiState->activeColor = selectedKey != nullptr ? &selectedKey->color : 0;
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawUiRectCheckered(cx1, cy1, cx2, cy2, 4.f);
		for (int x = cx1; x < cx2; ++x)
		{
			const float t = (x - cx1 + .5f) / float(cx2 - cx1 - .5f);
			ParticleColor c;
			curve.sample(t, curve.useLinearColorSpace, c);
			setColorf(c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);
			drawRect(x, cy1, x + 1.f, cy2);
		}
		
		{
			const float t = screenToCurve(x1, x2, mouse.x, 0.f);
			auto key = findNearestKey(curve, t, kMaxSelectionDeviation);
			for (int i = 0; i < curve.numKeys; ++i)
			{
				const float c =
					  !elem.hasFocus ? .5f
					: (key == &curve.keys[i]) ? 1.f
					: (selectedKey == &curve.keys[i]) ? .8f
					: .5f;
				const float x = curveToScreen(x1, x2, curve.keys[i].t);
				const float y = (y1 + y2) / 2.f;
				drawUiCircle(x, y, 5.5f, c, c, c, 1.f);
			}
		}

		//setColor(colorWhite);
		//drawText(x1 + kPadding + kCheckButtonSize + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);
	}
	
	doCheckBox(curve.useLinearColorSpace, "Linear Color Space", false);
	
	popMenu();
}

void doColorWheel(ParticleColor & color, const char * name, const float dt)
{
	UiElem & elem = g_menu->getElem(name);
	
	const float wheelX = g_drawX + (g_menu->sx - g_uiState->colorWheel->getSx()) / 2;
	const float wheelY = g_drawY;

	if (g_doActions)
	{
		elem.tick(wheelX, wheelY, wheelX + g_uiState->colorWheel->getSx(), wheelY + g_uiState->colorWheel->getSy());
		if (elem.isActive)
		{
			g_uiState->colorWheel->tick(
				mouse.x - wheelX,
				mouse.y - wheelY,
				mouse.wentDown(BUTTON_LEFT),
				mouse.isDown(BUTTON_LEFT), dt); // fixme : mouseDown and dt
		}
		g_uiState->colorWheel->toColor(
			g_uiState->activeColor->rgba[0],
			g_uiState->activeColor->rgba[1],
			g_uiState->activeColor->rgba[2],
			g_uiState->activeColor->rgba[3]);
	}

	if (g_doDraw)
	{
		gxPushMatrix();
		{
			gxTranslatef(wheelX, wheelY, 0.f);
			g_uiState->colorWheel->draw();
		}
		gxPopMatrix();
	}

	g_drawY += g_uiState->colorWheel->getSy();
}