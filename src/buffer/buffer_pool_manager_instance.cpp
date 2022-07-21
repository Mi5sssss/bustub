//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager_instance.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager_instance.h"

#include "common/macros.h"

namespace bustub {

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, DiskManager *disk_manager,
                                                     LogManager *log_manager)
    : BufferPoolManagerInstance(pool_size, 1, 0, disk_manager, log_manager) {}

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, uint32_t num_instances, uint32_t instance_index,
                                                     DiskManager *disk_manager, LogManager *log_manager)
    : pool_size_(pool_size),
      num_instances_(num_instances),
      instance_index_(instance_index),
      next_page_id_(instance_index),
      disk_manager_(disk_manager),
      log_manager_(log_manager) {
  BUSTUB_ASSERT(num_instances > 0, "If BPI is not part of a pool, then the pool size should just be 1");
  BUSTUB_ASSERT(
      instance_index < num_instances,
      "BPI index cannot be greater than the number of BPIs in the pool. In non-parallel case, index should just be 1.");
  // We allocate a consecutive memory space for the buffer pool.
  pages_ = new Page[pool_size_];
  replacer_ = new LRUReplacer(pool_size);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManagerInstance::~BufferPoolManagerInstance() {
  delete[] pages_;
  delete replacer_;
}

bool BufferPoolManagerInstance::FlushPgImp(page_id_t page_id) {
  // Make sure you call DiskManager::WritePage!
  // latch_.lock(); // We could use manual lock or automatic lock.
  std::lock_guard<std::mutex> guard(latch_);
  auto iter = page_table_.find(page_id); //Set the iterator found by page_id in page_table_.
  if (iter == page_table_.end()){
    // Page is not be found.
    // latch_.unlock();
    return false;
  }

    // Page is found.
    frame_id_t frame_id = iter->second;
    Page *page = pages_ + frame_id; // Set the address of page.
    page->is_dirty_ = false;
    disk_manager_->WritePage(page_id, page->GetData()); //Write the page in the buffer into disk.
    // latch_.unlock();
    return true;

}

void BufferPoolManagerInstance::FlushAllPgsImp() {
  // You can do it!
  // latch_.lock();
  std::lock_guard<std::mutex> guard(latch_);
  for (auto iter = page_table_.begin(); 
      iter != page_table_.end(); 
      iter++){ // Use iterator to traverse the page table.
        Page *page = pages_ + iter->second;
        disk_manager_->WritePage(iter->first, page->GetData()); //Write the page in the buffer into disk.
  }
  // latch_.unlock();
}

Page *BufferPoolManagerInstance::NewPgImp(page_id_t *page_id) {
  // 0.   Make sure you call AllocatePage!
  // 1.   If all the pages in the buffer pool are pinned, return nullptr.
  // 2.   Pick a victim page P from either the free list or the replacer. Always pick from the free list first.
  // 3.   Update P's metadata, zero out memory and add P to the page table.
  // 4.   Set the page ID output parameter. Return a pointer to P.
  // latch_.lock();
  std::lock_guard<std::mutex> guard(latch_);
  Page *P = nullptr; // Initiate page P's address.
  frame_id_t frame_id = -1; // Initiate frame id as -1.

  if (!free_list_.empty()) { // If some pages are free.
    frame_id = free_list_.front(); // Get the page from free list.
    free_list_.pop_front(); // Delete the page in the free list.
    // P = &pages_[frame_id]; 
    P = pages_ + frame_id; // Set the P's pointer as the extracted page from the free list.
  }
  else if (replacer_->Victim(&frame_id)) { // If no page is free and page is as the replacer's victim.
    P = pages_ + frame_id; // Set the P's pointer as the extracted page from the free list.
    if (P->IsDirty()) { // If P has dirty flag, modified.
      disk_manager_->WritePage(P->GetPageId(), P->GetData()); // Write P back to the disk.
    }
    page_table_.erase(P->GetPageId()); // Remove P from page table.
  }

  if (P != nullptr) {
    // Initiate P for a new page.
    *page_id = AllocatePage();
    P->page_id_ = *page_id;
    P->is_dirty_ = false;
    P->pin_count_ = 1;
    P->ResetMemory();
    page_table_[*page_id] = frame_id;
    replacer_->Pin(frame_id);
    // latch_.unlock();
    return P;
  }
  // latch_.unlock();
  return nullptr;
  
}

Page *BufferPoolManagerInstance::FetchPgImp(page_id_t page_id) {
  // 1.     Search the page table for the requested page (P).
  // 1.1    If P exists, pin it and return it immediately.
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  // 2.     If R is dirty, write it back to the disk.
  // 3.     Delete R from the page table and insert P.
  // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.
  std::lock_guard<std::mutex> guard(latch_);

  frame_id_t frame_id = -1; // Initiate frame id.

  auto p_iter = page_table_.find(page_id);
  if (p_iter != page_table_.end()) { // If page was found in the page table.
    frame_id = p_iter->second;
    // replacer_->Pin(frame_id);
    // pages_[frame_id].pin_count_++;
    // return &pages_[frame_id];
    Page *page = &pages_[frame_id];
    page->pin_count_++;
    replacer_->Pin(frame_id);
    return page;
  }

    Page *R = nullptr; // Initiate R, replacement page.
    if (!free_list_.empty()) { // Find R in the free list.
      frame_id = free_list_.front();
      free_list_.pop_front();
      R = &pages_[frame_id];
  }
    else if (replacer_->Victim(&frame_id)) { // Find R in the replacer.
      R = &pages_[frame_id];
      if (R->IsDirty()) { // If R is dirty, write it back to the disk.
        disk_manager_->WritePage(R->GetPageId(), R->GetData());
      }
      page_table_.erase(R->GetPageId());
    }
    if (R != nullptr) { // If R has value.
      disk_manager_->ReadPage(page_id, R->GetData()); // Get page from disk to the R's memory.
      page_table_[page_id] = frame_id; // Sync the page table.
      replacer_->Pin(frame_id); // Pin it.

      R->page_id_ = page_id; // Update R's meta value.
      R->pin_count_ = 1;
      R->is_dirty_ = false;
    }
    return R;

}

bool BufferPoolManagerInstance::DeletePgImp(page_id_t page_id) {
  // 0.   Make sure you call DeallocatePage!
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  // 2.   If P exists, but has a non-zero pin-count, return false. Someone is using the page.
  // 3.   Otherwise, P can be deleted. Remove P from the page table, reset its metadata and return it to the free list.
  std::lock_guard<std::mutex> guard(latch_);
  DeallocatePage(page_id);
  auto iter = page_table_.find(page_id);
  if (iter == page_table_.end()) {
    return true;
  }

    frame_id_t frame_id = iter->second;
    Page *page = pages_ + frame_id;
    if (page->pin_count_ > 0) {
      return false;
    }

      if (page->IsDirty()) {
        disk_manager_->WritePage(page_id, page->GetData());
      }
      page_table_.erase(page->GetPageId()); // After write it back to disk, erase it from page table.
      replacer_->Pin(frame_id);
      page->pin_count_ = 0; // Update page's metadata.
      page->page_id_ = INVALID_PAGE_ID;
      page->is_dirty_ = false;
      page->ResetMemory();
      free_list_.push_back(frame_id);
      return true;
}

bool BufferPoolManagerInstance::UnpinPgImp(page_id_t page_id, bool is_dirty) { 
  std::lock_guard<std::mutex> guard(latch_);
  auto iter = page_table_.find(page_id);
  if (iter == page_table_.end()) {
    return false;
  }
    frame_id_t frame_id = iter->second;
    Page *page = pages_ + frame_id;
    if (page->GetPinCount() <= 0) {
      return false;
    }

    if (is_dirty) {
      page->is_dirty_ = is_dirty;
    }

    // I can not fully understand here.
    // Only the pin count be less than 0, replacer will unpin that frame id.
    page->pin_count_--;
    if (page->GetPinCount() <= 0) {
      replacer_->Unpin(frame_id);
    }
    return true;
  
  }

page_id_t BufferPoolManagerInstance::AllocatePage() {
  const page_id_t next_page_id = next_page_id_;
  next_page_id_ += num_instances_;
  ValidatePageId(next_page_id);
  return next_page_id;
}

void BufferPoolManagerInstance::ValidatePageId(const page_id_t page_id) const {
  assert(page_id % num_instances_ == instance_index_);  // allocated pages mod back to this BPI
}

}  // namespace bustub
