#define HTU21D_ADDRESS 0x40
#define HTU21D_CMD_TEMP_HOLD 0xE3
#define HTU21D_CMD_HUMID_HOLD 0xE5
#define HTU21D_CMD_TEMP_NO_HOLD 0xF3
#define HTU21D_CMD_HUMID_NO_HOLD 0xF5
#define HTU21D_CMD_READ_USER 0xE7
#define HTU21D_CMD_SOFT_RESET 0xFE
#define HTU21D_SAMPLE_ERR 0xFFFF

#define VCNL4000_ADDRESS 0x13
#define VCNL4000_CMD_READ_PID 0x81

struct i2c_s {
    I2C_HANDLE_T *handle;
    uint32_t mem[24];
    ErrorCode_t err_code;
    uint8_t ready;
};

struct i2c_slaves_s {
    uint8_t addr;
    const char *name;
};

