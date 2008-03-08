/*
 * Copyright (C) 2005-2008 by Pieter Palmers
 *
 * This file is part of FFADO
 * FFADO = Free Firewire (pro-)audio drivers for linux
 *
 * FFADO is based upon FreeBoB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include "CycleTimerHelper.h"
#include "ieee1394service.h"
#include "libutil/PosixThread.h"
#include "libutil/Atomic.h"

#define DLL_PI        (3.141592653589793238)
#define DLL_SQRT2     (1.414213562373095049)

// the high-bandwidth coefficients are used
// to speed up inital tracking
#define DLL_BANDWIDTH_HIGH (0.1)
#define DLL_OMEGA_HIGH     (2.0*DLL_PI*DLL_BANDWIDTH_HIGH)
#define DLL_COEFF_B_HIGH   (DLL_SQRT2 * DLL_OMEGA_HIGH)
#define DLL_COEFF_C_HIGH   (DLL_OMEGA_HIGH * DLL_OMEGA_HIGH)

// the low-bandwidth coefficients are used once we have a
// good estimate of the internal parameters
#define DLL_BANDWIDTH (0.01)
#define DLL_OMEGA     (2.0*DLL_PI*DLL_BANDWIDTH)
#define DLL_COEFF_B   (DLL_SQRT2 * DLL_OMEGA)
#define DLL_COEFF_C   (DLL_OMEGA * DLL_OMEGA)

// is 5 sec
#define UPDATES_WITH_HIGH_BANDWIDTH \
         (5000000 / IEEE1394SERVICE_CYCLETIMER_DLL_UPDATE_INTERVAL_USEC)

IMPL_DEBUG_MODULE( CycleTimerHelper, CycleTimerHelper, DEBUG_LEVEL_NORMAL );

CycleTimerHelper::CycleTimerHelper(Ieee1394Service &parent, unsigned int update_period_us)
    : m_Parent ( parent )
    , m_ticks_per_update ( ((uint64_t)TICKS_PER_SECOND) * ((uint64_t)update_period_us) / 1000000ULL )
    , m_usecs_per_update ( update_period_us )
    , m_avg_wakeup_delay ( 0.0 )
    , m_dll_e2 ( 0.0 )
    , m_current_time_usecs ( 0 )
    , m_next_time_usecs ( 0 )
    , m_current_time_ticks ( 0 )
    , m_next_time_ticks ( 0 )
    , m_first_run ( true )
    , m_sleep_until ( 0 )
    , m_cycle_timer_prev ( 0 )
    , m_cycle_timer_ticks_prev ( 0 )
    , m_high_bw_updates ( UPDATES_WITH_HIGH_BANDWIDTH )
    , m_current_shadow_idx ( 0 )
    , m_Thread ( NULL )
    , m_realtime ( false )
    , m_priority ( 0 )
{
    debugOutput( DEBUG_LEVEL_VERBOSE, "Create %p...\n", this);
}

CycleTimerHelper::CycleTimerHelper(Ieee1394Service &parent, unsigned int update_period_us, bool rt, int prio)
    : m_Parent ( parent )
    , m_ticks_per_update ( ((uint64_t)TICKS_PER_SECOND) * ((uint64_t)update_period_us) / 1000000ULL )
    , m_usecs_per_update ( update_period_us )
    , m_avg_wakeup_delay ( 0.0 )
    , m_dll_e2 ( 0.0 )
    , m_current_time_usecs ( 0 )
    , m_next_time_usecs ( 0 )
    , m_current_time_ticks ( 0 )
    , m_next_time_ticks ( 0 )
    , m_first_run ( true )
    , m_sleep_until ( 0 )
    , m_cycle_timer_prev ( 0 )
    , m_cycle_timer_ticks_prev ( 0 )
    , m_high_bw_updates ( UPDATES_WITH_HIGH_BANDWIDTH )
    , m_current_shadow_idx ( 0 )
    , m_Thread ( NULL )
    , m_realtime ( rt )
    , m_priority ( prio )
{
    debugOutput( DEBUG_LEVEL_VERBOSE, "Create %p...\n", this);
}

CycleTimerHelper::~CycleTimerHelper()
{
    if (m_Thread) {
        m_Thread->Stop();
        delete m_Thread;
    }
}

bool
CycleTimerHelper::Start()
{
    debugOutput( DEBUG_LEVEL_VERBOSE, "Start %p...\n", this);

    // initialize the 'prev ctr' values
    uint64_t local_time;
    int maxtries2 = 10;
    do {
        if(!m_Parent.readCycleTimerReg(&m_cycle_timer_prev, &local_time)) {
            debugError("Could not read cycle timer register\n");
            return false;
        }
        if (m_cycle_timer_prev == 0) {
            debugOutput(DEBUG_LEVEL_VERBOSE,
                        "Bogus CTR: %08X on try %02d\n",
                        m_cycle_timer_prev, maxtries2);
        }
    } while (m_cycle_timer_prev == 0 && maxtries2--);
    m_cycle_timer_ticks_prev = CYCLE_TIMER_TO_TICKS(m_cycle_timer_prev);

#if IEEE1394SERVICE_USE_CYCLETIMER_DLL
    m_high_bw_updates = UPDATES_WITH_HIGH_BANDWIDTH;
    m_Thread = new Util::PosixThread(this, m_realtime, m_priority, 
                                     PTHREAD_CANCEL_DEFERRED);
    if(!m_Thread) {
        debugFatal("No thread\n");
        return false;
    }
    if (m_Thread->Start() != 0) {
        debugFatal("Could not start update thread\n");
        return false;
    }
#endif
    return true;
}

bool
CycleTimerHelper::Init()
{
    debugOutput( DEBUG_LEVEL_VERBOSE, "Initialize %p...\n", this);
    return true;
}

bool
CycleTimerHelper::setThreadParameters(bool rt, int priority) {
    debugOutput( DEBUG_LEVEL_VERBOSE, "(%p) switch to: (rt=%d, prio=%d)...\n", this, rt, priority);
    if (priority > THREAD_MAX_RTPRIO) priority = THREAD_MAX_RTPRIO; // cap the priority
    m_realtime = rt;
    m_priority = priority;

#if IEEE1394SERVICE_USE_CYCLETIMER_DLL
    if (m_Thread) {
        if (m_realtime) {
            m_Thread->AcquireRealTime(m_priority);
        } else {
            m_Thread->DropRealTime();
        }
    }
#endif

    return true;
}

#if IEEE1394SERVICE_USE_CYCLETIMER_DLL
float
CycleTimerHelper::getRate()
{
    float rate = (float)(diffTicks((uint64_t)m_next_time_ticks, (uint64_t)m_current_time_ticks));
    rate /= (float)(m_next_time_usecs - m_current_time_usecs);
    return rate;
}

float
CycleTimerHelper::getNominalRate()
{
    float rate = ((double)TICKS_PER_SECOND) / 1000000.0;
    return rate;
}

bool
CycleTimerHelper::Execute()
{
    debugOutputExtreme( DEBUG_LEVEL_VERY_VERBOSE, "Execute %p...\n", this);
    if (!m_first_run) {
        // wait for the next update period
        ffado_microsecs_t now = m_TimeSource.getCurrentTimeAsUsecs();
        int sleep_time = m_sleep_until - now;
        debugOutputExtreme( DEBUG_LEVEL_VERY_VERBOSE, "(%p) Sleep until %lld/%f (now: %lld, diff=%d) ...\n",
                    this, m_sleep_until, m_next_time_usecs, now, sleep_time);
        m_TimeSource.SleepUsecAbsolute(m_sleep_until);
        debugOutputExtreme( DEBUG_LEVEL_VERY_VERBOSE, " (%p) back...\n", this);
    }

    uint32_t cycle_timer;
    uint64_t local_time;
    int64_t usecs_late;
    int ntries=2;
    uint64_t cycle_timer_ticks;
    double diff_ticks;

    // if the difference between the predicted value and the
    // actual value seems to be too large, retry reading the cycle timer
    // some host controllers return bogus values on some reads
    // (looks like a non-atomic update of the register)
    do {
        if(!readCycleTimerWithRetry(&cycle_timer, &local_time, 10)) {
            debugError("Could not read cycle timer register\n");
            return false;
        }
        usecs_late = local_time - m_sleep_until;
        cycle_timer_ticks = CYCLE_TIMER_TO_TICKS(cycle_timer);
        diff_ticks = diffTicks(cycle_timer_ticks, (int64_t)m_next_time_ticks);

        // check for unrealistic CTR reads (NEC controller does that sometimes)
        if(diff_ticks < -((double)TICKS_PER_HALFCYCLE)) {
            debugOutput(DEBUG_LEVEL_VERBOSE, 
                        "have to retry CTR read, diff unrealistic: diff: %f, max: %f\n", 
                        diff_ticks, -((double)TICKS_PER_HALFCYCLE));
        }

    } while(diff_ticks < -((double)TICKS_PER_HALFCYCLE) && --ntries && !m_first_run);

    debugOutputExtreme( DEBUG_LEVEL_VERY_VERBOSE, " read : CTR: %11lu, local: %17llu\n",
                        cycle_timer, local_time);
        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE,
                           "  ctr   : 0x%08X %11llu (%03us %04ucy %04uticks)\n",
                           (uint32_t)cycle_timer, (uint64_t)cycle_timer_ticks,
                           (unsigned int)TICKS_TO_SECS( (uint64_t)cycle_timer_ticks ),
                           (unsigned int)TICKS_TO_CYCLES( (uint64_t)cycle_timer_ticks ),
                           (unsigned int)TICKS_TO_OFFSET( (uint64_t)cycle_timer_ticks ) );

    if (m_first_run) {
        m_sleep_until = local_time + m_usecs_per_update;
        m_dll_e2 = m_ticks_per_update;
        m_current_time_usecs = local_time;
        m_next_time_usecs = m_current_time_usecs + m_usecs_per_update;
        m_current_time_ticks = CYCLE_TIMER_TO_TICKS( cycle_timer );
        m_next_time_ticks = addTicks( (uint64_t)m_current_time_ticks, (uint64_t)m_dll_e2);
        debugOutput(DEBUG_LEVEL_VERBOSE, " First run\n");
        debugOutput(DEBUG_LEVEL_VERBOSE,
                    "  usecs/update: %lu, ticks/update: %lu, m_dll_e2: %f\n",
                    m_usecs_per_update, m_ticks_per_update, m_dll_e2);
        debugOutput(DEBUG_LEVEL_VERBOSE,
                    "  usecs current: %f, next: %f\n",
                    m_current_time_usecs, m_next_time_usecs);
        debugOutput(DEBUG_LEVEL_VERBOSE,
                    "  ticks current: %f, next: %f\n",
                    m_current_time_ticks, m_next_time_ticks);
        m_first_run = false;
    } else {
        // calculate next sleep time
        m_sleep_until += m_usecs_per_update;

        // correct for the latency between the wakeup and the actual CTR
        // read. The only time we can trust is the time returned by the
        // CTR read kernel call, since that (should be) atomically read
        // together with the ctr register itself.

        // if we are usecs_late usecs late 
        // the cycle timer has ticked approx ticks_late ticks too much
        // if we are woken up early (which shouldn't happen according to POSIX)
        // the cycle timer has ticked approx -ticks_late too little
        int64_t ticks_late = (usecs_late * TICKS_PER_SECOND) / 1000000LL;
        // the corrected difference between predicted and actual ctr
        // i.e. DLL error signal
        double diff_ticks_corr;
        if (ticks_late > 0) {
            diff_ticks_corr = diff_ticks - ticks_late;
            debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE,
                               "diff_ticks_corr=%f, diff_ticks = %f, ticks_late = %lld\n",
                               diff_ticks_corr, diff_ticks, ticks_late);
        } else {
            debugError("Early wakeup, should not happen!\n");
            // recover
            diff_ticks_corr = diff_ticks + ticks_late;
        }

        #ifdef DEBUG
        if(usecs_late > 200) {
            debugWarning("Rather late wakeup: %lld usecs\n", usecs_late);
        }
        #endif

        // update the x-axis values
        m_current_time_ticks = m_next_time_ticks;

        // decide what coefficients to use
        double coeff_b, coeff_c;
        if (m_high_bw_updates > 0) {
            coeff_b = DLL_COEFF_B_HIGH;
            coeff_c = DLL_COEFF_C_HIGH;
            m_high_bw_updates--;
            if (m_high_bw_updates == 0) {
                debugOutput(DEBUG_LEVEL_VERBOSE,
                            "Switching to low-bandwidth coefficients\n");
            }
        } else {
            coeff_b = DLL_COEFF_B;
            coeff_c = DLL_COEFF_C;
        }

        // it should be ok to not do this in tick space 
        // since diff_ticks_corr should not be near wrapping
        // (otherwise we are out of range. we need a few calls
        //  w/o wrapping for this to work. That should not be
        //  an issue as long as the update interval is smaller
        //  than the wrapping interval.)
        // and coeff_b < 1, hence tmp is not near wrapping

        double step_ticks = (coeff_b * diff_ticks_corr);
        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE,
                           "diff_ticks_corr=%f, step_ticks=%f\n",
                           diff_ticks_corr, step_ticks);

        // the same goes for m_dll_e2, which should be approx equal
        // to the ticks/usec rate (= 24.576) hence also not near
        // wrapping
        step_ticks += m_dll_e2;
        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE,
                           "add %f ticks to step_ticks => step_ticks=%f\n",
                           m_dll_e2, step_ticks);

        // it can't be that we have to update to a value in the past
        if(step_ticks < 0) {
            debugError("negative step: %f! (correcting to nominal)\n", step_ticks);
            // recover to an estimated value
            step_ticks = (double)m_ticks_per_update;
        }

        if(step_ticks > TICKS_PER_SECOND) {
            debugWarning("rather large step: %f ticks (> 1sec)\n", step_ticks);
        }

        // now add the step ticks with wrapping.
        m_next_time_ticks = (double)(addTicks((uint64_t)m_current_time_ticks, (uint64_t)step_ticks));

        // update the DLL state
        m_dll_e2 += coeff_c * diff_ticks_corr;

        // update the y-axis values
        m_current_time_usecs = m_next_time_usecs;
        m_next_time_usecs += m_usecs_per_update;

        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE, 
                           " usecs: current: %f next: %f usecs_late=%lld ticks_late=%lld\n",
                           m_current_time_usecs, m_next_time_usecs, usecs_late, ticks_late);
        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE,
                           " ticks: current: %f next: %f diff=%f\n",
                           m_current_time_ticks, m_next_time_ticks, diff_ticks);
        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE,
                           " ticks: current: %011llu (%03us %04ucy %04uticks)\n",
                           (uint64_t)m_current_time_ticks,
                           (unsigned int)TICKS_TO_SECS( (uint64_t)m_current_time_ticks ),
                           (unsigned int)TICKS_TO_CYCLES( (uint64_t)m_current_time_ticks ),
                           (unsigned int)TICKS_TO_OFFSET( (uint64_t)m_current_time_ticks ) );
        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE,
                           " ticks: next   : %011llu (%03us %04ucy %04uticks)\n",
                           (uint64_t)m_next_time_ticks,
                           (unsigned int)TICKS_TO_SECS( (uint64_t)m_next_time_ticks ),
                           (unsigned int)TICKS_TO_CYCLES( (uint64_t)m_next_time_ticks ),
                           (unsigned int)TICKS_TO_OFFSET( (uint64_t)m_next_time_ticks ) );

        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE,
                           " state: local: %11llu, dll_e2: %f, rate: %f\n",
                           local_time, m_dll_e2, getRate());
    }

    // prepare the new compute vars
    struct compute_vars new_vars;
    new_vars.ticks = (uint64_t)(m_current_time_ticks);
    new_vars.usecs = (uint64_t)m_current_time_usecs;
    new_vars.rate = getRate();
    
    // get the next index
    unsigned int next_idx = (m_current_shadow_idx + 1) % CTRHELPER_NB_SHADOW_VARS;
    
    // check whether next index position is in use
    while(m_shadow_usecount[next_idx] > 0) {
        debugOutput(DEBUG_LEVEL_VERBOSE,
                    "next shadow position still in use (pos: %d, %d uses)",
                    next_idx, m_shadow_usecount[next_idx]);

        // sleep for some time
        m_TimeSource.SleepUsecRelative(100);
    }
    //debugOutputExtreme(DEBUG_LEVEL_VERBOSE, " updating %d\n", next_idx);

    // the next index position is now unused, so we can update it
    m_shadow_vars[next_idx] = new_vars;

    // then we can update the current index
    m_current_shadow_idx = next_idx;
    //debugOutputExtreme(DEBUG_LEVEL_VERBOSE, " updated %d\n", m_current_shadow_idx);
    
    // reclaim is not necessary since it's a preallocated array
    
    return true;
}

uint32_t
CycleTimerHelper::getCycleTimerTicks()
{
    uint64_t now = m_Parent.getCurrentTimeAsUsecs();
    return getCycleTimerTicks(now);
}

uint32_t
CycleTimerHelper::getCycleTimerTicks(uint64_t now)
{
    uint32_t retval;
    struct compute_vars my_vars;

    // get pointer and prevent it from disappearing
    // by marking it as in-use
    unsigned int curr_idx;
    do {
        // get the current shadow var set idx
        curr_idx = m_current_shadow_idx;
        // mark it as used
        INC_ATOMIC(&m_shadow_usecount[curr_idx]);
    
        // check whether the pointer changed between getting the idx
        // and marking it as used
        if (curr_idx != m_current_shadow_idx) {
            debugOutput(DEBUG_LEVEL_VERBOSE,
                        "pointer (%d) changed to (%d) while marking it as used\n",
                        curr_idx, m_current_shadow_idx);
            // undo in-use marking
            DEC_ATOMIC(&m_shadow_usecount[curr_idx]);
            // this requires a retry to obtain the new pointer
        }
    } while(curr_idx != m_current_shadow_idx); // retry if needed
    //debugOutputExtreme(DEBUG_LEVEL_VERBOSE, " using %d\n",curr_idx);

    // copy the contents
    my_vars = m_shadow_vars[curr_idx];
    
    // release the pointer
    DEC_ATOMIC(&m_shadow_usecount[curr_idx]);
    
    int64_t time_diff = now - my_vars.usecs;
    double y_step_in_ticks = ((double)time_diff) * my_vars.rate;
    int64_t y_step_in_ticks_int = (int64_t)y_step_in_ticks;
    uint64_t offset_in_ticks_int = my_vars.ticks;

    if (y_step_in_ticks_int > 0) {
        retval = addTicks(offset_in_ticks_int, y_step_in_ticks_int);
/*        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE, "y_step_in_ticks_int > 0: %lld, time_diff: %f, rate: %f, retval: %lu\n", 
                    y_step_in_ticks_int, time_diff, my_vars.rate, retval);*/
    } else {
        retval = substractTicks(offset_in_ticks_int, -y_step_in_ticks_int);

        // this can happen if the update thread was woken up earlier than it should have been
/*        debugOutputExtreme(DEBUG_LEVEL_VERY_VERBOSE, "y_step_in_ticks_int <= 0: %lld, time_diff: %f, rate: %f, retval: %lu\n", 
                    y_step_in_ticks_int, time_diff, my_vars.rate, retval);*/
    }

    return retval;
}

uint32_t
CycleTimerHelper::getCycleTimer()
{
    uint64_t now = m_Parent.getCurrentTimeAsUsecs();
    return getCycleTimer(now);
}

uint32_t
CycleTimerHelper::getCycleTimer(uint64_t now)
{
    uint32_t ticks = getCycleTimerTicks(now);
    uint32_t result = TICKS_TO_CYCLE_TIMER(ticks);
#ifdef DEBUG
    if(CYCLE_TIMER_TO_TICKS(result) != ticks) {
        debugWarning("Bad ctr conversion");
    }
#endif
    return result;
}

#else

float
CycleTimerHelper::getRate()
{
    return getNominalRate();
}

float
CycleTimerHelper::getNominalRate()
{
    float rate = ((double)TICKS_PER_SECOND) / 1000000.0;
    return rate;
}

bool
CycleTimerHelper::Execute()
{
    usleep(1000*1000);
    return true;
}

uint32_t
CycleTimerHelper::getCycleTimerTicks()
{
    return CYCLE_TIMER_TO_TICKS(getCycleTimer());
}

uint32_t
CycleTimerHelper::getCycleTimerTicks(uint64_t now)
{
    debugWarning("not implemented!\n");
    return getCycleTimerTicks();
}

uint32_t
CycleTimerHelper::getCycleTimer()
{
    uint32_t cycle_timer;
    uint64_t local_time;
    readCycleTimerWithRetry(&cycle_timer, &local_time, 10);
    return cycle_timer;
}

uint32_t
CycleTimerHelper::getCycleTimer(uint64_t now)
{
    debugWarning("not implemented!\n");
    return getCycleTimer();
}

#endif

bool
CycleTimerHelper::readCycleTimerWithRetry(uint32_t *cycle_timer, uint64_t *local_time, int ntries)
{
    bool good=false;
    int maxtries = ntries;

    do {
        // the ctr read can return 0 sometimes. if that happens, reread the ctr.
        int maxtries2=ntries;
        do {
            if(!m_Parent.readCycleTimerReg(cycle_timer, local_time)) {
                debugError("Could not read cycle timer register\n");
                return false;
            }
            if (*cycle_timer == 0) {
                debugOutput(DEBUG_LEVEL_VERBOSE,
                           "Bogus CTR: %08X on try %02d\n",
                           *cycle_timer, maxtries2);
            }
        } while (*cycle_timer == 0 && maxtries2--);
        
        // catch bogus ctr reads (can happen)
        uint64_t cycle_timer_ticks = CYCLE_TIMER_TO_TICKS(*cycle_timer);
    
        if (diffTicks(cycle_timer_ticks, m_cycle_timer_ticks_prev) < 0) {
            debugOutput( DEBUG_LEVEL_VERY_VERBOSE,
                        "non-monotonic CTR (try %02d): %llu -> %llu\n",
                        maxtries, m_cycle_timer_ticks_prev, cycle_timer_ticks);
            debugOutput( DEBUG_LEVEL_VERY_VERBOSE,
                        "                            : %08X -> %08X\n",
                        m_cycle_timer_prev, *cycle_timer);
            debugOutput( DEBUG_LEVEL_VERY_VERBOSE,
                        " current: %011llu (%03us %04ucy %04uticks)\n",
                        cycle_timer_ticks,
                        (unsigned int)TICKS_TO_SECS( cycle_timer_ticks ),
                        (unsigned int)TICKS_TO_CYCLES( cycle_timer_ticks ),
                        (unsigned int)TICKS_TO_OFFSET( cycle_timer_ticks ) );
            debugOutput( DEBUG_LEVEL_VERY_VERBOSE,
                        " prev   : %011llu (%03us %04ucy %04uticks)\n",
                        m_cycle_timer_ticks_prev,
                        (unsigned int)TICKS_TO_SECS( m_cycle_timer_ticks_prev ),
                        (unsigned int)TICKS_TO_CYCLES( m_cycle_timer_ticks_prev ),
                        (unsigned int)TICKS_TO_OFFSET( m_cycle_timer_ticks_prev ) );
        } else {
            good = true;
            m_cycle_timer_prev = *cycle_timer;
            m_cycle_timer_ticks_prev = cycle_timer_ticks;
        }
    } while (!good && maxtries--);
    return true;
}

void
CycleTimerHelper::setVerboseLevel(int l)
{
    setDebugLevel(l);
}
