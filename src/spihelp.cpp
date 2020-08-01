#include <SPI.h>
#include <Arduino.h>
#include <spihelp.hpp>
#include <map>
#include <myConfig.hpp>

std::map<int, ADXL345_SPI *> callbacks;

SPIClass *SecondarySPI = NULL; // secondary, mostly unused by library code so free to use for this code

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
    callbacks[this->interruptPin] = this;
    switch (this->interruptPin)
    {
    case AdxlInt_AZ:
        attachInterrupt(this->interruptPin, interruptCallbackAZ, RISING);
        break;
    case AdxlInt_EL:
        attachInterrupt(this->interruptPin, interruptCallbackEL, RISING);
        break;
    case AdxlInt_Ballence:
        attachInterrupt(this->interruptPin, interruptCallbackBallence, RISING);
        break;
    default:
        break;
    }
}

ADXL345_SPI::ADXL345_SPI(uint8_t csLine, uint8_t inturuptLine)
{
    this->Buss = SecondarySPI;
    this->cs_Line = csLine;
    this->interruptPin = inturuptLine;
}

void IRAM_ATTR ADXL345_SPI::interruptCallback()
{
    this->bufferFull = true;
}

// idealy this should be called between the constructor and init
//if constructor using SPIClass is called this method should not be
void ADXL345_SPI::setupIntrupts()
{
    pinMode(this->interruptPin, INPUT_PULLUP);
    //callbacks[interruptPin] = this->interruptCallback();
    callbacks[this->interruptPin] = this;
    switch (this->interruptPin)
    {
    case AdxlInt_AZ:
        attachInterrupt(this->interruptPin, interruptCallbackAZ, RISING);
        break;
    case AdxlInt_EL:
        attachInterrupt(this->interruptPin, interruptCallbackEL, RISING);
        break;
    case AdxlInt_Ballence:
        attachInterrupt(this->interruptPin, interruptCallbackBallence, RISING);
        break;
    default:
        break;
    }
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


//this should be called before constructing any devices that use the spi buss
void initilizeSpiBuss()
{
    SecondarySPI = new SPIClass(HSPI);
    //SCLK = 14, MISO = 12, MOSI = 13, SS = 15
    SecondarySPI->begin(SecondarySPI_SCLK, SecondarySPI_MISO, SecondarySPI_MOSI, SecondarySPI_SS);
    /*
    accSPI_EL = new ADXL345_SPI(SecondarySPI, AdxlSS_AZ, AdxlInt_AZ);
    accSPI_EL->init();

    accSPI_AZ = new ADXL345_SPI(SecondarySPI, AdxlSS_EL, AdxlInt_EL);
    accSPI_AZ->init();

    accSPI_Ballence = new ADXL345_SPI(SecondarySPI, AdxlSS_Ballence, AdxlInt_Ballence);
    accSPI_Ballence->init();
    accSPI_Ballence->spi_clock_speed = 200000;
*/
}

/**
 * 
 * 
 * @param dev
*/
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
    IN[0] = 0x39 | 0b11000000; //fifo sample buffer size location
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
//retune number of 8 bit chars required to transmit the presented data
uint32_t calcTransitSize(ADXLbuffer *accDatAZ, ADXLbuffer *accDatEL, ADXLbuffer *accDatBAL, int16_t AZTempSize, int16_t ELTempSize)
{
    uint32_t length = 1 + 10 + 4; //identifier + [16|ACCcountAZ,16|ACCcountEL,16|ACCcountBAL,16|AZetmpCount,16|ELetmpCount] + total data length
    length += (accDatAZ->buffer.size() * 6);
    length += (accDatEL->buffer.size() * 6);
    length += (accDatBAL->buffer.size() * 6);
    length += (AZTempSize * 2);
    length += (ELTempSize * 2);
    return length;
}
uint32_t prepairTransit(uint8_t *reply, uint32_t dataSize, ADXLbuffer *accDatAZ, ADXLbuffer *accDatEL, ADXLbuffer *accDatBAL, int16_t *AZTempDat, int16_t AZTempSize, int16_t *ELTempDat, int16_t ELTempSize)
{
    //[16|ACCcount,16|AZetmpCount,16|ELetmpCount]
    //acc data length = accDat.buffer.size() * 6
    uint32_t i = 0;
    reply[0] = DATA_TRANSMIT_ID;
    reply[1] = (dataSize & 0xff000000) >> 24;
    reply[2] = (dataSize & 0x00ff0000) >> 16;
    reply[3] = (dataSize & 0x0000ff00) >> 8;
    reply[4] = dataSize & 0x000000ff;

    uint32_t accBufSixeAZ = accDatAZ->buffer.size();
    reply[5] = ((accBufSixeAZ * 6) & 0xff00) >> 8;
    reply[6] = ((accBufSixeAZ * 6) & 0x00ff);
    uint32_t accBufSixeEL = accDatEL->buffer.size();
    reply[7] = ((accBufSixeEL * 6) & 0xff00) >> 8;
    reply[8] = ((accBufSixeEL * 6) & 0x00ff);
    uint32_t accBufSixeBAL = accDatBAL->buffer.size();
    reply[9] = ((accBufSixeBAL * 6) & 0xff00) >> 8;
    reply[10] = ((accBufSixeBAL * 6) & 0x00ff);

    reply[11] = (AZTempSize & 0xff00) >> 8;
    reply[12] = (AZTempSize & 0x00ff);
    reply[13] = (ELTempSize & 0xff00) >> 8;
    reply[14] = (ELTempSize & 0x00ff);
    i = 15;
    for (uint32_t j = 0; j < accBufSixeAZ; j++)
    {
        acc current = accDatAZ->buffer.front();
        reply[i++] = (current.x & 0x00ff);
        reply[i++] = (current.x & 0xff00) >> 8;
        reply[i++] = (current.y & 0x00ff);
        reply[i++] = (current.y & 0xff00) >> 8;
        reply[i++] = (current.z & 0x00ff);
        reply[i++] = (current.z & 0xff00) >> 8;
        accDatAZ->buffer.pop();
    }
    for (uint32_t j = 0; j < accBufSixeEL; j++)
    {
        acc current = accDatEL->buffer.front();
        reply[i++] = (current.x & 0x00ff);
        reply[i++] = (current.x & 0xff00) >> 8;
        reply[i++] = (current.y & 0x00ff);
        reply[i++] = (current.y & 0xff00) >> 8;
        reply[i++] = (current.z & 0x00ff);
        reply[i++] = (current.z & 0xff00) >> 8;
        accDatEL->buffer.pop();
    }
    for (uint32_t j = 0; j < accBufSixeBAL; j++)
    {
        acc current = accDatBAL->buffer.front();
        reply[i++] = (current.x & 0x00ff);
        reply[i++] = (current.x & 0xff00) >> 8;
        reply[i++] = (current.y & 0x00ff);
        reply[i++] = (current.y & 0xff00) >> 8;
        reply[i++] = (current.z & 0x00ff);
        reply[i++] = (current.z & 0xff00) >> 8;
        accDatBAL->buffer.pop();
    }

    for (uint32_t j = 0; j < AZTempSize; j++)
    {
        reply[i++] = (AZTempDat[j] & 0x00ff);
        reply[i++] = (AZTempDat[j] & 0xff00) >> 8;
    }
    for (uint32_t j = 0; j < ELTempSize; j++)
    {
        reply[i++] = (ELTempDat[j] & 0x00ff);
        reply[i++] = (ELTempDat[j] & 0xff00) >> 8;
    }
    return i;
}