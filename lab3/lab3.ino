#include <STM32FreeRTOS.h>
#include <queue.h>

#define PIN_S1 A1  // Кнопка S1-A1: Очистка очереди
#define PIN_S2 A2  // Кнопка S2-A2: Чтение данных (при удержании каждые 500 мс)
#define PIN_S3 A3  // Кнопка S3-A3: Запись данных (одиночное нажатие)

QueueHandle_t xDataQueue = NULL;

// Глобальный счетчик отправляемых значений (от 1 до 10)
volatile int dataValue = 1;

// -----------------------------------------------------------------------------
// Задача 1: Запись в очередь (значения от 1 до 10)
// -----------------------------------------------------------------------------
void vSenderTask(void *pvParameters) {
  bool lastS3State = HIGH;

  for (;;) {
    bool currentS3State = digitalRead(PIN_S3);

    if (lastS3State == HIGH && currentS3State == LOW) {
      int currentVal = dataValue;
      
      if (xQueueSend(xDataQueue, &currentVal, 0) == pdPASS) {
        Serial.print("[S3] Отправлено в очередь: ");
        Serial.println(currentVal);
        
        // Увеличение счетчика (не более 10)
        dataValue++;
        if (dataValue > 10) {
          dataValue = 1;
        }
      } else {
        Serial.println("[Ошибка] Очередь переполнена!");
      }
      vTaskDelay(pdMS_TO_TICKS(150)); // Антидребезг
    }

    lastS3State = currentS3State;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// -----------------------------------------------------------------------------
// Задача 2: Чтение из очереди (при успешном извлечении сбрасываем dataValue на 1)
// -----------------------------------------------------------------------------
void vReceiverTask(void *pvParameters) {
  int receivedData = 0;

  for (;;) {
    if (digitalRead(PIN_S2) == LOW) {
      if (xQueueReceive(xDataQueue, &receivedData, 0) == pdPASS) {
        Serial.print("[S2] Извлечено значение: ");
        Serial.println(receivedData);
        
        // При извлечении 1 элемента счетчик отправки сбрасывается снова на 1
        dataValue = 1;
      } else {
        Serial.println("[Предупреждение] Попытка чтения: очередь пуста!");
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    } else {
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }
}

// -----------------------------------------------------------------------------
// Задача 3: Очистка очереди и сброс счетчика при нажатии S1
// -----------------------------------------------------------------------------
void vResetTask(void *pvParameters) {
  bool lastS1State = HIGH;

  for (;;) {
    bool currentS1State = digitalRead(PIN_S1);

    if (lastS1State == HIGH && currentS1State == LOW) {
      if (xDataQueue != NULL) {
        xQueueReset(xDataQueue);
        dataValue = 1; // Сброс счетчика на 1 при очистке
        Serial.println("[S1] Очередь успешно очищена!");
      }
      vTaskDelay(pdMS_TO_TICKS(150));
    }

    lastS1State = currentS1State;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// -----------------------------------------------------------------------------
// Инициализация
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);

  pinMode(PIN_S1, INPUT_PULLUP);
  pinMode(PIN_S2, INPUT_PULLUP);
  pinMode(PIN_S3, INPUT_PULLUP);

  xDataQueue = xQueueCreate(10, sizeof(int));

  if (xDataQueue == NULL) {
    Serial.println("Ошибка создания очереди!");
    while (1);
  }

  xTaskCreate(vSenderTask,   "Sender",   512, NULL, 1, NULL);
  xTaskCreate(vReceiverTask, "Receiver", 512, NULL, 1, NULL);
  xTaskCreate(vResetTask,    "Reset",    512, NULL, 1, NULL);

  vTaskStartScheduler();
}

void loop() {
}