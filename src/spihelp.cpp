#include <SPI.h>
#include <Arduino.h>

    class SPI_DEVICE
{
public:
    SPIClass *Buss = NULL;
    uint8_t cs_Line = 0;
    uint8_t spi_mode = 3;
    uint32_t spi_clock_speed = 2000000;
};

class ADXL345_SPI : SPI_DEVICE
{
public:
    static const int ADXL345_BW_RATE = 0x2C;
    static const int ADXL345_POWER_CTL = 0x2D;
    static const int ADXL345_INT_ENABLE = 0x2E;
    static const int ADXL345_INT_MAP = 0X2F;
    static const int ADXL345_INT_SOURCE = 0x30;
    static const int ADXL345_FIFO_CTL = 0x38;
    static const int ADXL345_DATA_FORMAT = 0x31;
    static const int FIFO_STATUS = 0x39;
    ADXL345_SPI(SPIClass * bus, uint8_t csLine){
        Buss = bus;
        cs_Line = csLine;
    }
    void init(){
        if (Buss == NULL){
            return;
        }
        spiWrite(*this, ADXL345_POWER_CTL, 0);
        delayMicroseconds(200);
        spiWrite(*this, ADXL345_POWER_CTL, 0b00001000);//place in measurement mode
        delayMicroseconds(200);
        spiWrite(*this, ADXL345_BW_RATE, 0b00001101);//800hz sample rate
        delayMicroseconds(200);
        spiWrite(*this, ADXL345_DATA_FORMAT, 0b00000000);//ensure defaults are active
        delayMicroseconds(200);
        spiWrite(*this, ADXL345_FIFO_CTL, 0);
        delayMicroseconds(200);
        spiWrite(*this, ADXL345_FIFO_CTL, 0b10111000); // (10|stream mode) (1|triger to INT2) (11000|trigger at 24 samples)
        delayMicroseconds(200);
        spiWrite(*this, ADXL345_INT_ENABLE, 0b00000011);// water mark and overrun intureupt enable
        delayMicroseconds(200);
        spiWrite(*this, ADXL345_INT_MAP, 0); // map interrupts to INT1
        delayMicroseconds(200);
    }
};

void spiWrite(SPI_DEVICE& dev, uint8_t adr, uint8_t daa)
{
    dev.Buss->beginTransaction(SPISettings(dev.spi_clock_speed, MSBFIRST, dev.spi_mode)); //sdi rising, sdo falling
    digitalWrite(dev.cs_Line, LOW);                                    //pull SS slow to prep other end for transfer
    dev.Buss->transfer(adr);
    dev.Buss->transfer(daa);
    digitalWrite(dev.cs_Line, HIGH); //pull ss high to signify end of data transfer
    dev.Buss->endTransaction();;
}