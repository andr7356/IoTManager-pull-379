#include "Cmd.h"

#include "Class/Item.h"
#include "Class/Button.h"
#include "Global.h"
#include "Module/Terminal.h"
#include "Servo/Servos.h"

static const char *MODULE = "Cmd";

Terminal *term = nullptr;

boolean but[NUM_BUTTONS];
Bounce *buttons = new Bounce[NUM_BUTTONS];

#ifdef ESP8266
SoftwareSerial *mySerial = nullptr;
#else
HardwareSerial *mySerial = nullptr;
#endif

void getData();

void cmd_init() {
    sCmd.addCommand("button", button);
    sCmd.addCommand("pwm", pwm);

    sCmd.addCommand("switch", switch_);

#ifdef ANALOG_ENABLED
    sCmd.addCommand("analog", analog);
#endif
#ifdef LEVEL_ENABLED
    sCmd.addCommand("levelPr", levelPr);
    sCmd.addCommand("ultrasonicCm", ultrasonicCm);
#endif
#ifdef DALLAS_ENABLED
    sCmd.addCommand("dallas", dallas);
#endif
#ifdef DHT_ENABLED
    sCmd.addCommand("dhtT", dhtT);
    sCmd.addCommand("dhtH", dhtH);
    sCmd.addCommand("dhtPerception", dhtP);
    sCmd.addCommand("dhtComfort", dhtC);
    sCmd.addCommand("dhtDewpoint", dhtD);
#endif

#ifdef BMP_ENABLED
    sCmd.addCommand("bmp280T", bmp280T);
    sCmd.addCommand("bmp280P", bmp280P);
#endif

#ifdef BME_ENABLED
    sCmd.addCommand("bme280T", bme280T);
    sCmd.addCommand("bme280P", bme280P);
    sCmd.addCommand("bme280H", bme280H);
    sCmd.addCommand("bme280A", bme280A);
#endif

#ifdef STEPPER_ENABLED
    sCmd.addCommand("stepper", stepper);
    sCmd.addCommand("stepperSet", stepperSet);
#endif

#ifdef SERVO_ENABLED
    sCmd.addCommand("servo", servo_);
    sCmd.addCommand("servoSet", servoSet);
#endif

#ifdef SERIAL_ENABLED
    sCmd.addCommand("serialBegin", serialBegin);
    sCmd.addCommand("serialWrite", serialWrite);
    sCmd.addCommand("getData", getData);
#endif

#ifdef LOGGING_ENABLED
    sCmd.addCommand("logging", logging);
#endif

    sCmd.addCommand("inputDigit", inputDigit);
    sCmd.addCommand("digitSet", digitSet);

    sCmd.addCommand("inputTime", inputTime);
    sCmd.addCommand("timeSet", timeSet);

    sCmd.addCommand("timerStart", timerStart_);
    sCmd.addCommand("timerStop", timerStop_);

    sCmd.addCommand("text", text);
    sCmd.addCommand("textSet", textSet);

    sCmd.addCommand("mqtt", mqttOrderSend);
    sCmd.addCommand("http", httpOrderSend);

#ifdef PUSH_ENABLED
    sCmd.addCommand("push", pushControl);
#endif

    sCmd.addCommand("firmwareUpdate", firmwareUpdate);
    sCmd.addCommand("firmwareVersion", firmwareVersion);

    handle_time_init();

    myItem = new Item();
}

//==========================================Модуль кнопок===================================================
//button out light toggle Кнопки Свет 1 pin[12] inv[1] st[1]
//==========================================================================================================
void button() {
    myItem->update();
    String key = myItem->gkey();
    String pin = myItem->gpin();
    String inv = myItem->ginv();
    String state = myItem->gstate();
    

    sCmd.addCommand(key.c_str(), buttonSet);


    myItem->pinModeSet(pin.toInt);

    if (pin != "") {
        pinMode(pin.toInt(), OUTPUT);
        jsonWriteInt(configOptionJson, key + "_pin", pin.toInt());
    }

    if (inv != "") {
        digitalWrite(pin.toInt(), !state.toInt());
        jsonWriteStr(configLiveJson, key, state);
        MqttClient::publishStatus(key, state);
    }

    if (state != "") {
        digitalWrite(pin.toInt(), state.toInt());
        jsonWriteStr(configLiveJson, key, state);
        MqttClient::publishStatus(key, state);
    }
    myItem->clear();
}

void buttonSet() {
    String key = sCmd.order();
    String state = sCmd.next();
    int pin = jsonReadInt(configOptionJson, key + "_pin");

    if (state == "change") {
        int newState = !digitalRead(pin);
        digitalWrite(pin, newState);
        eventGen(key, "");
        jsonWriteStr(configLiveJson, key, String(newState));
        MqttClient::publishStatus(key, String(newState));
    }

    if (state == "0" || state == "1") {
        digitalWrite(pin, state.toInt());
        eventGen(key, "");
        jsonWriteStr(configLiveJson, key, state);
        MqttClient::publishStatus(key, state);
    }
}

//==========================================Модуль управления ШИМ===================================================
//pwm out volume range Кнопки Свет 1 pin[12] st[500]
//==================================================================================================================
void pwm() {
    myItem->update();
    String key = myItem->gkey();
    String pin = myItem->gpin();
    String state = myItem->gstate();
    myItem->clear();

    sCmd.addCommand(key.c_str(), pwmSet);

    if (pin != "") {
        jsonWriteInt(configOptionJson, key + "_pin", pin.toInt());
        analogWrite(pin.toInt(), state.toInt());
        jsonWriteInt(configLiveJson, key, state.toInt());
        MqttClient::publishStatus(key, String(state));
    }
}

void pwmSet() {
    String key = sCmd.order();
    String state = sCmd.next();
    int pin = jsonReadInt(configOptionJson, key + "_pin");

    analogWrite(pin, state.toInt());
    eventGen(key, "");
    jsonWriteStr(configLiveJson, key, state);
    MqttClient::publishStatus(key, state);
}


//==================================================================================================================
//==========================================Модуль физической кнопки================================================
void switch_() {
    int number = String(sCmd.next()).toInt();
    int pin = String(sCmd.next()).toInt();
    int delay = String(sCmd.next()).toInt();

    buttons[number].attach(pin);
    buttons[number].interval(delay);
    but[number] = true;
}

void loopSerial() {
    if (term) {
        term->loop();
    }
}

void loopButton() {
    static uint8_t switch_number = 1;

    if (but[switch_number]) {
        buttons[switch_number].update();
        if (buttons[switch_number].fell()) {
            eventGen("switch", String(switch_number));

            jsonWriteStr(configLiveJson, "switch" + String(switch_number), "1");
        }
        if (buttons[switch_number].rose()) {
            eventGen("switch", String(switch_number));

            jsonWriteStr(configLiveJson, "switch" + String(switch_number), "0");
        }
    }
    switch_number++;
    if (switch_number == NUM_BUTTONS) {
        switch_number = 0;
    }
}

//=====================================================================================================================================
//=========================================Добавление окна ввода цифры=================================================================
void inputDigit() {
    String value_name = sCmd.next();
    String number = value_name.substring(5);
    String widget_name = sCmd.next();
    widget_name.replace("#", " ");
    String page_name = sCmd.next();
    page_name.replace("#", " ");
    String start_state = sCmd.next();
    String page_number = sCmd.next();

    jsonWriteStr(configLiveJson, "digit" + number, start_state);
    createWidget(widget_name, page_name, page_number, "inputNum", "digit" + number);
}

void digitSet() {
    String number = sCmd.next();
    String value = sCmd.next();

    jsonWriteStr(configLiveJson, "digit" + number, value);
    MqttClient::publishStatus("digit" + number, value);
}

//=====================================================================================================================================
//=========================================Добавление окна ввода времени===============================================================
void inputTime() {
    String value_name = sCmd.next();
    String number = value_name.substring(4);
    String widget_name = sCmd.next();
    widget_name.replace("#", " ");
    String page_name = sCmd.next();
    page_name.replace("#", " ");
    String start_state = sCmd.next();
    String page_number = sCmd.next();

    jsonWriteStr(configLiveJson, "time" + number, start_state);
    createWidget(widget_name, page_name, page_number, "inputTime", "time" + number);
}

void timeSet() {
    String number = sCmd.next();
    String value = sCmd.next();

    jsonWriteStr(configLiveJson, "time" + number, value);
    MqttClient::publishStatus("time" + number, value);
}

void handle_time_init() {
    ts.add(
        TIME, 1000, [&](void *) {
            jsonWriteStr(configLiveJson, "time", timeNow->getTime());
            jsonWriteStr(configLiveJson, "timenow", timeNow->getTimeJson());
            eventGen("timenow", "");
        },
        nullptr, true);
}

//=====================================================================================================================================
//=========================================Добавление текстового виджета============================================================
void text() {
    String number = sCmd.next();
    String widget_name = sCmd.next();
    String page_name = sCmd.next();
    String page_number = sCmd.next();

    createWidget(widget_name, page_name, page_number, "anydata", "text" + number);
}

void textSet() {
    String number = sCmd.next();
    String text = sCmd.next();
    text.replace("_", " ");

    if (text.indexOf("-time") >= 0) {
        text.replace("-time", "");
        text.replace("#", " ");
        text = text + " " + timeNow->getDateTimeDotFormated();
    }

    jsonWriteStr(configLiveJson, "text" + number, text);
    MqttClient::publishStatus("text" + number, text);
}
//=====================================================================================================================================
//=========================================Модуль шагового мотора======================================================================
#ifdef STEPPER_ENABLED
//stepper 1 12 13
void stepper() {
    String stepper_number = sCmd.next();
    String pin_step = sCmd.next();
    String pin_dir = sCmd.next();

    jsonWriteStr(configOptionJson, "stepper" + stepper_number, pin_step + " " + pin_dir);
    pinMode(pin_step.toInt(), OUTPUT);
    pinMode(pin_dir.toInt(), OUTPUT);
}

//stepperSet 1 100 5
void stepperSet() {
    String stepper_number = sCmd.next();
    String steps = sCmd.next();
    jsonWriteStr(configOptionJson, "steps" + stepper_number, steps);
    String stepper_speed = sCmd.next();
    String pin_step = selectToMarker(jsonReadStr(configOptionJson, "stepper" + stepper_number), " ");
    String pin_dir = deleteBeforeDelimiter(jsonReadStr(configOptionJson, "stepper" + stepper_number), " ");
    Serial.println(pin_step);
    Serial.println(pin_dir);
    if (steps.toInt() > 0) digitalWrite(pin_dir.toInt(), HIGH);
    if (steps.toInt() < 0) digitalWrite(pin_dir.toInt(), LOW);
    if (stepper_number == "1") {
        ts.add(
            STEPPER1, stepper_speed.toInt(), [&](void *) {
                int steps_int = abs(jsonReadInt(configOptionJson, "steps1") * 2);
                static int count;
                count++;
                String pin_step = selectToMarker(jsonReadStr(configOptionJson, "stepper1"), " ");
                digitalWrite(pin_step.toInt(), !digitalRead(pin_step.toInt()));
                yield();
                if (count > steps_int) {
                    digitalWrite(pin_step.toInt(), LOW);
                    ts.remove(STEPPER1);
                    count = 0;
                }
            },
            nullptr, true);
    }
    if (stepper_number == "2") {
        ts.add(
            STEPPER2, stepper_speed.toInt(), [&](void *) {
                int steps_int = abs(jsonReadInt(configOptionJson, "steps2") * 2);
                static int count;
                count++;
                String pin_step = selectToMarker(jsonReadStr(configOptionJson, "stepper2"), " ");
                digitalWrite(pin_step.toInt(), !digitalRead(pin_step.toInt()));
                yield();
                if (count > steps_int) {
                    digitalWrite(pin_step.toInt(), LOW);
                    ts.remove(STEPPER2);
                    count = 0;
                }
            },
            nullptr, true);
    }
}
#endif
//====================================================================================================================================================
//=================================================================Сервоприводы=======================================================================
#ifdef SERVO_ENABLED
//servo 1 13 50 Мой#сервопривод Сервоприводы 0 100 0 180 2
void servo_() {
    String number = sCmd.next();
    uint8_t pin = String(sCmd.next()).toInt();
    int value = String(sCmd.next()).toInt();

    String widget = sCmd.next();
    String page = sCmd.next();

    int min_value = String(sCmd.next()).toInt();
    int max_value = String(sCmd.next()).toInt();
    int min_deg = String(sCmd.next()).toInt();
    int max_deg = String(sCmd.next()).toInt();

    String pageNumber = sCmd.next();

    jsonWriteStr(configOptionJson, "servo_pin" + number, String(pin, DEC));

    value = map(value, min_value, max_value, min_deg, max_deg);

    Servo *servo = myServo.create(number.toInt(), pin);
    servo->write(value);
#ifdef ESP32
    myServo1.attach(servo_pin.toInt(), 500, 2400);
    myServo1.write(start_state_int);
#endif

    jsonWriteInt(configOptionJson, "s_min_val" + number, min_value);
    jsonWriteInt(configOptionJson, "s_max_val" + number, max_value);
    jsonWriteInt(configOptionJson, "s_min_deg" + number, min_deg);
    jsonWriteInt(configOptionJson, "s_max_deg" + number, max_deg);

    jsonWriteInt(configLiveJson, "servo" + number, value);

    createWidgetParam(widget, page, pageNumber, "range", "servo" + number, "min", String(min_value), "max", String(max_value), "k", "1");
}

void servoSet() {
    String number = sCmd.next();
    int value = String(sCmd.next()).toInt();

    value = map(value,
                jsonReadInt(configOptionJson, "s_min_val" + number),
                jsonReadInt(configOptionJson, "s_max_val" + number),
                jsonReadInt(configOptionJson, "s_min_deg" + number),
                jsonReadInt(configOptionJson, "s_max_deg" + number));

    Servo *servo = myServo.get(number.toInt());
    if (servo) {
        servo->write(value);
    }

    eventGen("servo", number);
    jsonWriteInt(configLiveJson, "servo" + number, value);
    MqttClient::publishStatus("servo" + number, String(value, DEC));
}
#endif
//====================================================================================================================================================
//=============================================================Модуль сериал порта=======================================================================

#ifdef SERIAL_ENABLED
void serialBegin() {
    String s_speed = sCmd.next();
    String rxPin = sCmd.next();
    String txPin = sCmd.next();

    if (mySerial) {
        delete mySerial;
    }

#ifdef ESP8266
    mySerial = new SoftwareSerial(rxPin.toInt(), txPin.toInt());
    mySerial->begin(s_speed.toInt());
#else
    mySerial = new HardwareSerial(2);
    mySerial->begin(rxPin.toInt(), txPin.toInt());
#endif

    term = new Terminal(mySerial);
    term->setEOL(LF);
    term->enableColors(false);
    term->enableControlCodes(false);
    term->enableEcho(false);
    term->setOnReadLine([](const char *str) {
        String line = String(str);
        addCommandLoop(line);
    });
}

void getData() {
    String param = sCmd.next();
    String res = param.length() ? jsonReadStr(configLiveJson, param) : configLiveJson;
    if (term) {
        term->println(res.c_str());
    }
}

void serialWrite() {
    String payload = sCmd.next();
    if (term) {
        term->println(payload.c_str());
    }
}
#endif
//====================================================================================================================================================
//=================================================Глобальные команды удаленного управления===========================================================

void mqttOrderSend() {
    String id = sCmd.next();
    String order = sCmd.next();

    String all_line = jsonReadStr(configSetupJson, "mqttPrefix") + "/" + id + "/order";
    mqtt.publish(all_line.c_str(), order.c_str(), false);
}

void httpOrderSend() {
    String ip = sCmd.next();
    String order = sCmd.next();
    order.replace("_", "%20");
    String url = "http://" + ip + "/cmd?command=" + order;
    getURL(url);
}

void firmwareUpdate() {
    updateFlag = true;
}

void firmwareVersion() {
    String widget = sCmd.next();
    String page = sCmd.next();
    String pageNumber = sCmd.next();

    jsonWriteStr(configLiveJson, "firmver", FIRMWARE_VERSION);
    createWidget(widget, page, pageNumber, "anydata", "firmver");
}

void addCommandLoop(const String &cmdStr) {
    order_loop += cmdStr;
    if (!cmdStr.endsWith(",")) {
        order_loop += ",";
    }
}

void fileExecute(const String &filename) {
    String cmdStr = readFile(filename, 2048);
    stringExecute(cmdStr);
}

void stringExecute(String &cmdStr) {
    cmdStr += "\r\n";
    cmdStr.replace("\r\n", "\n");
    cmdStr.replace("\r", "\n");

    while (cmdStr.length()) {
        String buf = selectToMarker(cmdStr, "\n");
        sCmd.readStr(buf);
        cmdStr = deleteBeforeDelimiter(cmdStr, "\n");
    }
}

void loopCmd() {
    if (order_loop.length()) {
        String tmp = selectToMarker(order_loop, ",");  //выделяем первую команду rel 5 1,
        pm.info("do: " + tmp);
        sCmd.readStr(tmp);                                    //выполняем
        order_loop = deleteBeforeDelimiter(order_loop, ",");  //осекаем
    }
}
