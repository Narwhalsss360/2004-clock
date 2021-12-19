#pragma region Defenitions
#define RECIPIENT "Franklyn"
#pragma endregion

#pragma region Headers
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <ButtonDebounce.h>
#include <Wire.h>
#pragma endregion

#pragma region Globals and Structs
void(*resetFunc) (void) = 0;

enum class MENUS
{
	CLOCK_FACE,
	BRIGHTNESS,
	SET_HOUR,
	SET_MINUTE,
	SET_SECOND,
	SET_MONTH,
	SET_DATE,
	SET_YEAR,
	SET_TIMEZONE,
	SHOW_GMT,
	USE_SHORT_DATE,
	USE_24_HOUR
};

char daysOfTheWeek[7][10] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
char months[12][10] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

struct PINS
{
	const byte button = 4,
		rotaryA = 2,
		rotaryB = 3,
		rotaryPush = 5,
		displayBacklight = 6;

};
const PROGMEM PINS pins;

enum class ROTARYSTATES
{
	IDLE,
	COUNTER_CLOCKWISE,
	CLOCKWISE
};

struct INPUTS
{
	ROTARYSTATES rotaryState = ROTARYSTATES::IDLE;
	ButtonDebounce button = ButtonDebounce(pins.button, 50);
	ButtonDebounce rotaryPush = ButtonDebounce(pins.rotaryPush, 100);
	int lastRotaryState;
	int lastButtonState;
	unsigned long buttonPressedTime;
	const unsigned int interval = 500;
	const unsigned int resetInterval = 7000;
};

INPUTS inputs;

MENUS menu = MENUS::CLOCK_FACE;

LiquidCrystal_I2C display(0x27, 20, 4);
RTC_DS3231 clock;

struct SETTINGS
{
	byte displayBrightness = 50;
	byte savedDisplayBrightness;
};

SETTINGS settings;

DateTime localTime;
DateTime GMT;
int timeZone = 0;
bool useGMT = false;
bool useShortDate = false;
bool use24Hour = false;

String nextLines[4];
String oldLines[4];

unsigned long uptime;
#pragma endregion

#pragma region Methods
bool boolToString(bool in)
{
	return (in) ? "True" : "False";
}

void setup()
{
	Wire.begin();
	pinMode(pins.button, INPUT_PULLUP);
	pinMode(pins.rotaryA, INPUT_PULLUP);
	pinMode(pins.rotaryB, INPUT_PULLUP);
	pinMode(pins.rotaryPush, INPUT_PULLUP);
	pinMode(pins.displayBacklight, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(pins.rotaryA), rotaryUpdate, CHANGE);
	display.init();
	display.setBacklight(0);
}

void rotaryUpdate()
{
	bool pinACurrent = !digitalRead(pins.rotaryA);
	if (!pinACurrent)
	{
		if (!digitalRead(pins.rotaryB))
		{
			inputs.rotaryState = ROTARYSTATES::COUNTER_CLOCKWISE;
		}
		else
		{
			inputs.rotaryState = ROTARYSTATES::CLOCKWISE;
		}
	}
}

void sendToDisplay()
{
	if ((nextLines[0] != oldLines[0]) || (nextLines[1] != oldLines[1]))
	{
		oldLines[0] = nextLines[0];
		oldLines[1] = nextLines[1];
		display.clear();
		display.setCursor(0, 0);
		display.print(nextLines[0]);

		display.setCursor(0, 1);
		display.print(nextLines[1]);

		display.setCursor(0, 2);
		display.print(nextLines[2]);

		display.setCursor(0, 3);
		display.print(nextLines[3]);
	}

	byte tempBrightness = map(settings.displayBrightness, 0, 100, 0, 255);
	analogWrite(pins.displayBacklight, settings.displayBrightness);
}

void inputRead()
{
	inputs.button.update();
	inputs.rotaryPush.update();
}

void menuSwitching()
{
	if ((inputs.button.state() == LOW) && (inputs.lastButtonState == HIGH))
	{
		inputs.lastButtonState = LOW;
		inputs.buttonPressedTime = uptime;
	}

	if ((inputs.button.state() == HIGH) && (inputs.lastButtonState == LOW))
	{
		inputs.lastButtonState = HIGH;
		if (uptime - inputs.buttonPressedTime >= inputs.interval)
		{
			if (settings.displayBrightness > 0)
			{
				settings.savedDisplayBrightness = settings.displayBrightness;
				settings.displayBrightness = 0;
			}
			else
			{
				settings.displayBrightness = settings.savedDisplayBrightness;
			}
		}
		else
		{
			menu = MENUS::CLOCK_FACE;
		}

		if (uptime - inputs.buttonPressedTime >= inputs.resetInterval)
		{
			resetFunc();
		}
	}

	if ((inputs.rotaryPush.state() == LOW) && (inputs.lastRotaryState == HIGH))
	{
		inputs.lastRotaryState = LOW;
	}

	if ((inputs.rotaryPush.state() == HIGH) && (inputs.lastRotaryState == LOW))
	{
		inputs.lastRotaryState = HIGH;
		if (menu != MENUS::USE_24_HOUR)
		{
			int temp = (int)menu;
			temp++;
			menu = (MENUS)temp;
		}
		else
		{
			menu = MENUS::BRIGHTNESS;
		}
	}

	switch (menu)
	{
	case MENUS::CLOCK_FACE:
	{
		String row1;
		String row2;
		String row3;
		String row4;

		row1 = RECIPIENT;

		row2 += (use24Hour) ? ((useGMT) ? GMT.hour() : localTime.hour()) : ((useGMT) ? GMT.twelveHour() : localTime.twelveHour());
		row2 += ":";
		row2 += localTime.minute();
		row2 += ":";
		row2 += localTime.second();
		row2 += (use24Hour) ? "" : ((localTime.isPM()) ? " PM" : " AM");

		row3 = daysOfTheWeek[(useGMT) ? GMT.dayOfTheWeek() : localTime.dayOfTheWeek()];

		if (useShortDate)
		{
			if (useGMT)
			{
				row3 = GMT.month();
				row3 += "/";
				row3 += GMT.day();
				row3 += "/";
				row3 += GMT.year();
			}
			else
			{
				row4 = localTime.month();
				row4 += "/";
				row4 += localTime.day();
				row4 += "/";
				row4 += localTime.year();
			}
		}
		else
		{
			if (useGMT)
			{
				row4 = months[GMT.month() - 1];
				row4 += " ";
				row4 += GMT.day();
				row4 += ", ";
				row4 += GMT.year();
			}
			else
			{
				row4 = months[localTime.month() - 1];
				row4 += " ";
				row4 += localTime.day();
				row4 += ", ";
				row4 += localTime.year();
			}
		}

		nextLines[0] = row1;
		nextLines[1] = row2;
		nextLines[2] = row3;
		nextLines[3] = row4;
		sendToDisplay();
		break;
	}
	case MENUS::BRIGHTNESS:
	{
		nextLines[0] = "Set Brightness";
		nextLines[1] = settings.displayBrightness;
		nextLines[1] += "%";
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			if (settings.displayBrightness < 99)
			{
				settings.displayBrightness += 2;
			}
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			if (settings.displayBrightness > 1)
			{
				settings.displayBrightness -= 2;
			}
		}

		sendToDisplay();
		break;
	}
	case MENUS::SET_HOUR:
	{
		int tempTime = localTime.hour();
		nextLines[0] = "Set Hour";
		nextLines[1] = tempTime;
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			if (tempTime < 24)
			{
				tempTime++;
			}
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			if (tempTime > 0)
			{
				tempTime--;
			}
		}
		clock.adjust(DateTime(localTime.year(), localTime.month(), localTime.day(), tempTime, localTime.minute(), localTime.second()));
		sendToDisplay();
		break;
	}
	case MENUS::SET_MINUTE:
	{
		int tempTime = localTime.minute();
		nextLines[0] = "Set Minute";
		nextLines[1] = tempTime;
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			if (tempTime < 60)
			{
				tempTime++;
			}
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			if (tempTime > 0)
			{
				tempTime--;
			}
		}
		clock.adjust(DateTime(localTime.year(), localTime.month(), localTime.day(), localTime.hour(), tempTime, localTime.second()));
		sendToDisplay();
		break;
	}
	case MENUS::SET_SECOND:
	{
		int tempTime = localTime.second();
		nextLines[0] = "Set Second";
		nextLines[1] = tempTime;
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			if (tempTime < 60)
			{
				tempTime++;
			}
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			if (tempTime > 0)
			{
				tempTime--;
			}
		}
		clock.adjust(DateTime(localTime.year(), localTime.month(), localTime.day(), localTime.hour(), localTime.minute(), tempTime));
		sendToDisplay();
		break;
	}
	case MENUS::SET_MONTH:
	{
		int tempTime = localTime.month();
		nextLines[0] = "Set Month";
		nextLines[1] = tempTime;
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			if (tempTime < 12)
			{
				tempTime++;
			}
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			if (tempTime > 1)
			{
				tempTime--;
			}
		}
		clock.adjust(DateTime(localTime.year(), tempTime, localTime.day(), localTime.hour(), localTime.minute(), localTime.second()));
		sendToDisplay();
		break;
	}
	case MENUS::SET_DATE:
	{
		int tempTime = localTime.day();
		nextLines[0] = "Set Date";
		nextLines[1] = tempTime;
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			if (tempTime < 31)
			{
				tempTime++;
			}
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			if (tempTime > 1)
			{
				tempTime--;
			}
		}
		clock.adjust(DateTime(localTime.year(), localTime.month(), tempTime, localTime.hour(), localTime.minute(), localTime.second()));
		sendToDisplay();
		break;
	}
	case MENUS::SET_YEAR:
	{
		int tempTime = localTime.year();
		nextLines[0] = "Set Year";
		nextLines[1] = tempTime;
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			if (tempTime < 2038)
			{
				tempTime++;
			}
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			if (tempTime > 2021)
			{
				tempTime--;
			}
		}
		clock.adjust(DateTime(tempTime, localTime.month(), localTime.day(), localTime.hour(), localTime.minute(), localTime.second()));
		sendToDisplay();
		break;
	}
	case MENUS::SET_TIMEZONE:
	{
		nextLines[0] = "Set Time Zone";
		nextLines[1] = "UTC ";
		nextLines[1] += timeZone;
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			if (timeZone < 14)
			{
				timeZone++;
			}
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			if (timeZone > -12)
			{
				timeZone--;
			}
		}

		long temp = timeZone * 3600;
		temp = localTime.unixtime() + temp;
		GMT = DateTime(temp);
		sendToDisplay();
		break;
	}
	case MENUS::SHOW_GMT:
	{
		nextLines[0] = "Show GMT";
		nextLines[1] = boolToString(useGMT);
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			useGMT = true;
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			useGMT = false;
		}

		sendToDisplay();
		break;
	}
	case MENUS::USE_SHORT_DATE:
	{
		nextLines[0] = "Use Short Date";
		nextLines[1] = boolToString(useShortDate);
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			useShortDate = true;
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			useShortDate = false;
		}

		sendToDisplay();
		break;
	}
	case MENUS::USE_24_HOUR:
	{
		nextLines[0] = "Use 24-Hour Format";
		nextLines[1] = boolToString(use24Hour);
		nextLines[2] = "";
		nextLines[3] = "";

		if (inputs.rotaryState == ROTARYSTATES::CLOCKWISE)
		{
			use24Hour = true;
		}
		if (inputs.rotaryState == ROTARYSTATES::COUNTER_CLOCKWISE)
		{
			use24Hour = false;
		}

		sendToDisplay();
		break;
	}
	default:
		break;
	}
}

void loop()
{
	uptime = millis();
	if (uptime < 1500)
	{
		settings.displayBrightness = 50;
		menu = MENUS::CLOCK_FACE;
	}
	inputRead();
	localTime = clock.now();
	menuSwitching();
	inputs.rotaryState = ROTARYSTATES::IDLE;
}
#pragma endregion