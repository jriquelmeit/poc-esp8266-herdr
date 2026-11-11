#pragma once

#include <Arduino.h>
#include "protocol.h"

class SerialManager {
public:
    SerialManager();
    void begin();
    void update(HerdrData &data);

private:
    void processLine(const String &line, HerdrData &data);
    String _buffer;
};
