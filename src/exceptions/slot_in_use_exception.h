/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#pragma once

#include <string>

#include "badgerdb_exception.h"
//#include "types.h"
#include "../types.h"

namespace badgerdb {

/**
 * @brief An exception that is thrown when a record is attempted to be inserted
 *        into a slot that is already in use.
 */
class SlotInUseException : public BadgerDbException {
 public:
  /**
   * Constructs a slot in use exception file for the given page and slot.
   *
   * @param page_num   Number of page containing slot.
   * @param slot_num   Number of slot which is in use.
   */
  SlotInUseException(const PageId page_num, const SlotId slot_num);

  /**
   * Returns the page number of the page containing the slot which caused this
   * exception.
   */
  virtual PageId page_number() const { return page_number_; }

  /**
   * Returns the slot number of the slot which caused this exception.
   */
  virtual SlotId slot_number() const { return slot_number_; }

 protected:
  /**
   * Page number of the page containing the slot which caused this exception.
   */
  const PageId page_number_;

  /**
   * Slot number of the slot which caused this exception.
   */
  const SlotId slot_number_;
};

}
