#define ADEBUG

enum SvetoforStates { InitState = 0, GreenIsOn = 1, GreenIsEnding = 2, YellowIsOn = 3, RedIsOn = 6, SleepIsOn, SleepIsOff };

enum SvetoforModes { ManualMode = 0, AutomaticMode, AutomaticModeSlow, SleepMode };

byte sfMode = 0;

byte sfState = 0;
bool buttonWasPressed = false;

// How many ticks (to switch states in automatic mode)
int ticks = 0;
int tickTimeMs = 100;
//#ifdef ADEBUG
//    int tickTimeMs = 400;
//#else
//    int tickTimeMs = 200;
//#endif

// To handle double click
unsigned long lastButtonPressed = 0;
unsigned long doubleClickTolleranceMs = 700;

// Svetofor light delays
const byte greenTimeTicks = 50;
const byte greenFlashes = 5;    // How many times will turn on and off
const byte yellowTimeTicks = 10;
const byte redTimeTicks = 40;
const byte sleepTimeTicks = 50; // LEDs are turned off in sleep mode

// If mode is AutomaticModeSlow it will shift delay (<<)
byte delayMultiplier = 0;

//const byte greenTimeTicks = 3;
//const byte greenFlashes = 1;    // How many times will turn on and off
//const byte yellowTimeTicks = 2;
//const byte redTimeTicks = 3;


#ifdef __AVR_ATtiny85__
    const int greenLightPin = 0;
    const int yellowLightPin = 1;
    const int redLightPin = 2;
    const int buttonPin = 4;
    const int modeButtonPin = 5;
#else
    const int greenLightPin = 10;
    const int yellowLightPin = 11;
    const int redLightPin = 12;
    const int buttonPin = 6;
    const int modeButtonPin = 7;
#endif


int oldButtonState = LOW;
int oldModeButtonState = LOW;

void setup() {
#ifdef ADEBUG
    Serial.begin(9600);

    Serial.print("Button PIN #");
    Serial.println(buttonPin);
    Serial.print("Mode button PIN #");
    Serial.println(modeButtonPin);    
#endif

    pinMode(greenLightPin, OUTPUT);
    pinMode(yellowLightPin, OUTPUT);
    pinMode(redLightPin, OUTPUT);

    pinMode(buttonPin, INPUT);
    pinMode(modeButtonPin, INPUT);

    digitalWrite(redLightPin, HIGH);
    digitalWrite(yellowLightPin, HIGH);
    digitalWrite(greenLightPin, HIGH);

    sfState = (byte)GreenIsOn;
    sfMode = (byte)AutomaticMode;

    delay(1000);
    turnLightsOff();
}

void loop() {
    if (wasButtonPressed()) {
        log("Button is pressed");
        handleButtonClick(false);
    }

    if (wasModeButtonPressed()) {
        log("Mode switch button is pressed");
        // Mode switch button emulates double click
        handleButtonClick(true);
    }

//    Serial.print("Svetofor state is: ");
//    Serial.print(sfState);
//    Serial.print(". Ticks: ");
//    Serial.print(ticks);
//    Serial.print(". Mode is: ");
//    Serial.println(sfMode);
    switch (sfState) {
        case (byte)SleepIsOn:
            if (ticks % 2 == 1) {
                digitalWrite(greenLightPin, HIGH);
                digitalWrite(yellowLightPin, HIGH);
                digitalWrite(redLightPin, HIGH);
            } else {
                digitalWrite(greenLightPin, LOW);
                digitalWrite(yellowLightPin, LOW);
                digitalWrite(redLightPin, LOW);
            }
            break;
        case (byte)SleepIsOff:
            if (ticks == 0) {
                turnLightsOff();
            }
            break;
        case (byte)GreenIsOn:
            turnLedOnOrOff(greenLightPin, greenTimeTicks);
            break;
        case (byte)GreenIsEnding:
            if (ticks % 2 == 1) {
                digitalWrite(greenLightPin, HIGH);
            } else {
                digitalWrite(greenLightPin, LOW);
            }
            break;
        case (byte)YellowIsOn:
            turnLedOnOrOff(yellowLightPin, yellowTimeTicks);
            break;
        case (byte)RedIsOn:
            turnLedOnOrOff(redLightPin, redTimeTicks);
            break;
        default:
            turnLightsOff();
            sfState = (byte)GreenIsOn;
            ticks = -1;
            break;
    }

    switch (sfMode) {
        case (byte)AutomaticMode:
        case (byte)AutomaticModeSlow:
            if (changeStateOnTicks(sfState) ) {
                turnLightsOff();
            } else
            {
                delay(tickTimeMs);
            }
            break;
        case (byte)ManualMode:
            if (sfState == GreenIsEnding || sfState == YellowIsOn) {
                changeStateOnTicks(sfState);
            }
            delay(tickTimeMs);
            break;
        case (byte)SleepMode:
            if (changeStateOnTicks(sfState) == false) {
                delay(tickTimeMs);
            }
            break;
        default:
            delay(tickTimeMs);
            break;
    }

    ticks++;
}


// Returns true if state was changed
bool changeStateOnTicks(byte currentState) {
    byte state = sfState;
    switch (currentState) {
        case (byte)GreenIsOn:
              if (ticks >= greenTimeTicks << delayMultiplier) {
//                  Serial.println("Changing state to GreenIsEnding");
                  ticks = -1;
                  state = (byte)GreenIsEnding;
              }
              break;
        case (byte)GreenIsEnding:
            // Green should goes LOW, HIGH,... LOW
            if (ticks >= (greenFlashes * 2) << delayMultiplier) {
                ticks = -1;
                if ((byte)ManualMode != sfMode) {
//                    Serial.println("Changing state to YellowIsOn");
                    state = (byte)YellowIsOn;
                } else {
//                    Serial.println("Changing state to RedIsOn (manual mode, skipping YellowIsOn)");
                    state = (byte)RedIsOn;
                }
            }
            break;
        case (byte)YellowIsOn:
            if (ticks >= yellowTimeTicks << delayMultiplier) {
                ticks = -1;
//                Serial.println("Changing state to RedIsOn");
                state = (byte)RedIsOn;
            }
            break;
        case (byte)RedIsOn:
            if (ticks >= redTimeTicks << delayMultiplier) {
                ticks = -1;
//                Serial.println("Changing state to GreenIsOn");
                state = (byte)GreenIsOn;
            }
            break;
        case (byte)SleepIsOn:
            if (ticks > 3) {
                ticks = -1;
                state = (byte)SleepIsOff;
            }
            break;
        case (byte)SleepIsOff:
            if (ticks > sleepTimeTicks) {
                ticks = -1;
                state = (byte)SleepIsOn;
            }
            break;
        default:
            ticks = -1;
//            Serial.println("Changing UNKNOWN state to GreenIsOn");
            state = GreenIsOn;
    }

    if (state != sfState) {
        sfState = state;
        return true;
    }
    return false;
}

void handleButtonClick(bool isDoubleClick) {
    if (isDoubleClick) {
        sfState = GreenIsOn;
        ticks = 0;
        switch (sfMode) {
            case (byte)ManualMode:
                sfMode = (byte)AutomaticMode;
                delayMultiplier = 0;
                break;
            case (byte)AutomaticMode:
                sfMode = (byte)AutomaticModeSlow;
                delayMultiplier = 1;
                break;
            case (byte)AutomaticModeSlow:
                sfMode = (byte)SleepMode;
                sfState = (byte)SleepIsOn;
                delayMultiplier = 0;
                break;
            default:
                sfMode = (byte)AutomaticMode;
//                sfMode = (byte)ManualMode;
                // Prevent long green flashing from AutomaticModeSlow
                delayMultiplier = 0;
                break;
        }
        turnLightsOff();
        return;
    }
    switch (sfMode) {
        case (byte)AutomaticMode:
//            Serial.println("Changing Automatic mode to Manual...");
            turnLightsOff();
            sfMode = (byte)ManualMode;
            sfState = GreenIsOn;
            ticks = 0;
            return;
        case (byte)ManualMode:
            ticks = 0;
            switch (sfState) {
                case (byte)GreenIsOn:
                    sfState = (byte)GreenIsEnding;
                    return;
                case (byte)GreenIsEnding:
                    digitalWrite(greenLightPin, LOW);
                    sfState = (byte)YellowIsOn;
                    return;
                case (byte)YellowIsOn:
                    digitalWrite(yellowLightPin, LOW);
                    sfState = (byte)RedIsOn;
                    return;
                case (byte)RedIsOn:
                    digitalWrite(redLightPin, LOW);
                    sfState = (byte)GreenIsOn;
                    return;
                default:
                    sfState = (byte)InitState;
                    break;
            }
            break;
    }

}

bool wasButtonPressed() {
    bool wasPressed = false;
    const int currentState = digitalRead(buttonPin);
    if (currentState != oldButtonState && currentState == HIGH) {
        wasPressed = true;
        delay(50);
    }
    oldButtonState = currentState;
    return wasPressed;
}

bool wasModeButtonPressed() {
    bool wasPressed = false;
    const int currentState = digitalRead(modeButtonPin);
    if (currentState != oldModeButtonState && currentState == HIGH) {
        wasPressed = true;
        delay(50);
    }
    oldModeButtonState = currentState;
    return wasPressed;
}

void turnLedOnOrOff(int pin, byte lengthOfSignal) {
    // Need to turn on
    if (ticks == 0) {
        digitalWrite(pin, HIGH);
        return;
    }

    if ((byte)AutomaticModeSlow == sfMode) {
        lengthOfSignal = lengthOfSignal << delayMultiplier;
    }
    
    if (ticks >= lengthOfSignal) {
        // Turn off on last tick
        if ((byte)AutomaticMode == sfMode || (byte)AutomaticModeSlow == sfMode || sfState == YellowIsOn || sfState == GreenIsEnding) {
            digitalWrite(pin, LOW);
        }
    }
}


void turnLightsOff() {
    digitalWrite(greenLightPin, LOW);
    digitalWrite(yellowLightPin, LOW);
    digitalWrite(redLightPin, LOW);
}

void log(char *msg) {
#ifdef ADEBUG
    Serial.println(msg);
#endif
}

