//
// A C++ Redis client that wraps hiredis
//

#ifndef YCSB_C_REDIS_CLIENT_H_
#define YCSB_C_REDIS_CLIENT_H_

#include <iostream>
#include <string>
#include <hiredis.h>

namespace ycsbc {

class RedisClient {
 public:
  RedisClient(const char *host, int port, int slaves);
  ~RedisClient();

  int Command(std::string cmd);
  redisReply *CommandArgv(int argc, const char **argv,
                          const size_t *argvlen);

  redisContext *context() { return context_; }
 private:
  void HandleError(redisReply *reply, const char *hint);

  redisContext *context_;
  int slaves_;
};

//
// Implementation
//
inline RedisClient::RedisClient(const char *host, int port, int slaves) :
    slaves_(slaves) {
  context_ = redisConnect(host, port);
  if (!context_ || context_->err) {
    if (context_) {
      std::cerr << "Connect error: " << context_->errstr << std::endl;
      redisFree(context_);
    } else {
      std::cerr << "Connect error: can't allocate redis context!" << std::endl;
    }
    exit(1);
  }
}

inline RedisClient::~RedisClient() {
  if (context_) {
    redisFree(context_);
  }
}

inline int RedisClient::Command(std::string cmd) {
  redisReply *reply = nullptr;
  if (redisAppendCommand(context_, cmd.data()) == REDIS_ERR) {
    HandleError(reply, cmd.c_str());
  }
  if (slaves_ &&
      redisAppendCommand(context_, "WAIT %d %d", slaves_, 0) == REDIS_ERR) {
    HandleError(reply, "WAIT");
  }
  if (redisGetReply(context_, (void **)&reply) == REDIS_ERR) {
    HandleError(reply, cmd.c_str());
  }
  freeReplyObject(reply);
  if (slaves_) {
    reply = nullptr;
    if (redisGetReply(context_, (void **)&reply) == REDIS_ERR) {
      HandleError(reply, "WAIT");
    }
    freeReplyObject(reply);
  }
  return 0;
}

inline redisReply *RedisClient::CommandArgv(int argc, const char **argv,
                                             const size_t *argvlen) {
  redisReply *reply = static_cast<redisReply *>(
      redisCommandArgv(context_, argc, argv, argvlen));
  if (!reply) {
    HandleError(reply, "Redis command");
  }
  return reply;
}

inline void RedisClient::HandleError(redisReply *reply, const char *hint) {
  std::cerr << hint << " error: " << context_->errstr << std::endl;
  if (reply) freeReplyObject(reply);
  redisFree(context_);
  context_ = nullptr;
  exit(2);
}

} // namespace ycsbc

#endif // YCSB_C_REDIS_CLIENT_H_
