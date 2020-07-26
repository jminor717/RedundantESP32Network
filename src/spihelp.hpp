#include <SPI.h>
#include <atomic>
#include <queue>
#include <array>
#include <myConfig.hpp>

class SPI_DEVICE
{
public:
    SPIClass *Buss = NULL;
    uint8_t cs_Line = 0;
    uint8_t spi_mode = 3; //default for adxl
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
    std::atomic<bool> bufferFull{true};
    uint8_t interruptPin;
    ADXL345_SPI(SPIClass *, uint8_t, uint8_t);
    void init();
    /*
    called from interruptCallbackAZ/EL/Ballence
    *should not be called from anywhere else
    */
    void IRAM_ATTR interruptCallback();
};

struct acc
{
    short x;
    short y;
    short z;
};
struct accbuffer
{
    acc buffer[32];
    uint8_t lenght;
};

template <typename T, size_t N>
class CyclicArray
{
public:
    typedef typename std::array<T, N>::value_type value_type;
    typedef typename std::array<T, N>::reference reference;
    typedef typename std::array<T, N>::const_reference const_reference;
    typedef typename std::array<T, N>::size_type size_type;

    ~CyclicArray()
    {
        while (size())
            pop_front();
    }

    void push_back(const T &v)
    {
        if (size_ + 1 > N)
            pop_front();
        new (&array_[(front_ + size_) % N]) T(v);
        ++size_;
    }

    void pop_front()
    {
        if (size_ < 1)
            throw;
        front().~T();
        ++front_;
        --size_;
        if (front_ >= N)
            front_ = 0;
    }

    const_reference front() const
    {
        return *static_cast<const T *>(&array_[front_]);
    }

    reference front()
    {
        return *reinterpret_cast<T *>(&array_[front_]);
    }

    size_type size() const
    {
        return size_;
    }

private:
    size_type front_ = 0;
    size_type size_ = 0;
    std::array<char[sizeof(T)], N> array_;
};

//push data from acc measurement thread pop in main thread use atomic bools to signal between threads
struct ADXLbuffer
{
    ADXLbuffer(size_t);
    std::atomic<bool> writing{false};
    std::atomic<bool> reading{false};
    std::atomic<bool> needsEmptied{false};
    size_t maxsize = ACC_BUFFER_SIZE;
    std::queue<acc, CyclicArray<acc, ACC_BUFFER_SIZE>> buffer;
};

//use in acc measurement thread only for when ADXLbuffer is being read by main thread
struct ADXLminibuffer
{
    ADXLminibuffer(size_t);
    size_t maxsize = 160;
    std::queue<acc, CyclicArray<acc, 160>> buffer;
};

void spiWriteSingleADXL(SPI_DEVICE, uint8_t, uint8_t);

uint8_t spiReadSingleADXL(SPI_DEVICE, uint8_t);
accbuffer emptyAdxlBuffer(SPI_DEVICE);
uint32_t calcTransitSize(ADXLbuffer *, ADXLbuffer *, ADXLbuffer *, int16_t, int16_t);
uint32_t prepairTransit(uint8_t *, uint32_t, ADXLbuffer *, ADXLbuffer *, ADXLbuffer *, int16_t *, int16_t, int16_t *, int16_t);