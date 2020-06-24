#include <SPI.h>
#include <Arduino.h>
#include <spihelp.hpp>
#include <map>

#define AdxlInt_AZ 25
#define AdxlInt_EL 26
#define AdxlInt_Ballence 27

std::map<int, ADXL345_SPI *> callbacks;

void IRAM_ATTR interruptCallbackAZ()
{
    callbacks[AdxlInt_AZ]->interruptCallback();
}

void IRAM_ATTR interruptCallbackEL()
{
    callbacks[AdxlInt_EL]->interruptCallback();
}

void IRAM_ATTR interruptCallbackBallence()
{
    callbacks[AdxlInt_Ballence]->interruptCallback();
}

ADXL345_SPI::ADXL345_SPI(SPIClass *bus, uint8_t csLine, uint8_t inturuptLine)
{
    this->Buss = bus;
    this->cs_Line = csLine;
    this->interruptPin = inturuptLine;
    pinMode(this->interruptPin, INPUT_PULLUP);
    //callbacks[interruptPin] = this->interruptCallback();
    callbacks[interruptPin] = this;
    if (inturuptLine == AdxlInt_AZ)
    {
        attachInterrupt(this->interruptPin, interruptCallbackAZ, RISING);
    }
    if (inturuptLine == AdxlInt_EL)
    {
        attachInterrupt(this->interruptPin, interruptCallbackEL, RISING);
    }
    if (inturuptLine == AdxlInt_Ballence)
    {
        attachInterrupt(this->interruptPin, interruptCallbackBallence, RISING);
    }
}

void IRAM_ATTR ADXL345_SPI::interruptCallback()
{
    this->bufferFull = true;
}

void ADXL345_SPI::init()
{
    if (Buss == NULL)
    {
        return;
    }
    pinMode(this->cs_Line, OUTPUT); //set chipselect to output mode
    spiWriteSingleADXL(*this, ADXL345_POWER_CTL, (uint8_t)0);
    delayMicroseconds(20);
    spiWriteSingleADXL(*this, ADXL345_POWER_CTL, (uint8_t)0b00001000); //place in measurement mode
    delayMicroseconds(20);
    spiWriteSingleADXL(*this, ADXL345_BW_RATE, (uint8_t)0b00001101); //800hz sample rate
    delayMicroseconds(20);
    spiWriteSingleADXL(*this, ADXL345_DATA_FORMAT, (uint8_t)0b00000000); //ensure defaults are active
    delayMicroseconds(20);
    spiWriteSingleADXL(*this, ADXL345_FIFO_CTL, (uint8_t)0);
    delayMicroseconds(20);
    spiWriteSingleADXL(*this, ADXL345_FIFO_CTL, (uint8_t)0b10111000); // (10|stream mode) (1|triger to INT1) (11000|trigger at 24 samples)
    delayMicroseconds(20);
    spiWriteSingleADXL(*this, ADXL345_INT_ENABLE, (uint8_t)0b00000011); // water mark and overrun intureupt enable
    delayMicroseconds(20);
    spiWriteSingleADXL(*this, ADXL345_INT_MAP, (uint8_t)0); // map interrupts to INT1
}

void spiWriteSingleADXL(SPI_DEVICE dev, uint8_t adr, uint8_t daa)
{
    dev.Buss->beginTransaction(SPISettings(dev.spi_clock_speed, MSBFIRST, dev.spi_mode)); //sdi rising, sdo falling
    digitalWrite(dev.cs_Line, LOW);                                                       //pull SS slow to prep other end for transfer
    dev.Buss->transfer(adr);
    dev.Buss->transfer(daa);
    digitalWrite(dev.cs_Line, HIGH); //pull ss high to signify end of data transfer
    dev.Buss->endTransaction();      // release buss for the rest of the system
}

uint8_t spiReadSingleADXL(SPI_DEVICE dev, uint8_t adr)
{
    uint8_t dat[2] = {0};
    uint8_t IN[2] = {0};
    IN[0] = adr | 0b11000000;                                                             //signafies a single read for the adxl chip
    dev.Buss->beginTransaction(SPISettings(dev.spi_clock_speed, MSBFIRST, dev.spi_mode)); //sdi rising, sdo falling
    digitalWrite(dev.cs_Line, LOW);                                                       //pull SS slow to prep other end for transfer
    dev.Buss->transferBytes(IN, dat, 2);
    digitalWrite(dev.cs_Line, HIGH); //pull ss high to signify end of data transfer
    dev.Buss->endTransaction();
    return dat[1];
}

accbuffer emptyAdxlBuffer(SPI_DEVICE dev)
{
    uint8_t outBuff[2] = {0};
    uint8_t IN[2] = {0};
    IN[0] = 0x39 | 0b11000000;
    int AccReadBufLen = 7;
    accbuffer buf;

    dev.Buss->beginTransaction(SPISettings(dev.spi_clock_speed, MSBFIRST, dev.spi_mode));
    digitalWrite(dev.cs_Line, LOW); //pull SS slow to prep other end for transfer
    dev.Buss->transferBytes(IN, outBuff, 2);
    digitalWrite(dev.cs_Line, HIGH);
    uint8_t fifolength = outBuff[1];
    buf.lenght = fifolength;

    for (int v = 0; v < fifolength; v++)
    {
        uint8_t dat2[AccReadBufLen] = {0};
        uint8_t IN2[AccReadBufLen] = {0};
        IN2[0] = 0x32 | 0b11000000; // read continuously starting at register 32 for AccReadBufLen-1   registers
        digitalWrite(dev.cs_Line, LOW);
        dev.Buss->transferBytes(IN2, dat2, AccReadBufLen);
        digitalWrite(dev.cs_Line, HIGH);

        acc accx;
        accx.x = (short)((dat2[2] << 8) + (dat2[1]));
        accx.y = (short)((dat2[4] << 8) + (dat2[3]));
        accx.z = (short)((dat2[6] << 8) + (dat2[5]));
        buf.buffer[v] = accx;
        delayMicroseconds(3); //minimum time between end of last read and start of next read is 5 us,
        //testing on an osciloxscope showed an extra 1 us on each side of the high pulse
    }
    digitalWrite(dev.cs_Line, HIGH);
    dev.Buss->endTransaction();
    return buf;
}

ADXLbuffer::ADXLbuffer(size_t len)
{
    this->maxsize = len;
}
