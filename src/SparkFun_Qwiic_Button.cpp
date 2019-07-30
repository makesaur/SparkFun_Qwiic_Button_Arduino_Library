/******************************************************************************
SparkFun_Qwiic_Button.cpp
SparkFun Qwiic Button/Switch Library Source File
Fischer Moseley @ SparkFun Electronics
Original Creation Date: July 24, 2019
https://github.com/sparkfunX/

This file implements the QwiicButton class, prototyped in SparkFun_Qwiic_Button.h

Development environment specifics:
	IDE: Arduino 1.8.9
	Hardware Platform: Arduino Uno/SparkFun Redboard
	Qwiic Button Version: 1.0.0
    Qwiic Switch Version: 1.0.0

This code is beerware; if you see me (or any other SparkFun employee) at the
local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.
******************************************************************************/

#include <Wire.h>
#include <SparkFun_Qwiic_Button.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

/*-------------------------------- Device Status ------------------------*/

bool QwiicButton::begin(uint8_t address, TwoWire &wirePort) {
    _deviceAddress = address; //grab the address that the sensor is on
    _i2cPort = &wirePort; //grab the port that the user wants to use
    
    //return true if the device is connected and the device ID is what we expect
    bool success = isConnected();
    success &= checkDeviceID();
    return success;
}

bool QwiicButton::isConnected() {
    _i2cPort->beginTransmission(_deviceAddress);
    return (_i2cPort->endTransmission() == 0);
}

uint8_t QwiicButton::deviceID() {
    return readSingleRegister(ID); //read and return the value in the ID register
}

bool QwiicButton::checkDeviceID() {
    return ( (deviceID() == DEV_ID_SW) || (deviceID() == DEV_ID_BTN) ); //Return true if the device ID matches either the button or the switch
}

uint8_t QwiicButton::getDeviceType() {
    if(isConnected()){ //only try to get the device ID if the device will acknowledge
        uint8_t id = deviceID();
        if(id == DEV_ID_BTN) return 1; //
        if(id == DEV_ID_SW) return 2;
    }
    return 0;
}

uint16_t QwiicButton::getFirmwareVersion() {
    uint16_t version = (readSingleRegister(FIRMWARE_MAJOR)) << 8;
    version |= readSingleRegister(FIRMWARE_MINOR);
    return version;
}


/*------------------------------ Button Status ---------------------- */
bool QwiicButton::isPressed() {
    statusRegisterBitField statusRegister;
    statusRegister.byteWrapped = readSingleRegister(BUTTON_STATUS);
    return statusRegister.isPressed;
}

bool QwiicButton::hasBeenClicked() {
    statusRegisterBitField statusRegister;
    statusRegister.byteWrapped = readSingleRegister(BUTTON_STATUS);
    return statusRegister.hasBeenClicked;
}

uint16_t QwiicButton::getDebounceTime() {
    return readDoubleRegister(BUTTON_DEBOUNCE_TIME);
}

uint8_t QwiicButton::setDebounceTime(uint16_t time) {
    writeDoubleRegisterWithReadback(BUTTON_DEBOUNCE_TIME, time);
}


/*------------------- Interrupt Status/Configuration ---------------- */
uint8_t QwiicButton::enablePressedInterrupt() {
    interruptConfigBitField interruptConfig;
    interruptConfig.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfig.pressedEnable = 1;
    writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfig.byteWrapped);
}

uint8_t QwiicButton::disablePressedInterrupt() {
    interruptConfigBitField interruptConfig;
    interruptConfig.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfig.pressedEnable = 0;
    writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfig.byteWrapped);
}

uint8_t QwiicButton::enableClickedInterrupt() {
    interruptConfigBitField interruptConfig;
    interruptConfig.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfig.clickedEnable = 1;
    writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfig.byteWrapped);  
}

uint8_t QwiicButton::disableClickedInterrupt() {
    interruptConfigBitField interruptConfig;
    interruptConfig.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfig.clickedEnable = 0;
    writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfig.byteWrapped);
}

bool QwiicButton::interruptTriggered() {
    interruptConfigBitField interruptConfig;
    interruptConfig.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    return interruptConfig.status;   
}

uint8_t QwiicButton::clearInterrupt() {
    interruptConfigBitField interruptConfig;
    interruptConfig.byteWrapped = readSingleRegister(INTERRUPT_CONFIG);
    interruptConfig.status = 0;
    writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfig.byteWrapped);
}

uint8_t QwiicButton::resetInterruptConfig() {
    interruptConfigBitField interruptConfig;
    interruptConfig.pressedEnable = 0;
    interruptConfig.clickedEnable = 0;
    interruptConfig.status = 0;
    return writeSingleRegisterWithReadback(INTERRUPT_CONFIG, interruptConfig.byteWrapped);
}


/*------------------------- Queue Manipulation ---------------------- */
//pressed queue manipulation
bool QwiicButton::isPressedQueueFull() {
    queueStatusBitField pressedQueueStatus;
    pressedQueueStatus.byteWrapped = readSingleRegister(PRESSED_QUEUE_STATUS);
    return pressedQueueStatus.isFull;    
}

bool QwiicButton::isPressedQueueEmpty() {
    queueStatusBitField pressedQueueStatus;
    pressedQueueStatus.byteWrapped = readSingleRegister(PRESSED_QUEUE_STATUS);
    return pressedQueueStatus.isEmpty;
}

unsigned long QwiicButton::timeSinceLastPress() {
    return readQuadRegister(PRESSED_QUEUE_FRONT);
}

unsigned long QwiicButton::timeSinceFirstPress() {
    return readQuadRegister(PRESSED_QUEUE_BACK);
}

unsigned long QwiicButton::popPressedQueue() {
    unsigned long tempData = timeSinceFirstPress();
    queueStatusBitField pressedQueueStatus;
    pressedQueueStatus.byteWrapped = readSingleRegister(PRESSED_QUEUE_STATUS);
    pressedQueueStatus.popRequest = 1;
    writeSingleRegister(PRESSED_QUEUE_STATUS, pressedQueueStatus.byteWrapped);
    return tempData;
}


//clicked queue manipulation
bool QwiicButton::isClickedQueueFull() {
    queueStatusBitField clickedQueueStatus;
    clickedQueueStatus.byteWrapped = readSingleRegister(CLICKED_QUEUE_STATUS);
    return clickedQueueStatus.isFull;    
}

bool QwiicButton::isClickedQueueEmpty() {
    queueStatusBitField clickedQueueStatus;
    clickedQueueStatus.byteWrapped = readSingleRegister(CLICKED_QUEUE_STATUS);
    return clickedQueueStatus.isEmpty;
}

unsigned long QwiicButton::timeSinceLastClick() {
    return readQuadRegister(CLICKED_QUEUE_FRONT);
}

unsigned long QwiicButton::timeSinceFirstClick() {
    return readQuadRegister(CLICKED_QUEUE_BACK);
}

unsigned long QwiicButton::popClickedQueue() {
    unsigned long tempData = timeSinceFirstClick();
    queueStatusBitField clickedQueueStatus;
    clickedQueueStatus.byteWrapped = readSingleRegister(CLICKED_QUEUE_STATUS);
    clickedQueueStatus.popRequest = 1;
    writeSingleRegister(CLICKED_QUEUE_STATUS, clickedQueueStatus.byteWrapped);
    return tempData;
}


/*------------------------ LED Configuration ------------------------ */
bool QwiicButton::LEDconfig(uint8_t brightness, uint8_t granularity, uint16_t cycleTime, uint16_t offTime) {
    bool success = writeSingleRegister(LED_BRIGHTNESS, brightness);
    success &= writeSingleRegister(LED_PULSE_GRANULARITY, granularity);
    success &= writeDoubleRegister(LED_PULSE_CYCLE_TIME, cycleTime);
    success &= writeDoubleRegister(LED_PULSE_OFF_TIME, offTime);
    return success;
}

bool QwiicButton::LEDoff() {
    return LEDconfig(0, 1, 0, 0);
}

bool QwiicButton::LEDon(uint8_t brightness) {
    return LEDconfig(brightness, 1, 0, 0);
}


/*------------------------- Internal I2C Abstraction ---------------- */

uint8_t QwiicButton::readSingleRegister(Qwiic_Button_Register reg) {
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->endTransmission();
    if(_i2cPort->requestFrom(_deviceAddress, 1) != 0){
        return _i2cPort->read();     
    }
}

uint16_t QwiicButton::readDoubleRegister(Qwiic_Button_Register reg) { //little endian
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->endTransmission();
    if(_i2cPort->requestFrom(_deviceAddress, 2) != 0){
        uint16_t data = _i2cPort->read();
        data |= (_i2cPort->read() << 8);
        return data;
    }
}

unsigned long QwiicButton::readQuadRegister(Qwiic_Button_Register reg) {
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->endTransmission();
    
    union databuffer {
        uint8_t array[4];
        unsigned long integer;
    };

    databuffer data;
    if(_i2cPort->requestFrom(_deviceAddress, 4) != 0) {
        for(uint8_t i = 0; i < 4; i++) {
            data.array[i] = _i2cPort->read();           
        }
    }
    return data.integer;
}

bool QwiicButton::writeSingleRegister(Qwiic_Button_Register reg, uint8_t data) {
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->write(data);
    return (_i2cPort->endTransmission() != 0);
}

bool QwiicButton::writeDoubleRegister(Qwiic_Button_Register reg, uint16_t data) {
    _i2cPort->beginTransmission(_deviceAddress);
    _i2cPort->write(reg);
    _i2cPort->write(lowByte(data));
    _i2cPort->write(highByte(data));
    return (_i2cPort->endTransmission() != 0);
}

uint8_t QwiicButton::writeSingleRegisterWithReadback(Qwiic_Button_Register reg, uint8_t data) {
    if(writeSingleRegister(reg, data)) return 1;
    if(readSingleRegister(reg) != data) return 2;
    return 0;
}

uint16_t QwiicButton::writeDoubleRegisterWithReadback(Qwiic_Button_Register reg, uint16_t data) {
    if(writeDoubleRegister(reg, data)) return 1;
    if(readDoubleRegister(reg) != data) return 2;
    return 0;
}