#include <SPI.h>
#include <Arduino.h>
#include <spihelp.h>

ADXL345_SPI::ADXL345_SPI(SPIClass *bus, uint8_t csLine)
{
    Buss = bus;
    cs_Line = csLine;
}

void ADXL345_SPI::init()
{
    if (Buss == NULL)
    {
        return;
    }
    spiWriteCustom((SPI_DEVICE *)this, ADXL345_POWER_CTL, (uint8_t)0);
    delayMicroseconds(200);
    spiWriteCustom((SPI_DEVICE *)this, ADXL345_POWER_CTL, (uint8_t)0b00001000); //place in measurement mode
    delayMicroseconds(200);
    spiWriteCustom((SPI_DEVICE *)this, ADXL345_BW_RATE, (uint8_t)0b00001101); //800hz sample rate
    delayMicroseconds(200);
    spiWriteCustom((SPI_DEVICE *)this, ADXL345_DATA_FORMAT, (uint8_t)0b00000000); //ensure defaults are active
    delayMicroseconds(200);
    spiWriteCustom((SPI_DEVICE *)this, ADXL345_FIFO_CTL, (uint8_t)0);
    delayMicroseconds(200);
    spiWriteCustom((SPI_DEVICE *)this, ADXL345_FIFO_CTL, (uint8_t)0b10111000); // (10|stream mode) (1|triger to INT2) (11000|trigger at 24 samples)
    delayMicroseconds(200);
    spiWriteCustom((SPI_DEVICE *)this, ADXL345_INT_ENABLE, (uint8_t)0b00000011); // water mark and overrun intureupt enable
    delayMicroseconds(200);
    spiWriteCustom((SPI_DEVICE *)this, ADXL345_INT_MAP, (uint8_t)0); // map interrupts to INT1
    delayMicroseconds(200);
}

void spiWriteCustom(SPI_DEVICE &dev, uint8_t adr, uint8_t daa)
{
    dev.Buss->beginTransaction(SPISettings(dev.spi_clock_speed, MSBFIRST, dev.spi_mode)); //sdi rising, sdo falling
    digitalWrite(dev.cs_Line, LOW);                                                       //pull SS slow to prep other end for transfer
    dev.Buss->transfer(adr);
    dev.Buss->transfer(daa);
    digitalWrite(dev.cs_Line, HIGH); //pull ss high to signify end of data transfer
    dev.Buss->endTransaction();
}
