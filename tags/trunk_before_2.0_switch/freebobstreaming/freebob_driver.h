/* freebob_driver.h
 *
 *   FreeBob Backend for Jack
 *   FreeBob = Firewire (pro-)audio for linux
 *
 *   http://freebob.sf.net
 *   http://jackit.sf.net
 *
 *   Copyright (C) 2005 Pieter Palmers <pieterpalmers@users.sourceforge.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* 
 * Main Jack driver entry routines
 *
 */ 
 
#ifndef __JACK_FREEBOB_DRIVER_H__
#define __JACK_FREEBOB_DRIVER_H__

#define FREEBOB_DRIVER_WITH_MIDI

#include <libfreebob/freebob.h>

#include <jack/driver.h>
#include <jack/engine.h>
#include <jack/types.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <poll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <endian.h>

#include <pthread.h>
#include <semaphore.h>

#include <jack/internal.h>
#include <jack/types.h>
#include <jack/ringbuffer.h>
#include <jack/driver.h>
#include <jack/engine.h>
#include <jack/types.h>
#include <jack/thread.h>

#include <alsa/asoundlib.h>

#include "freebob_streaming.h"

#ifndef DEBUG
//#define DEBUG
#endif

#define FREEBOB_USE_RT 			1
#define FREEBOB_RT_PRIORITY_PACKETIZER 	60
// midi priority should be higher than the audio priority in order to
// make sure events are not only delivered on period boundarys
// but I think it should be smaller than the packetizer thread in order not 
// to lose any packets
#define FREEBOB_RT_PRIORITY_MIDI 	59

unsigned long getCurrentUTime();

static unsigned long debugGetCurrentUTime() {
	struct timeval now;
	gettimeofday(&now, NULL);
	return now.tv_sec*1000000+now.tv_usec;
}

// debug print control flags
#define DEBUG_LEVEL_BUFFERS           	(1<<0)
#define DEBUG_LEVEL_HANDLERS			(1<<1)
#define DEBUG_LEVEL_XRUN_RECOVERY     	(1<<2)
#define DEBUG_LEVEL_WAIT     			(1<<3)

#define DEBUG_LEVEL_RUN_CYCLE         	(1<<8)

#define DEBUG_LEVEL_PACKETCOUNTER		(1<<16)
#define DEBUG_LEVEL_STARTUP				(1<<17)
#define DEBUG_LEVEL_THREADS				(1<<18)

// default debug level
#define DEBUG_LEVEL (  DEBUG_LEVEL_RUN_CYCLE | (DEBUG_LEVEL_XRUN_RECOVERY)| DEBUG_LEVEL_STARTUP | DEBUG_LEVEL_WAIT | DEBUG_LEVEL_PACKETCOUNTER)

#ifdef DEBUG
	#define printMessage(format, args...) fprintf( stderr, "FBDRV MSG: %s:%d (%s): " format,  __FILE__, __LINE__, __FUNCTION__, ##args )
	#define printError(format, args...) fprintf( stderr, "FBDRV ERR: %s:%d (%s): " format,  __FILE__, __LINE__, __FUNCTION__, ##args )
	
	//#define printEnter() fprintf( stderr, "FBDRV ENTERS: %s (%s)\n", __FUNCTION__,  __FILE__)
	//#define printExit() fprintf( stderr, "FBDRV EXITS: %s (%s)\n", __FUNCTION__,  __FILE__)
	#define printEnter() 
	#define printExit() 
	
	#define debugError(format, args...) fprintf( stderr, "FREEBOB ERR: %s:%d (%s): " format,  __FILE__, __LINE__, __FUNCTION__, ##args )
	#define debugPrint(Level, format, args...) if(DEBUG_LEVEL & (Level))  fprintf( stderr,"DEBUG %s:%d (%s) :"  format, __FILE__, __LINE__, __FUNCTION__, ##args );
	#define debugPrintShort(Level, format, args...) if(DEBUG_LEVEL & (Level))  fprintf( stderr, format,##args );
	#define debugPrintWithTimeStamp(Level, format, args...) if(DEBUG_LEVEL & (Level)) fprintf( stderr, "%16lu: "format, debugGetCurrentUTime(),##args );
	#define SEGFAULT int *test=NULL;	*test=1;
#else
	#define DEBUG_LEVEL
	
	#define printMessage(format, args...) fprintf( stderr, "FBDRV MSG: " format, ##args )
	#define printError(format, args...)   fprintf( stderr, "FBDRV ERR: " format, ##args )
	
	#define printEnter() 
	#define printExit() 
	
	#define debugError(format, args...) 
	#define debugPrint(Level, format, args...) 
	#define debugPrintShort(Level, format, args...)	
	#define debugPrintWithTimeStamp(Level, format, args...)
#endif


/*
 * Jack Driver command line parameters
 */
 
typedef struct _freebob_jack_settings freebob_jack_settings_t;
struct _freebob_jack_settings {
	int period_size_set;
	jack_nframes_t period_size;
	
	int sample_rate_set;
	int sample_rate;
	
	int fifo_size_set;
	jack_nframes_t fifo_size;
	
	int table_size_set;
	jack_nframes_t table_size;
	
	int iso_buffers_set;
	jack_nframes_t iso_buffers;
	
	int iso_prebuffers_set;
	jack_nframes_t iso_prebuffers;
	
	int iso_irq_interval_set;
	int iso_irq_interval;
	
	int buffer_size_set;
	jack_nframes_t buffer_size;

        int port_set;
        int port;
       
        int node_id_set;
        int node_id;

        freebob_handle_t fb_handle;
};

#ifdef FREEBOB_DRIVER_WITH_MIDI

#define ALSA_SEQ_BUFF_SIZE 1024
#define MIDI_TRANSMIT_BUFFER_SIZE 1024
#define MIDI_THREAD_SLEEP_TIME_USECS 100

typedef struct {
	int stream_nr;
	int seq_port_nr;
	snd_midi_event_t *parser;
	snd_seq_t *seq_handle;
} freebob_midi_port_t;

typedef struct _freebob_driver_midi_handle {
	freebob_device_t *dev;
	
	snd_seq_t *seq_handle;
	
	pthread_t queue_thread;
	pthread_t dequeue_thread;
	int queue_thread_realtime;
	int queue_thread_priority;

	int nb_input_ports;
	int nb_output_ports;

	freebob_midi_port_t **input_ports;
	freebob_midi_port_t **output_ports;

	freebob_midi_port_t **input_stream_port_map;
	int *output_port_stream_map;


} freebob_driver_midi_handle_t;

#endif
/*
 * JACK driver structure
 */
 
typedef struct _freebob_driver freebob_driver_t;

struct _freebob_driver
{
	JACK_DRIVER_NT_DECL
	
	jack_nframes_t  sample_rate;
	jack_nframes_t  period_size;
	unsigned long   wait_time;

    jack_time_t                   wait_last;
    jack_time_t                   wait_next;
	int wait_late;
	
	jack_client_t  *client;
	
	int		xrun_detected;
	int		xrun_count;
	
	int process_count;
	
	/* settings from the command line */
	freebob_jack_settings_t settings;
	
	/* the freebob virtual device */
	freebob_device_t *dev;
	
    JSList                       *capture_ports;
    JSList                       *playback_ports;
    JSList                       *monitor_ports;
    channel_t                     playback_nchannels;
    channel_t                     capture_nchannels;
    	

#ifdef FREEBOB_DRIVER_WITH_MIDI
	freebob_driver_midi_handle_t *midi_handle;
#endif

}; 



#endif /* __JACK_FREEBOB_DRIVER_H__ */


