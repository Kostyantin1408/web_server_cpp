#ifndef WEB_SERVER_ENCRYPTION_HPP
#define WEB_SERVER_ENCRYPTION_HPP

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include "sha1.hpp"
#include <cstring>

static constexpr std::string_view WS_MAGIC =
    "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

inline std::string make_websocket_accept(const std::string& clientKey)
{
  Chocobo1::SHA1 sha;
  sha.addData(
      reinterpret_cast<const Chocobo1::SHA1::Byte*>(clientKey.data()),
      clientKey.size()
  );
  sha.addData(
      reinterpret_cast<const Chocobo1::SHA1::Byte*>(WS_MAGIC.data()),
      WS_MAGIC.size()
  );
  sha.finalize();
  auto digest = sha.toVector();
  return Chocobo1::base64_encode(
      std::string_view{
          reinterpret_cast<const char*>(digest.data()),
          digest.size()
      }
  );
}


#endif // WEB_SERVER_ENCRYPTION_HPP
