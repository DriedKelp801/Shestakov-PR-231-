#include <STM32FreeRTOS.h>
#include <FreeRTOSConfig.h>

// Пины платы Multi-Function Shield (image_4)
#define LED1 13
#define LED2 12
#define LED3 11
#define LED4 10
#define BUTTON_S3 A3

// Флаг состояния (по умолчанию выключен согласно ТЗ)
volatile bool isRunning = false;

// Дескрипторы задач
TaskHandle_t xLEDTaskHandle = NULL;
TaskHandle_t xButtonTaskHandle = NULL;
TaskHandle_t xSerialTaskHandle = NULL;

// Буферы для статической задачи Serial
#define SERIAL_STACK_SIZE 256
StaticTask_t xSerialTaskTCB;
StackType_t xSerialTaskStack[SERIAL_STACK_SIZE];

// Память для задачи Idle (требуется при configSUPPORT_STATIC_ALLOCATION = 1)
static StaticTask_t xIdleTaskTCB;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

extern "C" void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                              StackType_t **ppxIdleTaskStackBuffer,
                                              uint32_t *pulIdleTaskStackSize) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = xIdleStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

// Гашение всех светодиодов (на MFS выключение = HIGH)
void clearAllLEDs() {
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, HIGH);
    digitalWrite(LED3, HIGH);
    digitalWrite(LED4, HIGH);
}

// -------------------------------------------------------------------
// 1. Динамическая задача: Светодиоды (Порядок: 1-2-1-3-1-4)
// -------------------------------------------------------------------
void vTaskLEDs(void *pvParameters) {
    const uint8_t pattern[] = {LED1, LED2, LED1, LED3, LED1, LED4};
    const uint8_t patternSize = sizeof(pattern) / sizeof(pattern[0]);
    uint8_t currentStep = 0;

    for (;;) {
        if (isRunning) {
            clearAllLEDs();
            digitalWrite(pattern[currentStep], LOW); // LOW = включен

            currentStep = (currentStep + 1) % patternSize;

            // Блокирующая задержка 500 мс по заданию Варианта 10
            delay(500);
            vTaskDelay(pdMS_TO_TICKS(10)); // Передача управления планировщику
        } else {
            clearAllLEDs();
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// -------------------------------------------------------------------
// 2. Динамическая задача: Опрос кнопки S3 (1 клик = Старт, 2 клика = Стоп)
// -------------------------------------------------------------------
void vTaskButtons(void *pvParameters) {
    for (;;) {
        // Кнопки на MFS подтянуты к VCC, активный уровень = LOW
        if (digitalRead(BUTTON_S3) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(40)); // Антидребезг

            if (digitalRead(BUTTON_S3) == LOW) {
                uint8_t clicks = 0;
                TickType_t startTime = xTaskGetTickCount();

                // Фиксируем клики в интервале 400 мс
                while ((xTaskGetTickCount() - startTime) < pdMS_TO_TICKS(400)) {
                    if (digitalRead(BUTTON_S3) == LOW) {
                        clicks++;
                        // Ожидание отпускания кнопки
                        while (digitalRead(BUTTON_S3) == LOW) {
                            vTaskDelay(pdMS_TO_TICKS(10));
                        }
                        vTaskDelay(pdMS_TO_TICKS(30)); // Антидребезг отпускания
                    }
                    vTaskDelay(pdMS_TO_TICKS(10));
                }

                if (clicks == 1) {
                    isRunning = true;
                    Serial.println(">>> Button S3: START (1 click)");
                } else if (clicks >= 2) {
                    isRunning = false;
                    Serial.println(">>> Button S3: STOP (2 clicks)");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// -------------------------------------------------------------------
// 3. Статическая задача: Последовательный порт (HighWaterMark)
// -------------------------------------------------------------------
void vTaskSerial(void *pvParameters) {
    for (;;) {
        UBaseType_t hwmLED = uxTaskGetStackHighWaterMark(xLEDTaskHandle);
        UBaseType_t hwmButton = uxTaskGetStackHighWaterMark(xButtonTaskHandle);
        UBaseType_t hwmSerial = uxTaskGetStackHighWaterMark(xSerialTaskHandle);

        Serial.println("--- FreeRTOS Stack High Water Mark (words) ---");
        Serial.print("LED Task: "); Serial.println(hwmLED);
        Serial.print("Button Task: "); Serial.println(hwmButton);
        Serial.print("Serial Task: "); Serial.println(hwmSerial);
        Serial.print("State: "); Serial.println(isRunning ? "RUNNING" : "STOPPED");
        Serial.println("----------------------------------------------");

        delay(1000); // Блокирующая задержка 1000 мс по ТЗ
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);
    pinMode(BUTTON_S3, INPUT_PULLUP);

    clearAllLEDs();

    // Задачи: Кнопки (Приоритет 2), Диоды (Приоритет 1), Serial (Приоритет 1)
    xTaskCreate(vTaskLEDs, "LEDTask", 128, NULL, 1, &xLEDTaskHandle);
    xTaskCreate(vTaskButtons, "ButtonTask", 128, NULL, 2, &xButtonTaskHandle);

    xSerialTaskHandle = xTaskCreateStatic(
        vTaskSerial,
        "SerialTask",
        SERIAL_STACK_SIZE,
        NULL,
        1,
        xSerialTaskStack,
        &xSerialTaskTCB
    );

    vTaskStartScheduler();
}

void loop() {}