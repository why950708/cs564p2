/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"

namespace badgerdb {

    BufMgr::BufMgr(std::uint32_t bufs)
            : numBufs(bufs) {
        bufDescTable = new BufDesc[bufs];

        for (FrameId i = 0; i < bufs; i++) {
            bufDescTable[i].frameNo = i;
            bufDescTable[i].valid = false;
        }

        bufPool = new Page[bufs];

        int htsize = ((((int) (bufs * 1.2)) * 2) / 2) + 1;
        hashTable = new BufHashTbl(htsize);  // allocate the buffer hash table

        clockHand = bufs - 1;
    }


    BufMgr::~BufMgr() {
    }

    void BufMgr::advanceClock() {
        clockHand = (clockHand + 1) % numBufs;
    }

    /*
     * * Allocate a free frame.
                      *
                      * @param frame   	Frame reference, frame ID of allocated frame returned via this variable
    * @throws BufferExceededException If no such buffer is found which can be allocated
    */

    void BufMgr::allocBuf(FrameId &frame) {
        // While page not found, keep looking for it
        int countPinnLargerThan0 = 0;
        while (1) {
            // Advance clock pointer
            advanceClock();

            //Check if the current frame is valid set
            //If no, call "set()" on the frame
            // then use the frame
            BufDesc cur = bufDescTable[clockHand];
            if (!cur.valid) {
                frame = clockHand;
                return;
            }

            //if valid, check if refbit is set
            if (cur.refbit) {
                //if so, clear refbit
                cur.refbit = false;
                // go back to advance clock
                continue;
            } else if (cur.pinCnt > 0) { //If refbit not set, check if the page is pinned
                // if pinned, advance clock
                continue;
            } else if (cur.dirty) { //If not, check if the dirty bit is set
                // If yes, flush page to disk
                //If yes, flush page to disk and use the page
                // TODO:// find the file
                flushFile(NULL);
                frame = clockHand;
                return;
            } else { // if it is not dirty, set on the frame and use the frame
                frame = clockHand;
                return;
            }


        }
    }


    void BufMgr::readPage(File *file, const PageId pageNo, Page *&page) {
    }


    void BufMgr::unPinPage(File *file, const PageId pageNo, const bool dirty) {
    }

    void BufMgr::flushFile(const File *file) {
    }

    void BufMgr::allocPage(File *file, PageId &pageNo, Page *&page) {
    }

    void BufMgr::disposePage(File *file, const PageId PageNo) {

    }

    void BufMgr::printSelf(void) {
        BufDesc *tmpbuf;
        int validFrames = 0;

        for (std::uint32_t i = 0; i < numBufs; i++) {
            tmpbuf = &(bufDescTable[i]);
            std::cout << "FrameNo:" << i << " ";
            tmpbuf->Print();

            if (tmpbuf->valid == true)
                validFrames++;
        }

        std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
    }

}
