#include "Global.h"
#include "classes/IoTItem.h"

class Loging : public IoTItem {
   private:
    String logval;

   public:
    Loging(String parameters) : IoTItem(parameters) {
        jsonRead(parameters, F("logid"), logval);
    }

    // void setValue(IoTValue Value) {
    //     value = Value;
    //     regEvent((String)(int)value.valD, "Loging");
    // }

    void doByInterval() {
        String value = getItemValue(logval);
        if (value == "") {
            SerialPrint("E", F("Logging"), F("no value set"));
        } else {
            regEvent(value, "Logging");
        }
    }
};

void* getAPI_Loging(String subtype, String param) {
    if (subtype == F("Loging")) {
        return new Loging(param);
    } else {
        return nullptr;
    }
}
