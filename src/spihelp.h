#include <SPI.h>

void spiWrite(SPIClass*, uint8_t, uint8_t);


class SPI_DEVICE{
    SPIClass *Buss = NULL;
    uint8_t cs_Line = 0;
    uint8_t spi_mode = 3;
    uint32_t spi_clock_speed = 2000000;
};

class ADXL345_SPI : SPI_DEVICE
{
    ADXL345_SPI(SPIClass *, uint8_t);
    void init();
};