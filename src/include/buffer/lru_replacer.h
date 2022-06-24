//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.h
//
// Identification: src/include/buffer/lru_replacer.h
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <list>
#include <mutex>  // NOLINT
#include <vector>
#include <functional>

#include "buffer/replacer.h"
#include "common/config.h"


namespace bustub {

/**
 * LRUReplacer implements the Least Recently Used replacement policy.
 */
class LRUReplacer : public Replacer {
  // using mutex_t = std::mutex;

 public:
  /**
   * Create a new LRUReplacer.
   * @param num_pages the maximum number of pages the LRUReplacer will be required to store
   */
  explicit LRUReplacer(size_t num_pages);

  /**
   * Destroys the LRUReplacer.
   */
  ~LRUReplacer() override;

  bool Victim(frame_id_t *frame_id) override;

  void Pin(frame_id_t frame_id) override;

  void Unpin(frame_id_t frame_id) override;

  size_t Size() override;

 private:
  // TODO(student): implement me!
  struct ListNode{
    frame_id_t frame_id{0};
    struct ListNode *prev{nullptr};
    struct ListNode *next{nullptr};
    explicit ListNode(frame_id_t frame_id):frame_id(frame_id){}
  };
  struct ListNode *dummyHead;
  struct ListNode *dummyTail;
  int capacity;
  int size;
  
  std::mutex lru_latch_;
  std::unordered_map<frame_id_t, ListNode *> cache{};
  void moveToHead(ListNode *node);
  void addToHead(ListNode *node);
  void removeNode(ListNode *node);
  frame_id_t getTail();
  void put(frame_id_t frame_id);
  void deleteFrame(frame_id_t frame_id);

};

}  // namespace bustub

