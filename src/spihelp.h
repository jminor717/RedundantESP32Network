#include <SPI.h>

class SPI_DEVICE
{
public:
    SPIClass *Buss = NULL;
    uint8_t cs_Line = 0;
    uint8_t spi_mode = 3;
    uint32_t spi_clock_speed = 2000000;
};

class ADXL345_SPI : public SPI_DEVICE
{
public:
    static const uint8_t ADXL345_BW_RATE = 0x2C;
    static const uint8_t ADXL345_POWER_CTL = 0x2D;
    static const uint8_t ADXL345_INT_ENABLE = 0x2E;
    static const uint8_t ADXL345_INT_MAP = 0X2F;
    static const uint8_t ADXL345_INT_SOURCE = 0x30;
    static const uint8_t ADXL345_FIFO_CTL = 0x38;
    static const uint8_t ADXL345_DATA_FORMAT = 0x31;
    static const uint8_t FIFO_STATUS = 0x39;
    ADXL345_SPI(SPIClass *, uint8_t);
    void init();
};
void spiWriteCustom(SPI_DEVICE *, uint8_t, uint8_t);
