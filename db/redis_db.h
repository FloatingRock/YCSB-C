//
//  redis_db.h
//  YCSB-C
//

#ifndef YCSB_C_REDIS_DB_H_
#define YCSB_C_REDIS_DB_H_

#include "core/db.h"

#include <iostream>
#include <string>
#include "core/properties.h"
#include "redis/redis_client.h"
#include <hiredis.h>

using std::cout;
using std::endl;

namespace ycsbc {

class RedisDB : public DB {
 public:
  RedisDB(const char *host, int port, int slaves) :
      host_(host), port_(port), slaves_(slaves) {
  }

  void Close() override;

  int Read(const std::string &table, const std::string &key,
           const std::vector<std::string> *fields,
           std::vector<KVPair> &result);

  int Scan(const std::string &table, const std::string &key,
           int len, const std::vector<std::string> *fields,
           std::vector<std::vector<KVPair>> &result) {
    throw "Scan: function not implemented!";
  }

  int Update(const std::string &table, const std::string &key,
             std::vector<KVPair> &values);

  int Insert(const std::string &table, const std::string &key,
             std::vector<KVPair> &values) {
    return Update(table, key, values);
  }

  int Delete(const std::string &table, const std::string &key) {
    std::string cmd("DEL " + key);
    GetRedisClient()->Command(cmd);
    return DB::kOK;
  }

 private:
  // Returns the calling thread's hiredis context, creating it on first use.
  // Each benchmark thread talks to the server over its own TCP connection
  // instead of sharing a single context, so transactions from different
  // threads no longer serialize on one connection.
  RedisClient *GetRedisClient();

  std::string host_;
  int port_;
  int slaves_;
  static thread_local RedisClient *tls_client_;
};

} // ycsbc

#endif // YCSB_C_REDIS_DB_H_

