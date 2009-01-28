/* $Id$ */
/* 
 * Copyright (C) 2008-2009 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
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
#ifndef __PJMEDIA_CODEC_CONFIG_H__
#define __PJMEDIA_CODEC_CONFIG_H__

#include <pjmedia/types.h>

/*
 * Include config_auto.h if autoconf is used (PJ_AUTOCONF is set)
 */
#if defined(PJ_AUTOCONF)
#   include <pjmedia-codec/config_auto.h>
#endif

/**
 * Unless specified otherwise, L16 codec is included by default.
 */
#ifndef PJMEDIA_HAS_L16_CODEC
#   define PJMEDIA_HAS_L16_CODEC    1
#endif


/**
 * Unless specified otherwise, GSM codec is included by default.
 */
#ifndef PJMEDIA_HAS_GSM_CODEC
#   define PJMEDIA_HAS_GSM_CODEC    1
#endif


/**
 * Unless specified otherwise, Speex codec is included by default.
 */
#ifndef PJMEDIA_HAS_SPEEX_CODEC
#   define PJMEDIA_HAS_SPEEX_CODEC    1
#endif

/**
 * Speex codec default complexity setting.
 */
#ifndef PJMEDIA_CODEC_SPEEX_DEFAULT_COMPLEXITY
#   define PJMEDIA_CODEC_SPEEX_DEFAULT_COMPLEXITY   2
#endif

/**
 * Speex codec default quality setting. Please note that pjsua-lib may override
 * this setting via its codec quality setting (i.e PJSUA_DEFAULT_CODEC_QUALITY).
 */
#ifndef PJMEDIA_CODEC_SPEEX_DEFAULT_QUALITY
#   define PJMEDIA_CODEC_SPEEX_DEFAULT_QUALITY	    8
#endif


/**
 * Unless specified otherwise, iLBC codec is included by default.
 */
#ifndef PJMEDIA_HAS_ILBC_CODEC
#   define PJMEDIA_HAS_ILBC_CODEC    1
#endif


/**
 * Unless specified otherwise, G.722 codec is included by default.
 */
#ifndef PJMEDIA_HAS_G722_CODEC
#   define PJMEDIA_HAS_G722_CODEC    1
#endif


/**
 * Enable the features provided by Intel IPP libraries, for example
 * codecs such as G.729, G.723.1, G.726, G.728, G.722.1, and AMR.
 *
 * By default this is disabled. Please follow the instructions in
 * http://trac.pjsip.org/repos/wiki/Intel_IPP_Codecs on how to setup
 * Intel IPP with PJMEDIA.
 */
#ifndef PJMEDIA_HAS_INTEL_IPP
#   define PJMEDIA_HAS_INTEL_IPP		0
#endif


/**
 * Visual Studio only: when this option is set, the Intel IPP libraries
 * will be automatically linked to application using pragma(comment)
 * constructs. This is convenient, however it will only link with
 * the stub libraries and the Intel IPP DLL's will be required when
 * distributing the application.
 *
 * If application wants to link with the different types of the Intel IPP
 * libraries (for example, the static libraries), it must set this option
 * to zero and specify the Intel IPP libraries in the application's input
 * library specification manually.
 *
 * Default 1.
 */
#ifndef PJMEDIA_AUTO_LINK_IPP_LIBS
#   define PJMEDIA_AUTO_LINK_IPP_LIBS		1
#endif


/**
 * Enable Intel IPP AMR codec. This also needs to be enabled when AMR WB
 * codec is enabled. This option is only used when PJMEDIA_HAS_INTEL_IPP 
 * is enabled.
 *
 * Default: 1
 */
#ifndef PJMEDIA_HAS_INTEL_IPP_CODEC_AMR
#   define PJMEDIA_HAS_INTEL_IPP_CODEC_AMR	1
#endif


/**
 * Enable Intel IPP AMR wideband codec. The PJMEDIA_HAS_INTEL_IPP_CODEC_AMR
 * option must also be enabled to use this codec. This option is only used 
 * when PJMEDIA_HAS_INTEL_IPP is enabled.
 *
 * Default: 1
 */
#ifndef PJMEDIA_HAS_INTEL_IPP_CODEC_AMRWB
#   define PJMEDIA_HAS_INTEL_IPP_CODEC_AMRWB	1
#endif


/**
 * Enable Intel IPP G.729 codec. This option is only used when
 * PJMEDIA_HAS_INTEL_IPP is enabled.
 *
 * Default: 1
 */
#ifndef PJMEDIA_HAS_INTEL_IPP_CODEC_G729
#   define PJMEDIA_HAS_INTEL_IPP_CODEC_G729	1
#endif


/**
 * Enable Intel IPP G.723.1 codec. This option is only used when
 * PJMEDIA_HAS_INTEL_IPP is enabled.
 *
 * Default: 1
 */
#ifndef PJMEDIA_HAS_INTEL_IPP_CODEC_G723_1
#   define PJMEDIA_HAS_INTEL_IPP_CODEC_G723_1	1
#endif


/**
 * Enable Intel IPP G.726 codec. This option is only used when
 * PJMEDIA_HAS_INTEL_IPP is enabled.
 *
 * Default: 1
 */
#ifndef PJMEDIA_HAS_INTEL_IPP_CODEC_G726
#   define PJMEDIA_HAS_INTEL_IPP_CODEC_G726	1
#endif


/**
 * Enable Intel IPP G.728 codec. This option is only used when
 * PJMEDIA_HAS_INTEL_IPP is enabled.
 *
 * Default: 1
 */
#ifndef PJMEDIA_HAS_INTEL_IPP_CODEC_G728
#   define PJMEDIA_HAS_INTEL_IPP_CODEC_G728	1
#endif


/**
 * Enable Intel IPP G.722.1 codec. This option is only used when
 * PJMEDIA_HAS_INTEL_IPP is enabled.
 *
 * Default: 1
 */
#ifndef PJMEDIA_HAS_INTEL_IPP_CODEC_G722_1
#   define PJMEDIA_HAS_INTEL_IPP_CODEC_G722_1	1
#endif

/**
 * Enable Passthrough codecs.
 *
 * Default: 0
 */
#ifndef PJMEDIA_HAS_PASSTHROUGH_CODECS
#   define PJMEDIA_HAS_PASSTHROUGH_CODECS	0
#endif

#endif	/* __PJMEDIA_CODEC_CONFIG_H__ */
