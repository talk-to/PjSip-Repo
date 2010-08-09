/* $Id$ */
/* 
 * Copyright (C)2003-2006 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#ifndef __PJ_LOG_H__
#define __PJ_LOG_H__

/**
 * @file log.h
 * @brief Logging Utility.
 */

#include <pj/types.h>
#include <stdarg.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJ_MISC Miscelaneous
 * @ingroup PJ
 */

/**
 * @defgroup PJ_LOG Logging Facility
 * @ingroup PJ_MISC
 * @{
 *
 * The PJLIB logging facility is a configurable, flexible, and convenient
 * way to write logging or trace information.
 *
 * To write to the log, one uses construct like below:
 *
 * <pre>
 *   ...
 *   PJ_LOG(3, ("main.c", "Starting hello..."));
 *   ...
 *   PJ_LOG(3, ("main.c", "Hello world from process %d", pj_getpid()));
 *   ...
 * </pre>
 *
 * In the above example, the number @b 3 controls the verbosity level of
 * the information (which means "information", by convention). The string
 * "main.c" specifies the source or sender of the message.
 *
 *
 * \section pj_log_quick_sample_sec Examples
 *
 * For examples, see:
 *  - @ref page_pjlib_samples_log_c.
 *
 */

/**
 * Log decoration flag, to be specified with #pj_log_set_decor().
 */
enum pj_log_decoration
{
    PJ_LOG_HAS_DAY_NAME   =   1, /**< Include day name [default: no].	     */
    PJ_LOG_HAS_YEAR       =   2, /**< Include year digit [default: no]	     */
    PJ_LOG_HAS_MONTH	  =   4, /**< Include month [default: no]	     */
    PJ_LOG_HAS_DAY_OF_MON =   8, /**< Include day of month [default: no]     */
    PJ_LOG_HAS_TIME	  =  16, /**< Include time [default: yes].	     */
    PJ_LOG_HAS_MICRO_SEC  =  32, /**< Include microseconds [yes]             */
    PJ_LOG_HAS_SENDER	  =  64, /**< Include sender in the log [yes].	     */
    PJ_LOG_HAS_NEWLINE	  = 128, /**< Terminate each call with newline [yes].*/
};

/**
 * Write log message.
 * This is the main macro used to write text to the logging backend. 
 *
 * @param level	    The logging verbosity level. Lower number indicates higher
 *		    importance, with level zero indicates fatal error. Only
 *		    numeral argument is permitted (e.g. not variable).
 * @param arg	    Enclosed 'printf' like arguments, with the first 
 *		    argument is the sender, the second argument is format 
 *		    string and the following arguments are variable number of 
 *		    arguments suitable for the format string.
 *
 * Sample:
 * \verbatim
   PJ_LOG(2, (__FILE__, "current value is %d", value));
   \endverbatim
 * @hideinitializer
 */
#define PJ_LOG(level,arg)	pj_log_wrapper_##level(arg)

/**
 * Signature for function to be registered to the logging subsystem to
 * write the actual log message to some output device.
 *
 * @param level	    Log level.
 * @param data	    Log message.
 * @param len	    Message length.
 */
typedef void pj_log_func(int level, const char *data, int len);

/**
 * Default logging writer function used by front end logger function.
 * Application normally should NOT need to call this function, but
 * rather use the PJ_LOG macro.
 *
 * @param level	    Log level.
 * @param buffer    Log message.
 * @param len	    Message length.
 */
PJ_DECL(void) pj_log_write(int level, const char *buffer, int len);


#if PJ_LOG_MAX_LEVEL >= 1

/**
 * Write to log.
 *
 * @param sender    Source of the message.
 * @param level	    Verbosity level.
 * @param format    Format.
 * @param marker    Marker.
 */
PJ_DECL(void) pj_log(const char *sender, int level, 
		     const char *format, va_list marker);

/**
 * Change log output function. The front-end logging functions will call
 * this function to write the actual message to the desired device. 
 * By default, the front-end functions use pj_log_write() to write
 * the messages, unless it's changed by calling this function.
 *
 * @param func	    The function that will be called to write the log
 *		    messages to the desired device.
 */
PJ_DECL(void) pj_log_set_log_func( pj_log_func *func );

/**
 * Get the current log output function that is used to write log messages.
 *
 * @return	    Current log output function.
 */
PJ_DECL(pj_log_func*) pj_log_get_log_func(void);

/**
 * Set maximum log level. Application can call this function to set 
 * the desired level of verbosity of the logging messages. The bigger the
 * value, the more verbose the logging messages will be printed. However,
 * the maximum level of verbosity can not exceed compile time value of
 * PJ_LOG_MAX_LEVEL.
 *
 * @param level	    The maximum level of verbosity of the logging
 *		    messages (6=very detailed..1=error only, 0=disabled)
 */
PJ_DECL(void) pj_log_set_level(int level);

/**
 * Get current maximum log verbositylevel.
 *
 * @return	    Current log maximum level.
 */
PJ_DECL(int) pj_log_get_level(void);

/**
 * Set log decoration. The log decoration flag controls what are printed
 * to output device alongside the actual message. For example, application
 * can specify that date/time information should be displayed with each
 * log message.
 *
 * @param decor	    Bitmask combination of #pj_log_decoration to control
 *		    the layout of the log message.
 */
PJ_DECL(void) pj_log_set_decor(unsigned decor);

/**
 * Get current log decoration flag.
 *
 * @return	    Log decoration flag.
 */
PJ_DECL(unsigned) pj_log_get_decor(void);


#else	/* #if PJ_LOG_MAX_LEVEL >= 1 */

/**
 * Change log output function. The front-end logging functions will call
 * this function to write the actual message to the desired device. 
 * By default, the front-end functions use pj_log_write() to write
 * the messages, unless it's changed by calling this function.
 *
 * @param func	    The function that will be called to write the log
 *		    messages to the desired device.
 */
#  define pj_log_set_log_func(func)

/**
 * Set maximum log level. Application can call this function to set 
 * the desired level of verbosity of the logging messages. The bigger the
 * value, the more verbose the logging messages will be printed. However,
 * the maximum level of verbosity can not exceed compile time value of
 * PJ_LOG_MAX_LEVEL.
 *
 * @param level	    The maximum level of verbosity of the logging
 *		    messages (6=very detailed..1=error only, 0=disabled)
 */
#  define pj_log_set_level(level)

/**
 * Set log decoration. The log decoration flag controls what are printed
 * to output device alongside the actual message. For example, application
 * can specify that date/time information should be displayed with each
 * log message.
 *
 * @param decor	    Bitmask combination of #pj_log_decoration to control
 *		    the layout of the log message.
 */
#  define pj_log_set_decor(decor)

#endif	/* #if PJ_LOG_MAX_LEVEL >= 1 */

/** 
 * @}
 */

///////////////////////////////////////////////////////////////////////////////
/*
 * Log functions implementation prototypes.
 * These functions are called by PJ_LOG macros according to verbosity
 * level specified when calling the macro. Applications should not normally
 * need to call these functions directly.
 */

/**
 * @def pj_log_wrapper_1(arg)
 * Internal function to write log with verbosity 1. Will evaluate to
 * empty expression if PJ_LOG_MAX_LEVEL is below 1.
 * @param arg       Log expression.
 */
#if PJ_LOG_MAX_LEVEL >= 1
    #define pj_log_wrapper_1(arg)	pj_log_1 arg
    /** Internal function. */
    PJ_DECL(void) pj_log_1(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_1(arg)
#endif

/**
 * @def pj_log_wrapper_2(arg)
 * Internal function to write log with verbosity 2. Will evaluate to
 * empty expression if PJ_LOG_MAX_LEVEL is below 2.
 * @param arg       Log expression.
 */
#if PJ_LOG_MAX_LEVEL >= 2
    #define pj_log_wrapper_2(arg)	pj_log_2 arg
    /** Internal function. */
    PJ_DECL(void) pj_log_2(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_2(arg)
#endif

/**
 * @def pj_log_wrapper_3(arg)
 * Internal function to write log with verbosity 3. Will evaluate to
 * empty expression if PJ_LOG_MAX_LEVEL is below 3.
 * @param arg       Log expression.
 */
#if PJ_LOG_MAX_LEVEL >= 3
    #define pj_log_wrapper_3(arg)	pj_log_3 arg
    /** Internal function. */
    PJ_DECL(void) pj_log_3(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_3(arg)
#endif

/**
 * @def pj_log_wrapper_4(arg)
 * Internal function to write log with verbosity 4. Will evaluate to
 * empty expression if PJ_LOG_MAX_LEVEL is below 4.
 * @param arg       Log expression.
 */
#if PJ_LOG_MAX_LEVEL >= 4
    #define pj_log_wrapper_4(arg)	pj_log_4 arg
    /** Internal function. */
    PJ_DECL(void) pj_log_4(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_4(arg)
#endif

/**
 * @def pj_log_wrapper_5(arg)
 * Internal function to write log with verbosity 5. Will evaluate to
 * empty expression if PJ_LOG_MAX_LEVEL is below 5.
 * @param arg       Log expression.
 */
#if PJ_LOG_MAX_LEVEL >= 5
    #define pj_log_wrapper_5(arg)	pj_log_5 arg
    /** Internal function. */
    PJ_DECL(void) pj_log_5(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_5(arg)
#endif

/**
 * @def pj_log_wrapper_6(arg)
 * Internal function to write log with verbosity 6. Will evaluate to
 * empty expression if PJ_LOG_MAX_LEVEL is below 6.
 * @param arg       Log expression.
 */
#if PJ_LOG_MAX_LEVEL >= 6
    #define pj_log_wrapper_6(arg)	pj_log_6 arg
    /** Internal function. */
    PJ_DECL(void) pj_log_6(const char *src, const char *format, ...);
#else
    #define pj_log_wrapper_6(arg)
#endif


PJ_END_DECL 

#endif  /* __PJ_LOG_H__ */
