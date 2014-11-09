// Boot packet types, exchanged between remote nodes and the boot server.
// -jcw, 2013-11-17

struct PairingRequest {
  uint16_t type;      // type of this remote node, 100..999 freely available
  uint8_t group;      // current network group, 1..250 or 0 if unpaired
  uint8_t nodeId;     // current node ID, 1..30 or 0 if unpaired
  uint16_t check;     // crc checksum over the current shared key
  uint8_t hwId [16];  // unique hardware ID or 0's if not available
};

struct PairingReply {
  uint16_t type;      // type, same as in request
  uint8_t group;      // assigned network group
  uint8_t nodeId;     // assigned node ID
  uint8_t shKey [16]; // shared key or 0's if not used
};

struct UpgradeRequest {
  uint16_t type;      // type, same as in request
  uint16_t swId;      // current software ID or 0 if unknown
  uint16_t swSize;    // current software download size, in units of 16 bytes
  uint16_t swCheck;   // current crc checksum over entire download
};

struct UpgradeReply {
  uint16_t type;      // type, same as in request
  uint16_t swId;      // assigned software ID
  uint16_t swSize;    // software download size, in units of 16 bytes
  uint16_t swCheck;   // crc checksum over entire download
};

struct DownloadRequest {
  uint16_t swId;      // current software ID
  uint16_t swIndex;   // current download index, as multiple of payload size
};

struct DownloadReply {
  uint16_t swIdXor;   // current software ID xor current download index
  uint8_t data [BOOT_DATA_MAX]; // download payload
};
