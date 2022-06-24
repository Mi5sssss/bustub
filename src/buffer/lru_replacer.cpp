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

#include "buffer/lru_replacer.h"

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) {
    dummyHead = new ListNode(0);
    dummyTail = new ListNode(0);
    dummyHead->next = dummyTail;
    dummyTail->prev = dummyHead;

    this->capacity = num_pages;
    this->size = 0;
}

// LRUReplacer::~LRUReplacer() = default;
LRUReplacer::~LRUReplacer() {
    ListNode *next;
    ListNode *cur = dummyHead;
    for (int i = 0; i < size + 2; i++){
        next = cur->next;
        delete cur;
        cur = next;
    }
}



bool LRUReplacer::Victim(frame_id_t *frame_id) {
    // Remove the object that was accessed the least recently compared to all 
    // the elements being tracked by the Replacer, store its contents in the output
    // parameter and return True. If the Replacer is empty return False.
    lru_latch_.lock();
    frame_id_t f_id = getTail();
    if (f_id == -1) {
        frame_id = nullptr;
        lru_latch_.unlock();
        return false;
    }
    *frame_id = f_id;
    deleteFrame(f_id);
    lru_latch_.unlock();
    return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
    // This method should be called after a page is pinned to a frame in the 
    // BufferPoolManager. It should remove the frame containing the pinned 
    // page from the LRUReplacer.
    lru_latch_.lock();
    deleteFrame(frame_id);
    lru_latch_.unlock();
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
    lru_latch_.lock();
    put(frame_id);
    lru_latch_.unlock();
}

size_t LRUReplacer::Size() { return 0; }

// The following functions were implemented.

void LRUReplacer::moveToHead(ListNode *node){
    node->prev->next = node->next;
    node->next->prev = node->prev;

    dummyHead->next->prev = node;
    dummyHead->next = node;
    node->next = dummyHead->next;
    node->prev = dummyHead;
}

void LRUReplacer::addToHead(ListNode *node){
    dummyHead->next->prev = node;
    dummyHead->next = node;
    node->next = dummyHead->next;
    node->prev = dummyHead;
}

void LRUReplacer::removeNode(ListNode *node){
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

void LRUReplacer::put(frame_id_t frame_id) {
    if(cache.find(frame_id) == cache.end()){ //Can not find the frame_id in cache
        ListNode *node = new ListNode(frame_id);
        cache[frame_id] = node;
        addToHead(node);
        size++;
    }
}

void LRUReplacer::deleteFrame(frame_id_t frame_id) {
    if(cache.find(frame_id) != cache.end()){ //If frame_id was found in cache
        ListNode *node = cache[frame_id];
        removeNode(node);
        size--;
    }
}

  frame_id_t LRUReplacer::getTail() {
    return dummyTail->frame_id;
  }
}  // namespace bustub

