// //===----------------------------------------------------------------------===//
// //
// //                         BusTub
// //
// // lru_replacer.cpp
// //
// // Identification: src/buffer/lru_replacer.cpp
// //
// // Copyright (c) 2015-2019, Carnegie Mellon University Database Group
// //
// //===----------------------------------------------------------------------===//

// #include "buffer/lru_replacer.h"

// namespace bustub {

// LRUReplacer::LRUReplacer(size_t num_pages) {
//     dummyHead = new ListNode(0);
//     dummyTail = new ListNode(0);
//     dummyHead->next = dummyTail;
//     dummyTail->prev = dummyHead;

//     this->capacity = num_pages;
//     this->size = 0;
// }

// // LRUReplacer::~LRUReplacer() = default;
// LRUReplacer::~LRUReplacer() {
//     ListNode *next;
//     ListNode *cur = dummyHead;
//     for (int i = 0; i < size + 2; i++){
//         next = cur->next;
//         delete cur;
//         cur = next;
//     }
// }



// bool LRUReplacer::Victim(frame_id_t *frame_id) {
//     // Remove the object that was accessed the least recently compared to all 
//     // the elements being tracked by the Replacer, store its contents in the output
//     // parameter and return True. If the Replacer is empty return False.
//     lru_latch_.lock();
//     frame_id_t f_id = getTail();
//     // printf("f_id is %d \n", f_id);
//     if (f_id == -1) {
//         frame_id = nullptr;
//         lru_latch_.unlock();
//         return false;
//     }
//     else {
//         *frame_id = f_id;
//         deleteFrame(f_id);
//         lru_latch_.unlock();
//         return true;
//     }
// }

// void LRUReplacer::Pin(frame_id_t frame_id) {
//     // This method should be called after a page is pinned to a frame in the 
//     // BufferPoolManager. It should remove the frame containing the pinned 
//     // page from the LRUReplacer.
//     lru_latch_.lock();
//     // printf("frame_id is %d\n", frame_id);
//     // printf("f_id is %d\n", getTail());
//     deleteFrame(frame_id);
//     lru_latch_.unlock();
// }

// void LRUReplacer::Unpin(frame_id_t frame_id) {
//     // This method should be called when the `pin_count` of a page becomes 0.
//     // This method should add the frame containing the unpinned page to the `LRUReplacer`.
//     lru_latch_.lock();
//     // printf("unpin\n");
//     put(frame_id);
//     lru_latch_.unlock();
// }

// size_t LRUReplacer::Size() {
//     // This method returns the number of frames that are currently in the `LRUReplacer`.
//     return size;
// }

// // The following functions were implemented.

// void LRUReplacer::moveToHead(ListNode *node){
//     node->prev->next = node->next;
//     node->next->prev = node->prev;

//     node->next = dummyHead->next;
//     node->prev = dummyHead;

//     dummyHead->next->prev = node;
//     dummyHead->next = node;

// }

// void LRUReplacer::addToHead(ListNode *node){
//     node->next = dummyHead->next;
//     node->prev = dummyHead;
    
//     dummyHead->next->prev = node;
//     dummyHead->next = node;

// }

// void LRUReplacer::removeNode(ListNode *node){
    
//     dummyTail->prev = node->prev;
//     dummyTail = node->prev->next;

//     node->prev->next = node->next;
//     node->next->prev = node->prev;

// }

// void LRUReplacer::put(frame_id_t frame_id) {
//     auto iter = cache.find(frame_id);
//     if(iter == cache.end()){ //Can not find the frame_id in cache
//         // printf("Can not find the frame_id in cache\n");
//         ListNode *node = new ListNode(frame_id);
//         cache[frame_id] = node;
//         addToHead(node);
//         size++;
//     }
//     else{
//         // // printf("Can find the frame_id in cache\n");
//         // ListNode *node = cache[frame_id];
//         // moveToHead(node);
//         return;
//     }
// }

// void LRUReplacer::deleteFrame(frame_id_t frame_id) {
//     auto iter = cache.find(frame_id);
//     if(iter != cache.end()){ //If frame_id was found in cache
//         // printf("frame_id was found in cache\n");
//         ListNode *node = cache[frame_id];
//         removeNode(node);
//         // cache.erase(frame_id);
//         cache.erase(iter);
//         size--;
//     }
//     else{
//         // printf("frame_id was not found in cache\n");
//         return;
//         }
// }

//   frame_id_t LRUReplacer::getTail() {
//     return dummyTail->prev->frame_id;
//   }
// }  // namespace bustub

//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.cpp
//
// Identification: src/buffer/lru_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

// Using std
#include "buffer/lru_replacer.h"
#include <iostream>

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) {}

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
    // Victim(frame_id_t*) : Remove the object that was accessed least recently compared 
    // to all the other elements being tracked by the Replacer, store its contents in the 
    // output parameter and return True. If the Replacer is empty return False.
  std::lock_guard<std::mutex> guard(lru_latch_);
  if (lru_list_.empty()) {
    return false;
  }
  *frame_id = lru_list_.back();
  lru_map_.erase(*frame_id);
  lru_list_.pop_back();
  return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
    // Pin(frame_id_t) : This method should be called after a page is pinned to a frame 
    // in the BufferPoolManager. It should remove the frame containing the pinned page from the LRUReplacer.
  std::lock_guard<std::mutex> guard(lru_latch_);
  auto iter = lru_map_.find(frame_id);
  if (iter == lru_map_.end()) {
    return;
  }

  lru_list_.erase(iter->second);
  lru_map_.erase(iter);
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
    // Unpin(frame_id_t) : This method should be called when the pin_count of 
    // a page becomes 0. This method should add the frame containing the unpinned page to the LRUReplacer.
  std::lock_guard<std::mutex> guard(lru_latch_);
  if (lru_map_.find(frame_id) != lru_map_.end()) {
    return;
  }
  lru_list_.push_front(frame_id);
  lru_map_[frame_id] = lru_list_.begin();
}

size_t LRUReplacer::Size() {
    // Size() : This method returns the number of frames that are currently in the LRUReplacer.
  std::lock_guard<std::mutex> guard(lru_latch_);
  return lru_list_.size();
}

}  // namespace bustub
