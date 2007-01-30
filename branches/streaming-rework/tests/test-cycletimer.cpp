/***************************************************************************
  Copyright (C) 2005 by Pieter Palmers   *
                                                                        *
  This program is free software; you can redistribute it and/or modify  *
  it under the terms of the GNU General Public License as published by  *
  the Free Software Foundation; either version 2 of the License, or     *
  (at your option) any later version.                                   *
                                                                        *
  This program is distributed in the hope that it will be useful,       *
  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
  GNU General Public License for more details.                          *
                                                                        *
  You should have received a copy of the GNU General Public License     *
  along with this program; if not, write to the                         *
  Free Software Foundation, Inc.,                                       *
  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>

#include <signal.h>
#include "src/debugmodule/debugmodule.h"

#include <netinet/in.h>

#include "src/libstreaming/cycletimer.h"

#include "src/libstreaming/IsoHandler.h"
#include "src/libstreaming/IsoStream.h"
#include "src/libstreaming/IsoHandlerManager.h"
#include "src/libutil/PosixThread.h"

#define TEST_PORT_0
// #define TEST_PORT_1
// #define TEST_PORT_2

using namespace FreebobStreaming;
using namespace FreebobUtil;

DECLARE_GLOBAL_DEBUG_MODULE;

int run;

struct CYCLE_TIMER_REGISTER {
	uint16_t seconds;
	uint16_t cycles;
	uint16_t offset;
};

uint64_t ctr_to_quadlet(struct CYCLE_TIMER_REGISTER x) {
    uint64_t retval=0;
    
    x.seconds &= 0x7F;
    
    x.cycles &= 0x1FFF;
    x.cycles %= 8000;
    
    x.offset &= 0xFFF;
    x.offset %= 3072;
    
    retval = (x.seconds << 25) + (x.cycles << 12) + (x.offset);
    return retval & 0xFFFFFFFF;
}

static void sighandler (int sig)
{
	run = 0;
}

int do_cycletimer_test() {
    
    struct CYCLE_TIMER_REGISTER cycle_timer;
    uint32_t *cycle_timer_as_uint=(uint32_t *)&cycle_timer;
    
    uint32_t i=0;
    uint32_t targetval=0;
    
    int failures=0;
    
    
    // test 1
    // 
    
    *cycle_timer_as_uint=0;
    for (i=0;i<3072;i++) {
        cycle_timer.offset=i;
        targetval=CYCLE_TIMER_GET_OFFSET(ctr_to_quadlet(cycle_timer));
        
        if(targetval != i) {
            debugOutput(DEBUG_LEVEL_NORMAL, "  test1 failed on i=%d (%08X), returns %d (%08X)\n",i,i,targetval,targetval);
            failures++;
        }
    }
    
    for (i=0;i<8000;i++) {
        cycle_timer.cycles=i;
        targetval=CYCLE_TIMER_GET_CYCLES(ctr_to_quadlet(cycle_timer));
        
        if(targetval != i) {
            debugOutput(DEBUG_LEVEL_NORMAL, "  test2 failed on i=%d (%08X), returns %d (%08X)\n",i,i,targetval,targetval);
            failures++;
        }
    }
    
    for (i=0;i<128;i++) {
        cycle_timer.seconds=i;
        targetval=CYCLE_TIMER_GET_SECS(ctr_to_quadlet(cycle_timer));
        
        if(targetval != i) {
            debugOutput(DEBUG_LEVEL_NORMAL, "  test3 failed on i=%d (%08X), returns %d (%08X)\n",i,i,targetval,targetval);
            failures++;
        }
    }
    
    
    // a value in ticks
    // should be: 10sec, 1380cy, 640ticks
    targetval=250000000L;
    cycle_timer.seconds = TICKS_TO_SECS(targetval);
    cycle_timer.cycles  = TICKS_TO_CYCLES(targetval);
    cycle_timer.offset  = TICKS_TO_OFFSET(targetval);
    
    if((cycle_timer.seconds != 10) |
        (cycle_timer.cycles != 1380) |
        (cycle_timer.offset != 640))
        {
        debugOutput(DEBUG_LEVEL_NORMAL, "  test4 failed: (%u,10)sec (%u,1380)cy (%u,640)ticks\n",
            cycle_timer.seconds,cycle_timer.cycles,cycle_timer.offset);
        failures++;
    } else {
         debugOutput(DEBUG_LEVEL_NORMAL, "  test4 ok\n");
    }
    
    i=TICKS_TO_CYCLE_TIMER(targetval);
    if (i != 0x14564280) {
         debugOutput(DEBUG_LEVEL_NORMAL, "  test5 failed: (0x%08X,0x14564280)\n",
            i);
        failures++;   
    } else {
         debugOutput(DEBUG_LEVEL_NORMAL, "  test5 ok\n");
    }
    
    targetval=CYCLE_TIMER_TO_TICKS(i);
    if (targetval!=250000000L) {
         debugOutput(DEBUG_LEVEL_NORMAL, "  test6 failed: (%u,250000000)\n",
            targetval);
        failures++;   
    } else {
         debugOutput(DEBUG_LEVEL_NORMAL, "  test6 ok\n");
    }
    
    if (failures) {
        debugOutput(DEBUG_LEVEL_NORMAL, " %d failures\n",failures);
        return -1;
    } else {
        debugOutput(DEBUG_LEVEL_NORMAL, " no failures\n");
    }
    return 0;
}

int main(int argc, char *argv[])
{

	run=1;
	
	IsoHandlerManager *m_isoManager=NULL;
    PosixThread * m_isoManagerThread=NULL;
    
#ifdef TEST_PORT_0	
    IsoStream *s=NULL;
#endif
#ifdef TEST_PORT_1	
    IsoStream *s2=NULL;
#endif
#ifdef TEST_PORT_2
    IsoStream *s3=NULL;
#endif

	signal (SIGINT, sighandler);
	signal (SIGPIPE, sighandler);

	debugOutput(DEBUG_LEVEL_NORMAL, "Freebob Cycle timer test application\n");
	
	debugOutput(DEBUG_LEVEL_NORMAL, "Testing cycle timer helper functions & macro's... \n");
	if(do_cycletimer_test()) {
	   debugOutput(DEBUG_LEVEL_NORMAL, " !!! FAILED !!!\n");
	   exit(1);
	} else {
	   debugOutput(DEBUG_LEVEL_NORMAL, " !!! PASSED !!!\n");
	}
	
// 	exit(1);
	
	m_isoManager=new IsoHandlerManager();
	
	if(!m_isoManager) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not create IsoHandlerManager\n");
		goto finish;
	}
	
	m_isoManager->setVerboseLevel(DEBUG_LEVEL_VERBOSE);
		
    // the thread to iterate the manager
 	m_isoManagerThread=new PosixThread(
 	      m_isoManager, 
 	      true, 80,
 	      PTHREAD_CANCEL_DEFERRED);
 	      
	if(!m_isoManagerThread) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not create iso manager thread\n");
		goto finish;
	}

#ifdef TEST_PORT_0	
	// add a stream to the manager so that it has something to do
	s=new IsoStream(IsoStream::EST_Receive, 0);
	
	if (!s) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not create IsoStream\n");
		goto finish;
	}	
	
	s->setVerboseLevel(DEBUG_LEVEL_VERBOSE);
	
	if (!s->init()) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not init IsoStream\n");
		goto finish;
	}
	
	s->setChannel(0);
	
	if(!m_isoManager->registerStream(s)) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not register IsoStream\n");
		goto finish;
	}
#endif

#ifdef TEST_PORT_1	
	// add a stream to the manager so that it has something to do
	s2=new IsoStream(IsoStream::EST_Receive, 1);
	
	if (!s2) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not create IsoStream\n");
		goto finish;
	}	
	
	s2->setVerboseLevel(DEBUG_LEVEL_VERBOSE);
	
	if (!s2->init()) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not init IsoStream\n");
		goto finish;
	}
	
	s2->setChannel(0);
	
	if(!m_isoManager->registerStream(s2)) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not register IsoStream\n");
		goto finish;
	}
#endif

#ifdef TEST_PORT_2		
	// add a stream to the manager so that it has something to do
	s3=new IsoStream(IsoStream::EST_Receive,2);
	
	if (!s3) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not create IsoStream\n");
		goto finish;
	}	
	
	s3->setVerboseLevel(DEBUG_LEVEL_VERBOSE);
	
	if (!s3->init()) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not init IsoStream\n");
		goto finish;
	}
	
	s3->setChannel(0);
	
	if(!m_isoManager->registerStream(s3)) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not register IsoStream\n");
		goto finish;
	}
#endif

	debugOutput(DEBUG_LEVEL_NORMAL,   "Preparing IsoHandlerManager...\n");
	if (!m_isoManager->prepare()) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not prepare isoManager\n");
		goto finish;
	}

	debugOutput(DEBUG_LEVEL_NORMAL,   "Starting ISO manager sync update thread...\n");
	// start the runner thread
	m_isoManagerThread->Start();
	
	debugOutput(DEBUG_LEVEL_NORMAL,   "Starting IsoHandler...\n");
	if (!m_isoManager->startHandlers(0)) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not start handlers...\n");
		goto finish;
	}
	
	while(run) {
        sleep(1);
        m_isoManager->dumpInfo();
	}

	debugOutput(DEBUG_LEVEL_NORMAL,   "Stopping handlers...\n");
	if(!m_isoManager->stopHandlers()) {
	   debugOutput(DEBUG_LEVEL_NORMAL, "Could not stop ISO handlers\n");
	   goto finish;
	}
	
	// stop the sync thread
	debugOutput(DEBUG_LEVEL_NORMAL,   "Stopping ISO manager sync update thread...\n");
	m_isoManagerThread->Stop();
	
#ifdef TEST_PORT_0	
	if(!m_isoManager->unregisterStream(s)) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not unregister IsoStream\n");
		goto finish;
	}
	delete s;
#endif

#ifdef TEST_PORT_1	
	if(!m_isoManager->unregisterStream(s1)) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not unregister IsoStream\n");
		goto finish;
	}
	delete s1;
#endif

#ifdef TEST_PORT_2	
	if(!m_isoManager->unregisterStream(s2)) {
		debugOutput(DEBUG_LEVEL_NORMAL, "Could not unregister IsoStream\n");
		goto finish;
	}
	delete s2;
#endif
	
	delete m_isoManagerThread;
    delete m_isoManager;

finish:
	debugOutput(DEBUG_LEVEL_NORMAL, "Bye...\n");

  return EXIT_SUCCESS;
}
