#ifndef WEB_SERVER_ENCRYPTION_HPP
#define WEB_SERVER_ENCRYPTION_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

class SHA1 {
public:
  SHA1() { reset(); }

  void update(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      buffer[bufflen++] = data[i];
      bitlen += 8;
      if (bufflen == 64) {
        transform(buffer);
        bufflen = 0;
      }
    }
  }

  void final(uint8_t hash[20]) {
    buffer[bufflen++] = 0x80;
    if (bufflen > 56) {
      while (bufflen < 64) buffer[bufflen++] = 0;
      transform(buffer);
      bufflen = 0;
    }
    while (bufflen < 56) buffer[bufflen++] = 0;
    for (int i = 7; i >= 0; --i)
      buffer[bufflen++] = static_cast<uint8_t>((bitlen >> (i*8)) & 0xFF);
    transform(buffer);

    for (int i = 0; i < 5; ++i) {
      hash[i*4]   = (state[i] >> 24) & 0xFF;
      hash[i*4+1] = (state[i] >> 16) & 0xFF;
      hash[i*4+2] = (state[i] >> 8 ) & 0xFF;
      hash[i*4+3] = (state[i]      ) & 0xFF;
    }
  }

private:
  uint32_t state[5];
  uint8_t  buffer[64];
  uint64_t bitlen;
  size_t   bufflen;

  void reset() {
    state[0] = 0x67452301;
    state[1] = 0xEFCDAB89;
    state[2] = 0x98BADCFE;
    state[3] = 0x10325476;
    state[4] = 0xC3D2E1F0;
    bitlen = 0;
    bufflen = 0;
  }

  static uint32_t rol(uint32_t v, unsigned int bits) {
    return (v << bits) | (v >> (32 - bits));
  }

  void transform(const uint8_t chunk[64]) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
      w[i] = (chunk[i*4] << 24)
             | (chunk[i*4+1] << 16)
             | (chunk[i*4+2] << 8)
             | (chunk[i*4+3]);
    }
    for (int i = 16; i < 80; ++i)
      w[i] = rol(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);

    uint32_t a = state[0], b = state[1], c = state[2],
             d = state[3], e = state[4];

    for (int i = 0; i < 80; ++i) {
      uint32_t f, k;
      if      (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
      else if (i < 40) { f = b ^ c ^ d;           k = 0x6ED9EBA1; }
      else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
      else             { f = b ^ c ^ d;           k = 0xCA62C1D6; }

      uint32_t temp = rol(a,5) + f + e + k + w[i];
      e = d; d = c; c = rol(b,30); b = a; a = temp;
    }

    state[0] += a; state[1] += b;
    state[2] += c; state[3] += d;
    state[4] += e;
  }
};

static const char* B64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline std::string base64_encode(const std::vector<uint8_t>& data) {
  std::string out;
  int val = 0, valb = -6;
  for (uint8_t c : data) {
    val = (val << 8) | c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(B64_CHARS[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    out.push_back(B64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (out.size() % 4) {
    out.push_back('=');
  }
  return out;
}

inline std::string compute_accept_key(const std::string& client_key) {
  static const std::string GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  std::string msg = client_key + GUID;

  SHA1 sha;
  sha.update(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
  uint8_t digest[20];
  sha.final(digest);

  std::vector<uint8_t> bytes(digest, digest+20);
  return base64_encode(bytes);
}



#endif // WEB_SERVER_ENCRYPTION_HPP
