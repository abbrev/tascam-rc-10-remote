/*
 * Replacement for TASCAM RC-10 remote control.
 *
 * Button mapping:
 *  1 stop
 *  2 play
 *  3 record
 *  4 forward
 *  5 back
 *  6 mark
 *  7 1/2 [SOLO]
 *  8 3/4 [SOLO]
 *  9 mic input level +
 * 10 mic input level -
 * 11 turbo
 *
 * (The actual pin mapping is currently up in the air.)
 *
 * Connector: 2.5mm stereo phone plug.
 *    _
 *   <_>    tip: data (from remote control)
 *   |_|   ring: 3.3VDC (to remote control)
 *   | | sleeve: gnd
 *  _|_|_
 *  |   |
 */

/*
 * N.B. This program is limited to one button press at a time because I
 * don't know how the TASCAM DR-40 (or other models) responds to interleaved
 * buttons, so I just avoid it in the first place. There's no function that
 * requires two or more simultaneous button presses anyway.
 */

/**** START OF CONFIGURABLES ****/

/*
 * XXX buttons are number 1..10 so we can return the negative value to
 * indicate that the button was released. So we have a dummy button at
 * index 0 to account for that.
 */
// TODO assign all ten (+1) button pins here
static const uint8_t buttonInputPins[] = {
	 0, // dummy
	12, 14, 15, 16, 17, 18, 19, 20, 21, 22, // function buttons
	23, // turbo button
};

#define TURBO_LED_PIN 13
#define TURBO_BUTTON_PIN 23

#define FIRST_REPEAT_PERIOD 100
#define REPEAT_PERIOD 100

#define TURBO_FIRST_REPEAT_PERIOD 100
#define TURBO_REPEAT_PERIOD 25

/**** END OF CONFIGURABLES ****/

// values to send in response to a button press/release/repeat (ORed with
// start/repeat/end mask)
static const uint8_t buttonBytes[] = {
	 0, // dummy
	 8,  9, 11, 14, 15, 24, 28, 29, 30, 31, // functions
};

#define SIZEOF_ARRAY(a) (sizeof a / sizeof a[0])
static const uint8_t numButtonBytes = SIZEOF_ARRAY(buttonBytes);
static const uint8_t numButtonInputPins = SIZEOF_ARRAY(buttonInputPins);

static uint8_t firstRepeatPeriod = FIRST_REPEAT_PERIOD;
static uint8_t repeatPeriod = REPEAT_PERIOD;
static bool turboRepeat = false;

#define EACHBUTTON(b) (uint8_t b = 1; b < numButtonInputPins; ++b)

void setup()
{
	Serial.begin(9600, SERIAL_8E1);
	for EACHBUTTON(b) {
		pinMode(buttonInputPins[b], INPUT_PULLUP);
	}
	pinMode(TURBO_LED_PIN, OUTPUT);
}

void loop()
{
	static unsigned long lastRepeatTime;
	static int8_t currentButton = 0;
	static unsigned long repeat;

	int8_t button = scanButtons();
	if (button) {
		lastRepeatTime = millis();
		repeat = firstRepeatPeriod;
		currentButton = button;
		if (button > 0) {
			handleButtonPress(button);
		} else {
			handleButtonRelease(-button);
		}
	} else if (currentButton > 0 &&
	           millis() - lastRepeatTime >= repeat) {
		lastRepeatTime = millis();
		repeat = repeatPeriod;
	        handleButtonRepeat(currentButton);
	}
}

/**** Button event handlers ****/

#define START_MASK  0x80
#define REPEAT_MASK 0xC0
#define END_MASK    0x00

static void handleButtonPress(uint8_t b)
{
	if (b < numButtonBytes) {
		Serial.write(buttonBytes[b] | START_MASK);
	} else if (buttonInputPins[b] == TURBO_BUTTON_PIN) {
		// toggle repeat rate
		turboRepeat ^= true;
		if (turboRepeat) {
			repeatPeriod = TURBO_REPEAT_PERIOD;
			firstRepeatPeriod = TURBO_FIRST_REPEAT_PERIOD;
			//digitalWrite(TURBO_LED_PIN, LOW);
		} else {
			repeatPeriod = REPEAT_PERIOD;
			firstRepeatPeriod = FIRST_REPEAT_PERIOD;
			//digitalWrite(TURBO_LED_PIN, HIGH);
		}
	}
}

static void handleButtonRelease(uint8_t b)
{
	if (b < numButtonBytes) {
		Serial.write(buttonBytes[b] | END_MASK);
	}
}

static void handleButtonRepeat(uint8_t b)
{
	if (b < numButtonBytes) {
		Serial.write(buttonBytes[b] | REPEAT_MASK);
	}
}

/**** Debounced button scanner ****/

#define DEBOUNCE_PERIOD 50

// return zero if no change in button presses
// return button number if button is now pressed
// return negative button number if button is now released
static int8_t scanButtons(void)
{
	static int8_t currentButton = 0;

	static bool debouncing = false;
	static unsigned long debounceTime = 0;

	if (debouncing && millis() - debounceTime < DEBOUNCE_PERIOD) {
		return 0;
	}
	debouncing = false;

	if (currentButton <= 0) {
		// no buttons currently pressed
		// scan all buttons
		for EACHBUTTON(b) {
			if (readButtonState(b)) {
				debouncing = true;
				debounceTime = millis();
				currentButton = b;
				return currentButton;
			}
		}
	} else {
		// scan only this button
		bool pressed = readButtonState(currentButton);
		if (!pressed) {
			// button is released
			currentButton = -currentButton;
			return currentButton;
		}
	}
	return 0;
}

// return true if button 'b' is pressed
static bool readButtonState(uint8_t b)
{
	return digitalRead(buttonInputPins[b]) == LOW;
}
