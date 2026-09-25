#include <STM32FreeRTOS.h>

// Определение пинов по Приложению 1
const uint8_t LED1 = PA5;   // LED1 для vLed2Task
const uint8_t LED3 = PA7;   // LED3 для vLed1Task
const uint8_t BTN_S2 = PA4; // Кнопка S2 для самоудаления vLed1Task

// Дескрипторы задач
TaskHandle_t xLed1TaskHandle = NULL;
TaskHandle_t xLed2TaskHandle = NULL;

// 1. Управляемая задача 1: мигание LED3 с периодом 0,4с, самоудаляется по нажатию S2
void vLed1Task(void *pvParameters) {
    pinMode(LED3, OUTPUT);
    pinMode(BTN_S2, INPUT_PULLUP);
    
    for(;;) {
        // Проверяем нажатие кнопки S2 (LOW при нажатии из-за PULLUP)
        if (digitalRead(BTN_S2) == LOW) {
            xLed1TaskHandle = NULL;
            vTaskDelete(NULL); // Самоудаление задачи
        }
        
        digitalWrite(LED3, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200)); // 200 мс вкл
        digitalWrite(LED3, LOW);
        vTaskDelay(pdMS_TO_TICKS(200)); // 200 мс выкл (суммарно 0,4с)
    }
}

// 2. Управляемая задача 2: мигание LED1 с периодом 0,7с
void vLed2Task(void *pvParameters) {
    pinMode(LED1, OUTPUT);
    
    for(;;) {
        digitalWrite(LED1, HIGH);
        vTaskDelay(pdMS_TO_TICKS(350)); // 350 мс вкл
        digitalWrite(LED1, LOW);
        vTaskDelay(pdMS_TO_TICKS(350)); // 350 мс выкл (суммарно 0,7с)
    }
}

// 3. Управляющая задача: получает команды по UART
void vCmTask(void *pvParameters) {
    char cmd;
    
    for(;;) {
        if (Serial.available() > 0) {
            cmd = Serial.read();
            
            if (cmd == 'X') {
                // «X» - создание задачи vLed2Task (LED1)
                if (xLed2TaskHandle == NULL) {
                    xTaskCreate(vLed2Task, "Led2", 1024, NULL, 2, &xLed2TaskHandle);
                }
            } 
            else if (cmd == 'C') {
                // «C» - приостановка vLed2Task
                if (xLed2TaskHandle != NULL) {
                    vTaskSuspend(xLed2TaskHandle);
                }
            } 
            else if (cmd == 'V') {
                // «V» - возобновление работы vLed2Task
                if (xLed2TaskHandle != NULL) {
                    vTaskResume(xLed2TaskHandle);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(9600);
    
    // Создаем задачу vLed1Task (LED3, приоритет 1) сразу при старте
    xTaskCreate(vLed1Task, "Led1", 1024, NULL, 1, &xLed1TaskHandle);
    
    // Создаем управляющую задачу vCmTask с высшим приоритетом (приоритет 3)
    xTaskCreate(vCmTask, "Cmd", 1024, NULL, 3, NULL);
    
    // Запуск планировщика FreeRTOS
    vTaskStartScheduler();
}

void loop() {
    // Пустой цикл, управление передано плануровщику FreeRTOS
    while(1) {}
}