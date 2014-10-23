#define BOOT_DATA_MAX 64

#include "packet.h"

#define PAGE_SIZE 64
#define SECTOR_SIZE (PAGE_SIZE * 16)
#define BASE_ADDR ((uint8_t*) 0x1000)
#define TOP_OF_BOOT BASE_ADDR
#define CONFIG_ADDR (TOP_OF_BOOT - PAGE_SIZE)

static void* memset(void* dst, uint8_t fill, int len) {
  uint8_t* to = (uint8_t*) dst;
  while (--len >= 0)
    *to++ = fill;
  return dst;
}

static void* memcpy(void* dst, const void* src, int len) {
  uint8_t* to = (uint8_t*) dst;
  const uint8_t* from = (const uint8_t*) src;
  while (--len >= 0)
    *to++ = *from++;
  return dst;
}

static uint16_t calcCRC (const void* ptr, int len) {
  int crc = ~0;
  for (uint16_t i = 0; i < len; ++i)
    crc = _crc16_update(crc, ((const char*) ptr)[i]);
  printf("crc %04X\n", crc);
  return crc;
}

#ifndef dump
static void dump (const char* msg, const void* buf, int len) {
  printf("%s #%d:", msg, len);
  for (int i = 0; i < len; ++i) {
    if (i % 32 == 0)
      printf("\n ");
    if (i % 4 == 0)
      printf(" ");
    printf("%02X", ((const uint8_t*) buf)[i]);
  }
  printf("\n");
}
#endif

// return 1 if good reply, 0 if crc error, -1 if timeout
static int sendRequest (const void* buf, int len, int hdrOr) {
  dump("send", buf, len);
  // printf("sending %d b\n", len);
  rf12_sendNow(RF12_HDR_CTL | RF12_HDR_ACK | hdrOr, buf, len);
  rf12_sendWait(0);
  uint32_t now = millis();
  while (!rf12_recvDone() || rf12_len == 0) // TODO: 0-check to avoid std acks?
    if ((millis() - now) >= 250) {
      printf("timed out\n");
      return -1;
    }
  if (rf12_crc) {
    printf("bad crc %04X\n", rf12_crc);
    return 0;
  }
  printf("got %d b hdr 0x%X crc %X\n", rf12_len, rf12_hdr, rf12_crc);
  dump("recv", (const uint8_t*) rf12_data, rf12_len);
  return 1;
}

static void copyPageToFlash (void* ram, void* flash) {
  int page = (uint32_t) flash / PAGE_SIZE;
  int sect = (uint32_t) flash / SECTOR_SIZE;
  printf("ram 0x%X flash 0x%X page %d sect %d ",
          (int) ram, (int) flash, page, sect);
  iap_prepare_sector(sect, sect);
  int e1 = iap_erase_page(page, page);
  printf("iap erase %d,", e1); (void) e1;
  iap_prepare_sector(sect, sect);
  int e2 = iap_copy_ram_to_flash(ram, flash, PAGE_SIZE);
  printf(" flash %d\n", e2); (void) e2;
}

int backOffCounter;

struct Config {
  uint32_t version;
  uint8_t group;
  uint8_t nodeId;
  uint8_t spare [2];
  uint8_t shKey [16];
  uint16_t swId;
  uint16_t swSize;
  uint16_t swCheck;
  uint16_t check;
} config;

static void loadConfig () {
  memcpy(&config, CONFIG_ADDR, sizeof config);
  if (calcCRC(&config, sizeof config) != 0) {
    printf("default config\n");
    memset(&config, 0, sizeof config);
  }
}

static void saveConfig () {
  config.version = 1;
  if (calcCRC(&config, sizeof config) != 0) {
    config.check = calcCRC(&config, sizeof config - 2);
    printf("save config 0x%X\n", config.check);
    copyPageToFlash(&config, CONFIG_ADDR);
  }
}

static void sendPairingCheck () {
  struct PairingRequest request;
  request.type = REMOTE_TYPE;
  request.group = config.group;
  request.nodeId = config.nodeId;
  request.check = calcCRC(&config.shKey, sizeof config.shKey);
  memcpy(request.hwId, hwId, sizeof request.hwId);
  
  struct PairingReply reply;
  if (sendRequest(&request, sizeof request, RF12_HDR_DST) > 0 && rf12_len == sizeof reply) {
    memcpy(&reply, (const void*) rf12_data, sizeof reply);
    config.group = reply.group;
    config.nodeId = reply.nodeId;
    memcpy(config.shKey, reply.shKey, sizeof config.shKey);
    saveConfig();
    printf("paired group %d node %d\n", config.group, config.nodeId);
  }
}

// retry up to 16 times to get a response, this make take several hours due to
// the exponential back-off logic, which starts with a 250 ms timeout period
// it'll stick to the maximum 4.5 hour cycle once the limit has been reached
// without server, this'll listen for 250 ms up to 85x/day = 21 s = 0.25% duty

static void exponentialBackOff () {
  printf("wait %d\n", 250 << backOffCounter);
  sleep(250L << backOffCounter);
  if (backOffCounter < 16)
    ++backOffCounter;
}

static int appIsValid () {
  return calcCRC(BASE_ADDR, config.swSize << 4) == config.swCheck;
}

static int sendUpgradeCheck () {
  struct UpgradeRequest request;
  request.type = REMOTE_TYPE;
  request.swId = config.swId;
  request.swSize = config.swSize;
  request.swCheck = config.swCheck;
  struct UpgradeReply reply;
  if (sendRequest(&request, sizeof request, 0) > 0 && rf12_len == sizeof reply) {
    memcpy(&reply, (const void*) rf12_data, sizeof reply);
    // ...
    config.swId = reply.swId;
    config.swSize = reply.swSize;
    config.swCheck = reply.swCheck;
    saveConfig();
    return 1;
  }
  return 0;
}

static int sendDownloadRequest (int index) {
  struct DownloadRequest request;
  request.swId = config.swId;
  request.swIndex = index;
  struct DownloadReply reply;
  if (sendRequest(&request, sizeof request, 0) > 0 && rf12_len == sizeof reply) {
    for (int i = 0; i < BOOT_DATA_MAX; ++i)
      rf12_data[2+i] ^= 211 * i;
    // dump("de-whitened", (const void*) rf12_data, rf12_len);
    union { uint32_t longs[16]; uint8_t bytes[64]; } aligned;
    memcpy(aligned.bytes, (const void*) (rf12_data + 2), sizeof aligned);
    void* flash = BASE_ADDR + PAGE_SIZE * index;
    copyPageToFlash(aligned.bytes, flash);
    dump("in flash", flash, PAGE_SIZE);
    return 1;
  }
  return 0;
}

static void bootLoaderLogic () {
  printf("1\n");
  loadConfig();
  
  rf12_initialize(1, RF12_868MHZ, PAIRING_GROUP);

  printf("2\n");
  backOffCounter = 0;
  while (1) {
    sendPairingCheck();
    if (config.group != 0 && config.nodeId != 0) // paired
      break;
    exponentialBackOff();
  }
  
  rf12_initialize(config.nodeId, RF12_868MHZ, config.group);

  printf("3\n");
  backOffCounter = 0;
  do {
    if (sendUpgradeCheck())
      break;
    exponentialBackOff();
  } while (! appIsValid());
  
  printf("4\n");
  if (! appIsValid()) {
    int limit = ((config.swSize << 4) + PAGE_SIZE - 1) / PAGE_SIZE;
    for (int i = 0; i < limit; ++i) {
      backOffCounter = 0;
      while (sendDownloadRequest(i) == 0)
        exponentialBackOff();
    }
  }

  printf("5\n");
}

static void bootLoader () {
  sleep(20); // needed to make RFM69 work properly on power-up
  
  // this will not catch the runaway case when the server replies with data,
  // but the application that ends up in memory does not match the crc given
  // in this case, we'll constantly keep retrying... and drain the battery :(
  // to avoid this, an extra level of exponential back-off has been added here
  for (int backOff = 0; /*forever*/; ++backOff) {
    bootLoaderLogic();
    if (appIsValid())
      break;
    sleep(100L << (backOff & 0x0F));
  }
}
