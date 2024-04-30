#include <Servo.h>         // Include Servo library for controlling servo motor
#include <LiquidCrystal.h> // Include LiquidCrystal library for LCD display
#include <NewPing.h>       // Include NewPing library for ultrasonic sensor

// Constants declaration
const int maxWastePercentage = 96; // Maximum waste percentage before indicating bin is full
const int trigPin = 3;             // Pin for triggering ultrasonic sensor
const int echoPin = 4;             // Pin for receiving echo from ultrasonic sensor
const int servoPin = 5;            // Pin for controlling servo motor

// Variables declaration
int wastePercentage;                // Current waste percentage
float durationWaste, distanceWaste; // Duration and distance variables for waste level
float maxLevel = 23.0;              // Maximum level of waste before indicating bin is full

// LCD pins and maximum distance for ultrasonic sensor
#define wasteTrig A0     // Ultrasonic sensor trigger pin
#define wasteEcho A1     // Ultrasonic sensor echo pin
#define MAX_DISTANCE 200 // Maximum distance for ultrasonic sensor in centimeters

NewPing sonar(wasteTrig, wasteEcho, MAX_DISTANCE); // NewPing object for ultrasonic sensor
Servo servo;                                       // Servo motor object
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);            // LCD object

void setup()
{
    lcd.begin(16, 2);              // Initialize LCD with 16 columns and 2 rows
    lcd.print("...Welcome to..."); // Display welcome message
    lcd.setCursor(0, 1);           // Move cursor to the second row
    lcd.print("    Crap Bag    "); // Print device name
    Serial.begin(9600);            // Initialize serial communication
    servo.attach(servoPin);        // Attach servo motor to its pin
    DDRB |= B100000;               // Set LED pin as output
    pinMode(trigPin, OUTPUT);      // Set trigger pin of ultrasonic sensor as output
    pinMode(echoPin, INPUT);       // Set echo pin of ultrasonic sensor as input
    servo.write(0);                // Set servo motor to initial position (0 degrees)
    servo.detach();                // Detach the servo after setting initial position
    cli();                         // Disable global interrupt

    // Configure Timer2
    TCNT2 = 0;          // Reset Timer2 counter
    TCCR2A = B00000000; // Set Timer2 control register A to 0
    TCCR2B = B00000000; // Set Timer2 control register B to 0

    OCR2A = 252;         // Set compare match register for 16ns
    TCCR2A |= B00000100; // Set Timer2 to CTC mode
    TCCR2B |= B00000111; // Set prescaler of 1024
    TIMSK2 |= B00000010; // Enable Timer2 compare match interrupt
    sei();               // Enable global interrupt
}

// Measure distance from sensor to door
long measureSensorDoor()
{
    digitalWrite(trigPin, LOW);                // Set trigger pin to LOW
    delayMicroseconds(2);                      // Delay for stabilization
    digitalWrite(trigPin, HIGH);               // Set trigger pin to HIGH
    delayMicroseconds(10);                     // Delay for pulse duration
    digitalWrite(trigPin, LOW);                // Set trigger pin back to LOW
    float duration = pulseIn(echoPin, HIGH);   // Measure pulse duration
    float distdoor = (duration * 0.034) / 2.0; // Convert pulse duration to distance
    return distdoor;                           // Return distance
}

// Check door position based on waste percentage
void checkDoor(long distdoor, int wastePercentage)
{
    if (wastePercentage < maxWastePercentage)
    { // Check if waste percentage is below threshold
        if (distdoor < 30 && distdoor > 0)
        {                     // Check if door is close to sensor
            servo.write(360); // Open door (360 degrees)
            delay(1000);      // Delay for door opening
        }
        else
        {
            servo.write(0); // Close door (0 degrees)
        }
    }
}

// Measure distance of waste level
int checkWasteDistance()
{
    unsigned int wastePing = sonar.ping();                // Perform ultrasonic ping measurement
    Serial.print("Ping: ");                               // Print ping value
    unsigned int wasteInCm = sonar.convert_cm(wastePing); // Convert ping to distance in cm
    while (wasteInCm < 0)
    {                                                         // Ensure valid distance measurement
        unsigned int wastePing = sonar.ping();                // Perform ultrasonic ping measurement
        unsigned int wasteInCm = sonar.convert_cm(wastePing); // Convert ping to distance in cm
    }
    wastePercentage = 100 - ((wasteInCm / maxLevel) * 100); // Calculate waste percentage
    delay(1000);                                            // Delay for stability
    checkWasteThreshold(wastePercentage);                   // Check if waste percentage exceeds threshold
    Serial.print("Waste Percentage: ");                     // Print waste percentage
    Serial.println(wastePercentage);                        // Print waste percentage
    return wastePercentage;                                 // Return waste percentage
}

// Check if waste percentage exceeds threshold limits
void checkWasteThreshold(int wastePercentage)
{
    if (wastePercentage <= 0)
    {                        // Check if waste percentage is below lower limit
        wastePercentage = 1; // Set waste percentage to minimum value
    }
    else if (wastePercentage >= 99)
    {                          // Check if waste percentage is above upper limit
        wastePercentage = 100; // Set waste percentage to maximum value
    }
    ::wastePercentage = wastePercentage; // Update global waste percentage variable
}

// Print waste percentage on LCD
void printWastePercentage(int wastePercentage)
{
    if (wastePercentage < maxWastePercentage)
    {                                   // Check if waste percentage is below threshold
        lcd.clear();                    // Clear LCD display
        lcd.print("Waste percentage:"); // Print label
        lcd.setCursor(0, 1);            // Move cursor to second row
        lcd.print(wastePercentage);     // Print waste percentage
        lcd.print("%");                 // Print percentage symbol
        PORTB &= B011111;               // Turn off LED indicating bin full
    }
    else
    {
        PORTB |= B100000;            // Turn on LED indicating bin full
        servo.write(0);              // Close door
        lcd.clear();                 // Clear LCD display
        lcd.print("Bin is FULL!!!"); // Print warning message
    }
}

void loop()
{
    float distdoor = measureSensorDoor(); // Measure distance to door
    servo.attach(servoPin);               // Reattach servo motor
    checkDoor(distdoor, wastePercentage); // Check door position based on waste percentage
}

// Interrupt service routine for Timer2
ISR(TIMER2_COMPA_vect)
{
    int wastePercentange = checkWasteDistance(); // Measure waste percentage
    if (wastePercentage >= 0)
    {                                          // Check if waste percentage is valid
        printWastePercentage(wastePercentage); // Print waste percentage on LCD
    }
}
