/*
 * Replacement for TASCAM RC-10 remote control.
 *
 * Copyright 2016 Christopher Williams. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CHRISTOPHER WILLIAMS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
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

struct function {
	uint8_t pin;
	uint8_t value;
};

// map of pin numbers and values to send in response to a button
// press/release/repeat (ORed with start/repeat/end mask)
/*
 * XXX buttons are numbered 1..10 so we can return the negative value to
 * indicate that the button was released. So we have a dummy button at
 * index 0 to account for that.
 */
static const struct function functions[] = {
	{  0,  0, }, // dummy
	{ 12,  8, }, //  1 stop
	{ 14,  9, }, //  2 play
	{ 15, 11, }, //  3 record
	{ 16, 14, }, //  4 forward
	{ 17, 15, }, //  5 back
	{ 18, 24, }, //  6 mark
	{ 19, 28, }, //  7 1/2 [SOLO]
	{ 20, 29, }, //  8 3/4 [SOLO]
	{ 21, 30, }, //  9 mic input level +
	{ 22, 31, }, // 10 mic input level -
	{ 23,  0, }, // 11 turbo
};

#define TURBO_LED_PIN 13
#define TURBO_BUTTON_PIN 23

#define FIRST_REPEAT_PERIOD 100
#define REPEAT_PERIOD 100

#define TURBO_FIRST_REPEAT_PERIOD 100
#define TURBO_REPEAT_PERIOD 50

/**** END OF CONFIGURABLES ****/

#define SIZEOF_ARRAY(a) (sizeof a / sizeof a[0])
static const uint8_t numButtonInputPins = SIZEOF_ARRAY(functions);

static uint8_t firstRepeatPeriod = FIRST_REPEAT_PERIOD;
static uint8_t repeatPeriod = REPEAT_PERIOD;
static bool turboRepeat = false;

#define EACHBUTTON(b) (uint8_t b = 1; b < numButtonInputPins; ++b)

void setup()
{
	Serial.begin(9600, SERIAL_8E1);
	for EACHBUTTON(b) {
		pinMode(functions[b].pin, INPUT_PULLUP);
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
		lastRepeatTime += repeat;
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
	if (functions[b].value) {
		Serial.write(functions[b].value | START_MASK);
	} else if (functions[b].pin == TURBO_BUTTON_PIN) {
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
	if (functions[b].value) {
		Serial.write(functions[b].value | END_MASK);
	}
}

static void handleButtonRepeat(uint8_t b)
{
	if (functions[b].value) {
		Serial.write(functions[b].value | REPEAT_MASK);
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
			debouncing = true;
			debounceTime = millis();
			currentButton = -currentButton;
			return currentButton;
		}
	}
	return 0;
}

// return true if button 'b' is pressed
static bool readButtonState(uint8_t b)
{
	return digitalRead(functions[b].pin) == LOW;
}
