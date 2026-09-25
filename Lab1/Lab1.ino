#include <STM32FreeRTOS.h>
#include <FreeRTOSConfig.h>

#define LED1_PIN D2
#define LED2_PIN D3
#define LED3_PIN D4
#define LED4_PIN D5

#define BUTTON_S3 D6

#define LED_DELAY_MS       250
#define LED_BLOCK_DELAY_MS 500
#define SERIAL_DELAY_MS    1000

#define LED_STACK_SIZE     256
#define BUTTON_STACK_SIZE  256
#define SERIAL_STACK_SIZE  256

StaticTask_t serialTaskTCB;
StackType_t serialTaskStack[SERIAL_STACK_SIZE];

TaskHandle_t serialTaskHandle;
TaskHandle_t ledTaskHandle;
TaskHandle_t buttonTaskHandle;
Volatile bool running = false;
Void allLedsOff()
{
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    digitalWrite(LED3_PIN, LOW);
    digitalWrite(LED4_PIN, LOW);
}
Void showLed(uint8_t number)
{
    allLedsOff();

    switch (number)
    {
        Case 1:
            digitalWrite(LED1_PIN, HIGH);
            break;

        case 2:
            digitalWrite(LED2_PIN, HIGH);
            break;

        case 3:
            digitalWrite(LED3_PIN, HIGH);
            break;

        case 4:
            digitalWrite(LED4_PIN, HIGH);
            break;
    }
}

Void LedTask(void *pvParameters)
{

    Const uint8_t sequence[] = {1, 2, 1, 3, 1, 4};

    Const uint8_t sequenceLength =
        Sizeof(sequence) / sizeof(sequence[0]);

    Uint8_t index = 0;

    For (;;)
    {
        If (running)
        {
            showLed(sequence[index]);

            Delay(LED_DELAY_MS);
            Delay(LED_BLOCK_DELAY_MS);

            Index++;

            If (index >= sequenceLength)
            {
                Index = 0;
            }
        }
        Else
        {
            allLedsOff();

            Delay(20);
        }
    }
}

Void ButtonTask(void *pvParameters)
{
    Bool lastButtonState = HIGH;

    Uint8_t pressCount = 0;

    Unsigned long firstPressTime = 0;

    For (;;)
    {
        Bool currentButtonState =
            digitalRead(BUTTON_S3);

        If (lastButtonState == HIGH &&
            currentButtonState == LOW)
        {
            Unsigned long currentTime = millis();

            If (pressCount == 0)
            {
                pressCount = 1;
                firstPressTime = currentTime;
            }
            Else
            {
                If (currentTime – firstPressTime <= 1000)
                {
                    pressCount = 2;
                }
            }

            Delay(50);
        }

        lastButtonState = currentButtonState;

        If (pressCount == 1 &&
            Millis() – firstPressTime > 1000)
        {
            Running = true;

            pressCount = 0;
        }

        If (pressCount >= 2)
        {
            Running = false;

            allLedsOff();

            pressCount = 0;
        }

        Delay(500);
    }
}

Void SerialTask(void *pvParameters)
{
    For (;;)
    {
        Serial.println();
        Serial.println(“==============================”);
        Serial.println(“ FreeRTOS – Variant 10”);
        Serial.println(“==============================”);

        Serial.print(“LED task HighWaterMark: “);
        Serial.print(
            uxTaskGetStackHighWaterMark(ledTaskHandle)
        );
        Serial.println(“ words”);

        Serial.print(“Button task HighWaterMark: “);
        Serial.print(
            uxTaskGetStackHighWaterMark(buttonTaskHandle)
        );
        Serial.println(“ words”);

        Serial.print(“Serial task HighWaterMark: “);
        Serial.print(
            uxTaskGetStackHighWaterMark(NULL)
        );
        Serial.println(“ words”);

        Serial.print(“Running: “);

        If (running)
            Serial.println(“YES”);
        Else
            Serial.println(“NO”);

        Serial.println(“==============================”);

        Delay(SERIAL_DELAY_MS);
    }
}

Void setup()
{

    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(LED3_PIN, OUTPUT);
    pinMode(LED4_PIN, OUTPUT);

    allLedsOff();

    pinMode(BUTTON_S3, INPUT_PULLUP);

    Serial.begin(115200);

    Delay(1000);

    Serial.println();
    Serial.println(“FreeRTOS Laboratory Work #2”);
    Serial.println(“Variant 10”);
    Serial.println(“Sequence: 1-2-1-3-1-4”);
    Serial.println(“Button S3:”);
    Serial.println(“1 press  -> START”);
    Serial.println(“2 presses -> STOP”);
    Serial.println();

    ledTaskHandle = xTaskCreate(
        LedTask,
        “LED_Task”,
        LED_STACK_SIZE,
        NULL,
        2,
        &ledTaskHandle
    );

    buttonTaskHandle = xTaskCreate(
        ButtonTask,
        “Button_Task”,
        BUTTON_STACK_SIZE,
        NULL,
        3,
        &buttonTaskHandle
    );

    serialTaskHandle = xTaskCreateStatic(
        SerialTask,
        “Serial_Task”,
        SERIAL_STACK_SIZE,
        NULL,
        1,
        serialTaskStack,
        &serialTaskTCB
    );

    vTaskStartScheduler();
}

Void loop(){
}

