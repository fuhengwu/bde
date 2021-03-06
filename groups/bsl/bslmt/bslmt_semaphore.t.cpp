// bslmt_semaphore.t.cpp                                              -*-C++-*-

// ----------------------------------------------------------------------------
//                                   NOTICE
//
// This component is not up to date with current BDE coding standards, and
// should not be used as an example for new development.
// ----------------------------------------------------------------------------

#include <bslmt_semaphore.h>

#include <bslmt_threadattributes.h>     // for testing only
#include <bslmt_threadutil.h>           // for testing only

#include <bslim_testutil.h>

#include <bsls_timeinterval.h>
#include <bsls_atomic.h>
#include <bsl_iostream.h>
#include <bsl_cstdlib.h>
#include <bsl_deque.h>

using namespace BloombergLP;
using namespace bsl;  // automatically added by script

//=============================================================================
//                             TEST PLAN
//-----------------------------------------------------------------------------
//                              OVERVIEW
//
//
//-----------------------------------------------------------------------------
// [1] Breathing test

// ============================================================================
//                     STANDARD BDE ASSERT TEST FUNCTION
// ----------------------------------------------------------------------------

namespace {

int testStatus = 0;

void aSsErT(bool condition, const char *message, int line)
{
    if (condition) {
        cout << "Error " __FILE__ "(" << line << "): " << message
             << "    (failed)" << endl;

        if (0 <= testStatus && testStatus <= 100) {
            ++testStatus;
        }
    }
}

}  // close unnamed namespace

// ============================================================================
//               STANDARD BDE TEST DRIVER MACRO ABBREVIATIONS
// ----------------------------------------------------------------------------

#define ASSERT       BSLIM_TESTUTIL_ASSERT
#define ASSERTV      BSLIM_TESTUTIL_ASSERTV

#define LOOP_ASSERT  BSLIM_TESTUTIL_LOOP_ASSERT
#define LOOP0_ASSERT BSLIM_TESTUTIL_LOOP0_ASSERT
#define LOOP1_ASSERT BSLIM_TESTUTIL_LOOP1_ASSERT
#define LOOP2_ASSERT BSLIM_TESTUTIL_LOOP2_ASSERT
#define LOOP3_ASSERT BSLIM_TESTUTIL_LOOP3_ASSERT
#define LOOP4_ASSERT BSLIM_TESTUTIL_LOOP4_ASSERT
#define LOOP5_ASSERT BSLIM_TESTUTIL_LOOP5_ASSERT
#define LOOP6_ASSERT BSLIM_TESTUTIL_LOOP6_ASSERT

#define Q            BSLIM_TESTUTIL_Q   // Quote identifier literally.
#define P            BSLIM_TESTUTIL_P   // Print identifier and value.
#define P_           BSLIM_TESTUTIL_P_  // P(X) without '\n'.
#define T_           BSLIM_TESTUTIL_T_  // Print a tab (w/o newline).
#define L_           BSLIM_TESTUTIL_L_  // current Line number

///Usage
///-----
// This example illustrates a very simple queue where potential clients can
// push integers to a queue, and later retrieve the integer values from the
// queue in FIFO order.  It illustrates two potential uses of semaphores: to
// enforce exclusive access, and to allow resource sharing.
//..
    class IntQueue {
        // FIFO queue of integer values.

        // DATA
        bsl::deque<int> d_queue;        // underlying queue
        bslmt::Semaphore d_mutexSem;     // mutual-access semaphore
        bslmt::Semaphore d_resourceSem;  // resource-availability semaphore

        // NOT IMPLEMENTED
        IntQueue(const IntQueue&);
        IntQueue& operator=(const IntQueue&);

      public:
        // CREATORS
        explicit IntQueue(bslma::Allocator *basicAllocator = 0);
            // Create an 'IntQueue' object.  Optionally specified a
            // 'basicAllocator' used to supply memory.  If 'basicAllocator' is
            // 0, the currently installed default allocator is used.

        ~IntQueue();
            // Destroy this 'IntQueue' object.

        // MANIPULATORS
        int getInt();
            // Retrieve an integer from this 'IntQueue' object.  Integer values
            // are obtained from the queue in FIFO order.

        void pushInt(int value);
            // Push the specified 'value' to this 'IntQueue' object.
    };
//..
// Note that the 'IntQueue' constructor increments the count of the semaphore
// to 1 so that values can be pushed into the queue immediately following
// construction:
//..
    // CREATORS
    IntQueue::IntQueue(bslma::Allocator *basicAllocator)
    : d_queue(basicAllocator)
    {
        d_mutexSem.post();
    }

    IntQueue::~IntQueue()
    {
        d_mutexSem.wait();  // Wait for potential modifier.
    }

    // MANIPULATORS
    int IntQueue::getInt()
    {
        // Waiting for resources.
        d_resourceSem.wait();

        // 'd_mutexSem' is used for exclusive access.
        d_mutexSem.wait();        // lock
        const int ret = d_queue.back();
        d_queue.pop_back();
        d_mutexSem.post();        // unlock

        return ret;
    }

    void IntQueue::pushInt(int value)
    {
        d_mutexSem.wait();
        d_queue.push_front(value);
        d_mutexSem.post();

        d_resourceSem.post();  // Signal we have resources available.
    }
//..

// ============================================================================
//                   GLOBAL TYPEDEFS/CONSTANTS FOR TESTING
// ----------------------------------------------------------------------------

typedef bslmt::Semaphore Obj;

class WaitAndSetJob {

    bslmt::Semaphore *d_obj;
    bsls::AtomicInt *d_val;

  public:
    WaitAndSetJob(bslmt::Semaphore *obj,
                  bsls::AtomicInt *val)
    : d_obj(obj)
    , d_val(val)
    {
    }

    void operator()() const {
        d_obj->wait();
        (*d_val) = 1;
    }
};

// ============================================================================
//                               MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int                 test = argc > 1 ? atoi(argv[1]) : 0;
    bool             verbose = argc > 2;
    bool         veryVerbose = argc > 3;
    bool     veryVeryVerbose = argc > 4;
    bool veryVeryVeryVerbose = argc > 5;

    (void)        veryVerbose;  // unused variable warning
    (void)    veryVeryVerbose;  // unused variable warning
    (void)veryVeryVeryVerbose;  // unused variable warning

    cout << "TEST " << __FILE__ << " CASE " << test << endl;

    switch (test) {  case 0:
      case 3: {
        IntQueue testQueue;

        testQueue.pushInt(1);
        ASSERT(1 == testQueue.getInt());
        testQueue.pushInt(2);
        ASSERT(2 == testQueue.getInt());
      } break;
      case 2: {
        // --------------------------------------------------------------------
        // BLOCKING TEST
        //
        // Test that a thread can block on the semaphore.
        // --------------------------------------------------------------------

        if (verbose) {
            cout << endl
                 << "Blocking Test" << endl
                 << "=============" << endl;
        }
        {
            Obj X;
            bslmt::ThreadUtil::Handle h;
            bsls::AtomicInt flag;
            ASSERT(0 == bslmt::ThreadUtil::create(&h,
                                                  WaitAndSetJob(&X, &flag)));
            bslmt::ThreadUtil::sleep(bsls::TimeInterval(1));
            ASSERT(0 == flag);
            X.post();
            bslmt::ThreadUtil::join(h);
            ASSERT(1 == flag);
        }
      } break;
      case 1: {
        // --------------------------------------------------------------------
        // BREATHING TEST:
        //
        // Exercises basic functionality.
        // --------------------------------------------------------------------

        if (verbose) {
            cout << endl
                 << "Breathing Test" << endl
                 << "==============" << endl;
        }
        {
            Obj X;
            X.post();
            X.post(2);
            X.wait();
            X.wait();
            ASSERT(0 == X.tryWait());
            ASSERT(0 != X.tryWait());
        }
      } break;

      default: {
          testStatus = -1; break;
      }

    }
    return testStatus;
}

// ----------------------------------------------------------------------------
// Copyright 2015 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
