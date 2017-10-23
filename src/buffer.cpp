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
#include "exceptions/invalid_page_exception.h"
#include "exceptions/hash_already_present_exception.h"

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


    void BufMgr::allocBuf(FrameId &frame) {
        // While page not found, keep looking for it
        std::uint32_t countPinnLargerThan0 = 0;
        while (countPinnLargerThan0 < numBufs) {

            // Advance clock pointer
            advanceClock();

            //Check if the current frame is valid set
            //If no
            // then use the frame
            BufDesc *cur = &bufDescTable[clockHand];
            if (!cur->valid) {
                frame = clockHand;
                return;
            }

            //if valid, check if refbit is set
            if (cur->refbit) {
                //if so, clear refbit
                cur->refbit = false;
                // go back to advance clock
                continue;
            }
            if (cur->pinCnt >
                       0) { //If refbit not set, check if the page is pinned
                // if pinned, advance clock, increment count of pinned frames
                countPinnLargerThan0++;
                continue;
            }
            if (cur->dirty) { //If not, check if the dirty bit is set
                //If yes, flush page to disk and use the page
                cur->file->writePage(bufPool[cur->frameNo]);
            }
            // if it is not dirty,  use the frame
            hashTable->remove(cur->file, cur->pageNo);
            frame = clockHand;

            return;
        }
        //If all the buffer frames are pinned, throw bufferExceededException
        throw BufferExceededException();
    }


    void BufMgr::readPage(File *file, const PageId pageNo, Page *&page) {
        // Case 1: page is in the buffer pool
        FrameId frameId;
        try {
            hashTable->lookup(file, pageNo, frameId);
            page = &bufPool[frameId];
            // Set the refbit
            bufDescTable[frameId].refbit = true;
            // Increment the pin count
            bufDescTable[frameId].pinCnt++;
            return;
        } catch (HashNotFoundException &e) {
            // Case2: the page is not in the buffer
            try {
                Page curPage = file->readPage(pageNo);
                // Find the spot and replace the page inside the picked frame
                allocBuf(frameId);
                // Read the page from the file and insert it into the buf pool
                bufPool[frameId] = curPage;
                // Insert the page into the hash table
                hashTable->insert(file, pageNo, frameId);
                // Set the page in the desc table
                bufDescTable[frameId].Set(file, pageNo);
                // return the pointer to the page
                page = &bufPool[frameId];
                return;
            } catch (BufferExceededException &e) {
                std::cout << e.message() << std::endl;
                exit(-1);
            }
        }
    }

    void BufMgr::unPinPage(File *file, const PageId pageNo, const bool dirty) {
        FrameId  frameId;
        try {
            // Check if the picked page is in the buffer
            hashTable->lookup(file, pageNo, frameId);
            // Get the frame
            BufDesc *desc = &bufDescTable[frameId];
            // Check if the pinCnt is 0
            if (desc->pinCnt <= 0) {
                throw PageNotPinnedException(file->filename(),pageNo,frameId);
            }
            // Decrement the pinCnt;
            desc->pinCnt--;
            // If it is dirty, set the dirty bit
            if (dirty) {
                desc->dirty = true;
            }
        } catch (HashNotFoundException &e) {
            std::cout<<"Cannot find the page trying to unpinning"<<std::endl;
            std::cout<<e.message()<<std::endl;
            // TODO:Maybe need to exit here
        }
    }

    void BufMgr::flushFile(const File *file) {
        // Scan bufTable for pages belonging to the file
        for (int i = 0; i < numBufs; i++) {
            BufDesc *buf = &bufDescTable[i];
            // TODO: might not be the same file
            // If the page belong to the file is not valid, throw BadBUfferException
            if (!buf->valid) {
                throw BadBufferException(buf->frameNo,buf->dirty,buf->valid,buf->refbit);
            }
            if (buf->file == file) {
                // If the page is pinned throw PagePinnedException
                if (buf -> pinCnt > 0) {
                    throw PagePinnedException(file->filename(), buf->pageNo, buf->frameNo);
                }
                // If the page is dirty, write the page to the file
                // TODO: might not need to catch InvalidPageException

                if (buf->dirty) {
                    try {
                        buf->file->writePage(bufPool[buf->frameNo]);
                    } catch(InvalidPageException &e) {
                        std::cout << "Trying flush file" << e.message() << std::endl;
                        exit(-1);
                    }
                }

                // Remove the page form the hash table
                // TODO: may not need to catch the hashNotFound error
                try {
                    hashTable->remove(buf->file, buf->pageNo);
                } catch(HashNotFoundException &e) {
                    std::cout <<"Trying flush file" << e.message() << std::endl;
                    exit(-1);
                }
                // Clear the page frame
                buf->Clear();
            }
        }
    }

    void BufMgr::allocPage(File *file, PageId &pageNo, Page *&page) {
        // Invoke empty page
        Page curPage = file->allocatePage();

        // Get a buffer pool frame
        FrameId frameId;
        allocBuf(frameId);

        // Entry into hash table
        try {
            hashTable->insert(file, curPage.page_number(), frameId);
        } catch (HashAlreadyPresentException &e) {
            std::cout << "hash collision" << e.message() << std::endl;
            exit(-1);
        } catch (HashNotFoundException &e) {
            std::cout << "hash not found" << e.message() << std::endl;
            exit(-1);
        }

        // Call the set on the buf table
        bufDescTable[frameId].Set(file, curPage.page_number());

        // Isnert the page into the bufPool;
        bufPool[frameId] = curPage;

        // Return values
        page = &bufPool[frameId];
        pageNo = curPage.page_number();
    }

    void BufMgr::disposePage(File *file, const PageId PageNo) {
        // Try to find the page
        FrameId frameId;

        try{
            hashTable->lookup(file,PageNo, frameId);
            // If the page is found in the buffer pool, free the frame and deleter from hashTable
            bufDescTable[frameId].Clear();
            hashTable->remove(file, PageNo);

        } catch (HashNotFoundException &e) {
            // Print some message
            std::cout << "the page trying to dispose is not in the buffer" << e.message() << std::endl;
        }
        // Delete the page from the file
        file->deletePage(PageNo);
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
