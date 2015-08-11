/******************************************************************************/
/**
@file
@author		Scott Ronald Fazackerley, Graeme Douglas
@brief		A timing library for embedded platforms.
@details	For more information, please see @ref millisec.h.
*/
/******************************************************************************/

#include "millisec.h"

#if MILLSEC_PLATFORM == MILLSEC_PLATFORM_UNIX
#ifdef  __MACH__
#include <sys/time.h>
#include <mach/mach_time.h>

#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)

#endif
#endif

#if MILLISEC_PLATFORM == MILLISEC_PLATFORM_AVR
ISR(
	TIMER1_COMPA_vect
)
{
	ms_current_millis++;
}
#endif

void
ms_init(
)
{
#if MILLISEC_PLATFORM == MILLISEC_PLATFORM_AVR
	// CTC mode, Clock/8
	TCCR1B |= (1 << WGM12) | (1 << CS11);
	
	// Load the high byte, then the low byte
	// into the output compare
	OCR1AH = (CTC_MATCH_OVERFLOW >> 8);
	OCR1AL = CTC_MATCH_OVERFLOW;
	
	// Enable the compare match interrupt
	TIMSK1 |= (1 << OCIE1A);
	ms_base_millis = ms_milliseconds();
#endif
	/* We will use the standard UNIX epoch for *nix systems. */
	ms_base_millis = 0;
}

milliseconds_t
ms_milliseconds(
)
{
	milliseconds_t millis_return;
	
#if MILLISEC_PLATFORM == MILLISEC_PLATFORM_AVR
	/* Ensure this cannot be disrupted. */
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		millis_return = ms_current_millis;
	}
#else
#ifdef  __MACH__
	struct timeval time;
	gettimeofday(&time, NULL);
	millis_return = (time.tv_sec * 1000) + (time.tv_usec / 1000);
#else
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	/* Convert nano to milliseconds. */
	millis_return = llround(spec.tv_sec*1000) + llround(spec.tv_nsec/1.0e6);
#endif
#endif
	
	return millis_return;
}

milliseconds_t
ms_get_time_relative(
)
{
	return (ms_milliseconds() - ms_base_millis);
}

void
ms_set_time(
	milliseconds_t	new_time
)
{
#if MILLISEC_PLATFORM == MILLISEC_PLATFORM_AVR
	/* Ensure this cannot be disrupted. */
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
#endif
		ms_current_millis	= new_time;
#if MILLISEC_PLATFORM == MILLISEC_PLATFORM_AVR
	}
#endif
}

void
ms_set_base(
	milliseconds_t	new_time
)
{
#if MILLISEC_PLATFORM == MILLISEC_PLATFORM_AVR
	/* Ensure this cannot be disrupted. */
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
#endif
		ms_base_millis		= new_time;
#if MILLISEC_PLATFORM == MILLISEC_PLATFORM_AVR
	}
#endif
}
