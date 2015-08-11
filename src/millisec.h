/******************************************************************************/
/**
@file
@author		Scott Ronald Fazackerley, Graeme Douglas
@brief		A timing library for embedded platforms.
@details	Platform specific code to allow for millisecond resolution
		clocks.
*/
/******************************************************************************/

#ifndef MILLISEC_H_
#define MILLISEC_H_

/**
 *		Compile option for *nix systems (Linux, OS X specifically).
*/
#define MILLISEC_PLATFORM_UNIX	0

/**
@brief		Compile option for AVR platform.
*/
#define MILLISEC_PLATFORM_AVR	1

/**
@brief		Target compilation platform.
*/
#define MILLISEC_PLATFORM	MILLISEC_PLATFORM_UNIX

#ifdef  __cplusplus
extern "C" {
#endif

#if MILLISEC_PLATFORM == MILLISEC_PLATFORM_AVR
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#else
#include <time.h>
#include <math.h>
#endif

#define F_CPU 16000000UL

#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 8)

/**
@brief		Type for storing the number of milliseconds.
*/
typedef unsigned long long	milliseconds_t;

/**
@brief		Current number of milliseconds.
@details	For certain platforms, this need not be used (*nix OSs for
		instance) since they manage there own system times since the
		standard Epoch.
*/
static volatile milliseconds_t 	ms_current_millis;

/**
@brief		Base time or "local epoch".
@details	The base time is by default at or "near" the systems epoch.
		The systems epoch for *nix systems is the UNIX epoch (1/1/1970
		at 00:00:00). For embedded platforms, the epoch is 0 and
		is the moment this library is initialized.
		The base time can be reconfigured with @ref ms_set_base.
		However, @ref ms_milliseconds is not relative.
		To get time relative to the base time, use
		@ref ms_get_time_relative.
*/
static volatile milliseconds_t	ms_base_millis;

/**
@brief		Macro for getting base milliseconds.
*/
#define MS_GET_BASE_MILLIS	ms_base_millis

/**
@brief		Get the current time since epoch in milliseconds.
*/
milliseconds_t
ms_milliseconds(
);

/**
@brief		Initializer the milliseconds.
**/
void
ms_init(
);

/**
@brief		Get the time relative to the epoch.
@returns	The time relative to the epoch.
*/
milliseconds_t
ms_get_time_relative(
);

/**
@brief		Set the current time (not the epoch).
@details	This adjusts the current position of the time.
@param		new_time
			The new time, in milliseconds, to set the current
			clock timer to.
*/
void
ms_set_time(
	milliseconds_t	new_time
);

/**
@brief		Set the base time to a different reference point.
@details	This will not change the behaviour of @ref ms_milliseconds.
		This will, however, change what @ref ms_get_time_relative
		returns.
@param		new_time
			The new time, in milliseconds, to set the base time
			to.
*/
void
ms_set_base(
	milliseconds_t	new_time
);

#ifdef  __cplusplus
}
#endif

#endif /* MILLISEC_H_ */
