#define PAGE_SIZE 64
#define BASE_ADDR ((uint8_t*) 0x1000)
#define CONFIG_ADDR (BASE_ADDR - PAGE_SIZE)

struct config_s {
    uint32_t version;
    uint8_t grp;
    uint8_t nid;
    uint8_t spare[2];
    uint8_t key[16];
    uint16_t sw_id;
    uint16_t sw_size;
    uint16_t sw_check;
    uint16_t check;
};

static void *memcpy(void *dst, const void *src, int len)
{
    uint8_t *to = (uint8_t *) dst;
    const uint8_t *from = (const uint8_t *) src;
    while (--len >= 0)
        *to++ = *from++;
    return dst;
}

static void config_load(struct config_s *cfg)
{
    memcpy(cfg, CONFIG_ADDR, sizeof(struct config_s));
    printf("[cfg] grp: %u\n", cfg->grp);
    printf("[cfg] nid: %u\n", cfg->nid);
}
