// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// Static memory pool for packets

#include <memory>

#include <etl/pool.h>
#include <etl/vector.h>

class Packet {
 public:
  static constexpr size_t SIZE = 2000;
  etl::vector<char, SIZE> data;

 private:
  // Packet can only be created by the pool
  friend class etl::ipool;
  Packet()
  {
  }
};

class PacketPool {
 public:
  static constexpr size_t PACKET_POOL_SIZE = 10;

  using Pool = etl::pool<Packet, PACKET_POOL_SIZE>;

  // Get the singleton instance of the underlying pool
  static Pool* getPool()
  {
    static Pool pool;
    return &pool;
  }

  // Allocate a packet from the pool
  static std::shared_ptr<Packet> allocate()
  {
    Pool* pool = PacketPool::getPool();
    if(pool->available() == 0) {
      return nullptr;
    }

    return std::shared_ptr<Packet>(
        pool->create(), [](Packet* p) { PacketPool::getPool()->destroy(p); });
  }

  PacketPool(const PacketPool&) = delete;
  void operator=(const PacketPool&) = delete;

 private:
  PacketPool() = default;
};
