//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// parallel_buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/parallel_buffer_pool_manager.h"
#include "buffer/buffer_pool_manager_instance.h"

namespace bustub {

ParallelBufferPoolManager::ParallelBufferPoolManager(size_t num_instances, size_t pool_size, DiskManager *disk_manager,
                                                     LogManager *log_manager) {
  // Allocate and create individual BufferPoolManagerInstances
  size_t index = 0;
  managers_ = new BufferPoolManagerInstance *[static_cast<int>(num_instances)]; // Not sure about this, managers_ is the pointer of the pointer.
  while (index < num_instances) {
    BufferPoolManagerInstance *manager =
      new BufferPoolManagerInstance(pool_size, num_instances, index, disk_manager, log_manager); // Why didn't disk_manager and log_manager be the pointer?
      
      // &managers_[index] = manager;
      *(managers_ + index) = manager; // manager is a pointer, placed in the managers_[index].
      index++;
  }
  num_instances_ = num_instances;
  pool_size_ = pool_size;
  next_instance_ = 0;
}

// Update constructor to destruct all BufferPoolManagerInstances and deallocate any associated memory
ParallelBufferPoolManager::~ParallelBufferPoolManager() {
  delete[] managers_;
}

size_t ParallelBufferPoolManager::GetPoolSize() {
  // Get size of all BufferPoolManagerInstances
  return num_instances_ * pool_size_;
}

BufferPoolManager *ParallelBufferPoolManager::GetBufferPoolManager(page_id_t page_id) {
  // Get BufferPoolManager responsible for handling given page id. You can use this method in your other methods.
  return *(managers_ + page_id % num_instances_); // Get the pointer of the buffer pool.
}

Page *ParallelBufferPoolManager::FetchPgImp(page_id_t page_id) {
  // Fetch page for page_id from responsible BufferPoolManagerInstance
  return GetBufferPoolManager(page_id)->FetchPage(page_id);
}

bool ParallelBufferPoolManager::UnpinPgImp(page_id_t page_id, bool is_dirty) {
  // Unpin page_id from responsible BufferPoolManagerInstance
  return GetBufferPoolManager(page_id)->UnpinPage(page_id, is_dirty);
}

bool ParallelBufferPoolManager::FlushPgImp(page_id_t page_id) {
  // Flush page_id from responsible BufferPoolManagerInstance
  return GetBufferPoolManager(page_id)->FlushPage(page_id);
}

Page *ParallelBufferPoolManager::NewPgImp(page_id_t *page_id) {
  // create new page. We will request page allocation in a round robin manner from the underlying
  // BufferPoolManagerInstances
  // 1.   From a starting index of the BPMIs, call NewPageImpl until either 1) success and return 2) looped around to
  // starting index and return nullptr
  // 2.   Bump the starting index (mod number of instances) to start search at a different BPMI each time this function
  // is called
  std::lock_guard<std::mutex> guard(latch_);
  size_t i = 0;
  while (i < num_instances_) {
    BufferPoolManager *manager = *(managers_ + next_instance_); // manager is the pointer of the next buffer pool.
    Page *page = manager->NewPage(page_id); // Use different manager in each round.
    next_instance_ = (next_instance_ + 1) % num_instances_; //next_instance_ is the page id.
    if (page != nullptr) {
      return page;
    }
    i++;
  }
  return nullptr;
}

bool ParallelBufferPoolManager::DeletePgImp(page_id_t page_id) {
  // Delete page_id from responsible BufferPoolManagerInstance
  return GetBufferPoolManager(page_id)->DeletePage(page_id);
}

void ParallelBufferPoolManager::FlushAllPgsImp() {
  // flush all pages from all BufferPoolManagerInstances
  size_t i = 0;
  while (i < num_instances_) {
    BufferPoolManager *manager = *(managers_ + i);
    manager->FlushAllPages();
    i++;
  }
}

}  // namespace bustub
