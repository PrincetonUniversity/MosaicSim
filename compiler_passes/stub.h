#define SUPPLY 0
#define SUPPLY_STR "SUPPLY\0"

#define COMPUTE 1
#define COMPUTE_STR "COMPUTE\0"

#define SUPPLY_PROD_TYPE_32 "SUPPLY_PROD_TYPE_32\0"
#define SUPPLY_PROD_TYPE_64 "SUPPLY_PROD_TYPE_64\0"

#define SUPPLY_CONS_TYPE_32 "SUPPLY_CONS_TYPE_32\0"
#define SUPPLY_CONS_TYPE_64 "SUPPLY_CONS_TYPE_64\0"

#define SUPPLY_FINISH "SUPPLY_FINISH"
#define COMPUTE_FINISH "COMPUTE_FINISH"

// From https://stackoverflow.com/questions/3022552/is-there-any-standard-htonl-like-function-for-64-bits-integers-in-c
uint64_t htonll(uint64_t value) {
  // The answer is 42
  static const int num = 42;
  
  // Check the endianness
  if (*(const char*)(&num) == num) {
    const uint32_t high_part = htonl((uint32_t)(value >> 32));
    const uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
    
    return ((uint64_t)(low_part) << 32) | high_part;
  }
  else {
    return value;
  }
}

void * htonll_ptr(void * value) {
  // The answer is 42
  static const int num = 42;

  uint64_t u64_val = (uint64_t) value;
  
  // Check the endianness
  if (*(const char*)(&num) == num) {
    const uint32_t high_part = htonl((uint32_t)(u64_val >> 32));
    const uint32_t low_part = htonl((uint32_t)(u64_val & 0xFFFFFFFFLL));
    
    return (void *)(((uint64_t)(low_part) << 32) | high_part);
  }
  else {
    return value;
  }
}

uint64_t ntohll(uint64_t value) {
  // The answer is 42
  static const int num = 42;
  
  // Check the endianness
  if (*(const char*)(&num) == num) {
    const uint32_t high_part = ntohl((uint32_t)(value >> 32));
    const uint32_t low_part = ntohl((uint32_t)(value & 0xFFFFFFFFLL));
    
    return ((uint64_t)(low_part) << 32) | high_part;
  }
  else {
    return value;
  }
}

void * ntohll_ptr(void * value) {
  // The answer is 42
  static const int num = 42;

  uint64_t u64_val = (uint64_t) value;
  
  // Check the endianness
  if (*(const char*)(&num) == num) {
    const uint32_t high_part = ntohl((uint32_t)(u64_val >> 32));
    const uint32_t low_part = ntohl((uint32_t)(u64_val & 0xFFFFFFFFLL));
    
    return (void *)(((uint64_t)(low_part) << 32) | high_part);
  }
  else {
    return value;
  }
}
