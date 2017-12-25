/**
 * b_plus_tree_leaf_page.cpp
 */

#include <sstream>
#include <include/page/b_plus_tree_internal_page.h>

#include "common/exception.h"
#include "common/rid.h"
#include "page/b_plus_tree_leaf_page.h"

namespace cmudb {

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set page id/parent id, set
 * next page id and set max size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(page_id_t page_id, page_id_t parent_id) {
  SetPageType(IndexPageType::LEAF_PAGE);
  SetSize(0);
  SetPageId(page_id);
  SetParentPageId(parent_id);
  SetNextPageId(INVALID_PAGE_ID);
  SetMaxSize((PAGE_SIZE - sizeof(BPlusTreePage)) / sizeof(MappingType) - 1);//leave a always available slot for insertion
}

/**
 * Helper methods to set/get next page id
 */
INDEX_TEMPLATE_ARGUMENTS
page_id_t B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const {
  return next_page_id_;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) { next_page_id_ = next_page_id; }

/**
 * Helper method to find the first index i so that array[i].first >= key
 * NOTE: This method is only used when generating index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE_TYPE::KeyIndex(
    const KeyType &key, const KeyComparator &comparator) const {
  int len = GetSize();
  int b = 0;
  int e = len;
  while (b < e) {
    int mid = b + (e - b) / 2;
    if (comparator(array[mid].first, key)) {
      b = mid + 1;
    } else {
      e = mid;
    }
  }
  return b;
}

/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
KeyType B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const {
  // replace with your own code
  KeyType key;
  assert(index >= 0 && index < GetSize());
  key = array[index].first;
  return key;
}

/*
 * Helper method to find and return the key & value pair associated with input
 * "index"(a.k.a array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
const MappingType &B_PLUS_TREE_LEAF_PAGE_TYPE::GetItem(int index) {
  // replace with your own code
  assert(index >= 0 && index < GetSize());
  return array[index];
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert key & value pair into leaf page ordered by key
 * @return  page size after insertion
 */

//where is reconstruction code?
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE_TYPE::Insert(const KeyType &key,
                                       const ValueType &value,
                                       const KeyComparator &comparator) {
  assert(GetSize() < GetMaxSize());
  int i = GetSize() - 1;
  for (; i >= 0; i--) {
    if (comparator(key, array[i])) {
      array[i + 1].first = array[i].first;
      array[i + 1].second = array[i].second;
    } else {
      break;
    }
  }

  array[i + 1].first = key;
  array[i + 1].second = value;
  IncreaseSize(1);
  return GetSize();
}

/*****************************************************************************
 * SPLIT
 *****************************************************************************/
/*
 * Remove half of key & value pairs from this page to "recipient" page
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveHalfTo(
    BPlusTreeLeafPage *recipient,
    __attribute__((unused)) BufferPoolManager *buffer_pool_manager) {
  assert(recipient != nullptr);
  assert(GetSize() == GetMaxSize());
  //maintain the single link list
  recipient->next_page_id_ = next_page_id_;
  next_page_id_ = recipient->GetPageId();

  //copy
  int length = GetMaxSize();
  int start = (length + 1) / 2;
  for (int i = start, j = 0; i < length; i++, j++) {
    recipient->array[j].first = array[i].first;
    recipient->array[j].second = array[i].second;
  }

  //maintain size:
  SetSize(start);
  recipient->IncreaseSize(length - start);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyHalfFrom(MappingType *items, int size) {
  //I don't understand this for the moment.
  assert(false);
}

/*****************************************************************************
 * LOOKUP
 *****************************************************************************/
/*
 * For the given key, check to see whether it exists in the leaf page. If it
 * does, then store its corresponding value in input "value" and return true.
 * If the key does not exist, then return false
 */
INDEX_TEMPLATE_ARGUMENTS
bool B_PLUS_TREE_LEAF_PAGE_TYPE::Lookup(const KeyType &key, ValueType &value,
                                        const KeyComparator &comparator) const {
  auto index = KeyIndex(key, comparator);
  if (index >= 0 && index <= GetSize()) {
    array[index] = value;
    return true;
  }
  return false;
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * First look through leaf page to see whether delete key exist or not. If
 * exist, perform deletion, otherwise return immediately.
 * NOTE: store key&value pair continuously after deletion
 * @return   page size after deletion
 */
INDEX_TEMPLATE_ARGUMENTS
int B_PLUS_TREE_LEAF_PAGE_TYPE::RemoveAndDeleteRecord(
    const KeyType &key, const KeyComparator &comparator) {
  auto index = KeyIndex(key, comparator);
  if (index >= 0 && index <= GetSize()) {
    for (int i = index; i + 1 < GetSize(); i++) {
      array[i].first = array[i + 1].first;
      array[i].second = array[i + 1].second;
    }
    return true;
  }
  return false;
}

/*****************************************************************************
 * MERGE
 *****************************************************************************/
/*
 * Remove all of key & value pairs from this page to "recipient" page, then
 * update next page id
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveAllTo(BPlusTreeLeafPage *recipient,
                                           int, BufferPoolManager *) {
  assert(false);
  //remove and assign to recipient?
}
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyAllFrom(MappingType *items, int size) {
  for (int i = 0, j = GetSize(); i < size; i++) {
    array[j].first = items[i].first;
    array[j].second = items[i].second;
  }
  IncreaseSize(size);
}

/*****************************************************************************
 * REDISTRIBUTE
 *****************************************************************************/
/*
 * Remove the first key & value pair from this page to "recipient" page, then
 * update relevant key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveFirstToEndOf(
    BPlusTreeLeafPage *recipient,
    BufferPoolManager *buffer_pool_manager) {
  BPlusTreeInternalPage
      *parent =
      reinterpret_cast<BPlusTreeInternalPage *> (buffer_pool_manager->FetchPage(GetParentPageId())->GetData());
  MappingType item = GetItem(0);
  MappingType endOfRecipient = recipient->GetItem(recipient->GetSize() - 1);
  //insert in parent
  //need to remove first kv which points to recipient
  auto parentKVIndexToRecipient = parent->ValueIndex(recipient->GetPageId());
  parent->InsertNodeAfter(endOfRecipient.second, item.first, GetPageId());
  parent->Remove(parentKVIndexToRecipient);

  recipient->array[GetSize()].first = item.first;
  recipient->array[GetSize()].second = item.second;
  recipient->IncreaseSize(1);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyLastFrom(const MappingType &item) {
  array[GetSize()].first = item.first;
  array[GetSize()].second = item.second;
  IncreaseSize(1);
}
/*
 * Remove the last key & value pair from this page to "recipient" page, then
 * update relevant key & value pair in its parent page.
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveLastToFrontOf(
    BPlusTreeLeafPage *recipient, int parentIndex,
    BufferPoolManager *buffer_pool_manager) {
  BPlusTreeInternalPage
      *parent =
      reinterpret_cast<BPlusTreeInternalPage *> (buffer_pool_manager->FetchPage(GetParentPageId())->GetData());
  MappingType item = GetItem(GetSize() - 1);
  IncreaseSize(-1);
  int index = parent->ValueIndex(GetPageId());
  parent->InsertNodeAfter(GetPageId(), GetItem(GetSize() - 1).first, GetPageId());
  parent->Remove(index);

  recipient->CopyFirstFrom(item, parentIndex, buffer_pool_manager);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::CopyFirstFrom(
    const MappingType &item, int parentIndex,
    BufferPoolManager *buffer_pool_manager) {
  for (int i = GetSize(); i > 0; i--) {
    array[i].first = array[i - 1].first;
    array[i].second = array[i - 1].second;
  }
  array[0].first = item.first;
  array[0].second = item.second;
}

/*****************************************************************************
 * DEBUG
 *****************************************************************************/
INDEX_TEMPLATE_ARGUMENTS
std::string B_PLUS_TREE_LEAF_PAGE_TYPE::ToString(bool verbose) const {
  if (GetSize() == 0) {
    return "";
  }
  std::ostringstream stream;
  if (verbose) {
    stream << "[pageId: " << GetPageId() << " parentId: " << GetParentPageId()
           << "]<" << GetSize() << "> ";
  }
  int entry = 0;
  int end = GetSize();
  bool first = true;

  while (entry < end) {
    if (first) {
      first = false;
    } else {
      stream << " ";
    }
    stream << std::dec << array[entry].first;
    if (verbose) {
      stream << "(" << array[entry].second << ")";
    }
    ++entry;
  }
  return stream.str();
}

template
class BPlusTreeLeafPage<GenericKey<4>, RID,
                        GenericComparator<4>>;
template
class BPlusTreeLeafPage<GenericKey<8>, RID,
                        GenericComparator<8>>;
template
class BPlusTreeLeafPage<GenericKey<16>, RID,
                        GenericComparator<16>>;
template
class BPlusTreeLeafPage<GenericKey<32>, RID,
                        GenericComparator<32>>;
template
class BPlusTreeLeafPage<GenericKey<64>, RID,
                        GenericComparator<64>>;
} // namespace cmudb
