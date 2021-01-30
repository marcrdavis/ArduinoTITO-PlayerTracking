#include "Arduino.h"
#include <stddef.h>
#include <util/delay.h>


#define DIRECTION(X,D)	if (D) pinMode(X##_PIN, OUTPUT); else pinMode(X##_PIN, INPUT)
#define RAISE(X)	digitalWrite(X##_PIN, HIGH)
#define LOWER(X)	digitalWrite(X##_PIN, LOW)
#define CHECK(X)	digitalRead(X##_PIN)



class GU7000_Interface{
public:
    virtual void init() = 0;
    virtual void write(uint8_t data) = 0;
    virtual void hardReset() = 0;
	
	unsigned getModelClass;
};

