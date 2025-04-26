#include <ESP8266WiFi.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <ThingSpeak.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// Настройки Wi-Fi
const char* ssid = "HONOR 200 Lite";
const char* password = "ilia123il";

// Настройки ThingSpeak
const unsigned long channelID = 2935407;
const char* apiKey = "D5279UK7AQZ4E7P2";

// Настройки Telegram бота
#define BOT_TOKEN "7783493729:AAHim-qxd3xoC1sGjpPDoDfNNNgN_V9Y7rI"
#define CHAT_ID "1039477265"

// Пины подключения
#define ONE_WIRE_BUS D2
#define VIBRATION_PIN D3
#define CURRENT_PIN A0
#define LED_PIN D0

// Интервалы
const unsigned long CHECK_INTERVAL = 30000;
const unsigned long BOT_UPDATE_INTERVAL = 5000;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

WiFiClient client;
WiFiClientSecure secureClient;
UniversalTelegramBot bot(BOT_TOKEN, secureClient);

// Состояние системы
struct SystemState {
  bool needsLubrication = false;
  unsigned long lastLubricationTime = 0;
  unsigned long lastVibrationCheck = 0;
  unsigned long lastBotUpdate = 0;
  int vibrationCount = 0;
} systemState;

// Прототипы функций
float readDSTemperature();
void handleTelegramMessages();
void sendSensorData(String chat_id);
void sendHelp(String chat_id);
void sendStatus(String chat_id);
void updateKeyboard(String chat_id);

void setup() {
  Serial.begin(115200);
  
  // Инициализация датчиков
  ds18b20.begin();
  pinMode(VIBRATION_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(CURRENT_PIN, INPUT);
  
  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi подключен!");
  
  // Настройка Telegram бота
  secureClient.setInsecure();
  bot.longPoll = 60;
  
  // Инициализация ThingSpeak
  ThingSpeak.begin(client);
  
  // Отправка приветственного сообщения с клавиатурой
  bot.sendMessage(CHAT_ID, "🤖 Система мониторинга смазки запущена!", "");
  updateKeyboard(CHAT_ID);
}

void loop() {
  // Чтение данных с датчиков
  float temperature = readDSTemperature();
  int current = analogRead(CURRENT_PIN);
  
  // Обработка вибрации
  if (digitalRead(VIBRATION_PIN)) {
    systemState.vibrationCount++;
    delay(50);
  }

  // Проверка состояния
  if (millis() - systemState.lastVibrationCheck >= CHECK_INTERVAL) {
    systemState.lastVibrationCheck = millis();
    systemState.vibrationCount = 0;
  }

  // Обработка сообщений Telegram
  if (millis() - systemState.lastBotUpdate > BOT_UPDATE_INTERVAL) {
    handleTelegramMessages();
    systemState.lastBotUpdate = millis();
  }

  delay(100);
}

// Обновление клавиатуры меню
void updateKeyboard(String chat_id) {
  String keyboardJson = "[[\"/help\", \"/data\", \"/status\"]]";
  bot.sendMessageWithReplyKeyboard(chat_id, "Выберите действие:", "", keyboardJson, true);
}

// Обработка сообщений Telegram
void handleTelegramMessages() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while (numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
      String text = bot.messages[i].text;
      String chat_id = String(bot.messages[i].chat_id);
      
      if (text == "/start") {
        bot.sendMessage(chat_id, "Добро пожаловать! Используйте меню ниже.", "");
        updateKeyboard(chat_id);
      }
      else if (text == "/help") {
        sendHelp(chat_id);
      }
      else if (text == "/data") {
        sendSensorData(chat_id);
      }
      else if (text == "/status") {
        sendStatus(chat_id);
      }
      else {
        bot.sendMessage(chat_id, "Неизвестная команда. Используйте /help", "");
      }
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}
// Отправка данных с датчиков
void sendSensorData(String chat_id) {
  float temp = readDSTemperature();
  int current = analogRead(CURRENT_PIN);
  
  String data = "📊 Данные датчиков:\n";
  data += "🌡 Температура: " + String(temp) + "°C\n";
  data += "⚡ Ток: " + String(map(current, 0, 1023, 0, 100)) + "%\n";
  data += "📳 Вибрации: " + String(systemState.vibrationCount) + "\n";
  bot.sendMessage(chat_id, data, "");
}

// Отправка справки
void sendHelp(String chat_id) {
  String help = "📌 Доступные команды:\n\n"
                "/help - показать это сообщение\n"
                "/data - текущие показания датчиков\n"
                "/status - состояние системы";
  bot.sendMessage(chat_id, help, "");
}

// Отправка статуса системы
void sendStatus(String chat_id) {
  String status = "📶 Сигнал WiFi: " + String(WiFi.RSSI()) + " dBm\n";
  status += "⏱ Время работы: " + String(millis() / 3600000) + " ч\n";
  status += "🛢 Статус смазки: " + String(systemState.needsLubrication ? "Требуется" : "Норма");
  bot.sendMessage(chat_id, status, "");
}

// Чтение температуры
float readDSTemperature() {
  ds18b20.requestTemperatures();
  float temp = ds18b20.getTempCByIndex(0);
  return temp != -127.00 ? temp : 0;
}
