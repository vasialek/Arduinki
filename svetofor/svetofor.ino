enum SvetoforStates { InitState = 0, GreenIsOn = 1, GreenIsEnding = 2, YellowIsOn = 3, RedIsOn = 6 };

enum SvetoforModes { ManualMode = 0, AutomaticMode = 1 };

/* Modes:
 *  0: manual, on button press
 *  1: auto, after certain time
 */
byte sfMode = 0;

byte sfState = 0;
bool buttonWasPressed = false;

// How many ticks (to switch states in automatic mode)
int ticks = 0;
int tickTimeMs = 200;

// To handle double click
unsigned long lastButtonPressed = 0;
unsigned long doubleClickTolleranceMs = 500;

// Svetofor light delays
const byte greenTimeTicks = 20;
const byte greenFlashes = 5;    // How many times will turn on and off
const byte yellowTimeTicks = 5;
const byte redTimeTicks = 20;

//const byte greenTimeTicks = 3;
//const byte greenFlashes = 1;    // How many times will turn on and off
//const byte yellowTimeTicks = 2;
//const byte redTimeTicks = 3;


const int greenLightPin = 0;
const int yellowLightPin = 1;
const int redLightPin = 2;
const int buttonPin = 4;

int oldButtonState = LOW;

void setup() {
//    Serial.begin(9600);
    pinMode(greenLightPin, OUTPUT);
    pinMode(yellowLightPin, OUTPUT);
    pinMode(redLightPin, OUTPUT);

    pinMode(buttonPin, INPUT);

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
//        Serial.println("Button is pressed");
//        Serial.print("  previous was pressed before: ");
//        Serial.println(millis() - lastButtonPressed);
        if (millis() - lastButtonPressed < doubleClickTolleranceMs) {
//            Serial.println("Double clicked");
            handleButtonClick(true);
        } else {
            handleButtonClick(false);
        }
        lastButtonPressed = millis();
    }

//    Serial.print("Svetofor state is: ");
//    Serial.print(sfState);
//    Serial.print(". Ticks: ");
//    Serial.print(ticks);
//    Serial.print(". Mode is: ");
//    Serial.println(sfMode);
    switch (sfState) {
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
            if (changeStateOnTicks(sfState) == false) {
                delay(tickTimeMs);
            }
            break;
        case (byte)ManualMode:
            if (sfState == GreenIsEnding || sfState == YellowIsOn) {
                changeStateOnTicks(sfState);
            }
            delay(tickTimeMs);
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
              if (ticks == greenTimeTicks) {
//                  Serial.println("Changing state to GreenIsEnding");
                  ticks = -1;
                  state = (byte)GreenIsEnding;
              }
              break;
        case (byte)GreenIsEnding:
            // Green should goes LOW, HIGH,... LOW
            if (ticks >= greenFlashes * 2) {
                ticks = -1;
                if ((byte)AutomaticMode == sfMode) {
//                    Serial.println("Changing state to YellowIsOn");
                    state = (byte)YellowIsOn;
                } else {
//                    Serial.println("Changing state to RedIsOn (manual mode, skipping YellowIsOn)");
                    state = (byte)RedIsOn;
                }
            }
            break;
        case (byte)YellowIsOn:
            if (ticks == yellowTimeTicks) {
                ticks = -1;
//                Serial.println("Changing state to RedIsOn");
                state = (byte)RedIsOn;
            }
            break;
        case (byte)RedIsOn:
            if (ticks == redTimeTicks) {
                ticks = -1;
//                Serial.println("Changing state to GreenIsOn");
                state = (byte)GreenIsOn;
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
        sfMode = (byte)ManualMode == sfMode ? (byte)AutomaticMode : (byte)ManualMode;
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
    // Emulate button with Serial :)
//    if (Serial.available() > 0) {
//        Serial.read();
//        return true;
//    }
//    return false;
}

void turnLedOnOrOff(int pin, byte lengthOfSignal) {
    // Need to turn on
    if (ticks == 0) {
        digitalWrite(pin, HIGH);
    } else if (ticks >= lengthOfSignal) {
        // Turn off on last tick
        if ((byte)AutomaticMode == sfMode || sfState == YellowIsOn || sfState == GreenIsEnding) {
            digitalWrite(pin, LOW);
        }
    }
}


void turnLightsOff() {
    digitalWrite(greenLightPin, LOW);
    digitalWrite(yellowLightPin, LOW);
    digitalWrite(redLightPin, LOW);
}

