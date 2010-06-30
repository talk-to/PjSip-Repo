/* $Header: /pjproject/pjsip/src/pjsip/sip_transport.h 9     6/17/05 11:16p Bennylp $ */
/* 
 * PJSIP - SIP Stack
 * (C)2003-2005 Benny Prijono <bennylp@bulukucing.org>
 *
 * Author:
 *  Benny Prijono <bennylp@bulukucing.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __PJSIP_SIP_TRANSPORT_H__
#define __PJSIP_SIP_TRANSPORT_H__

/**
 * @file sip_transport.h
 * @brief SIP Transport
 */

#include <pjsip/sip_msg.h>
#include <pjsip/sip_parser.h>
#include <pj/sock.h>
#include <pj/list.h>

PJ_BEGIN_DECL

/**
 * @defgroup PJSIP_TRANSPORT SIP Transport
 * @ingroup PJSIP
 *
 * This is the low-level transport layer. Application normally won't need to 
 * use this function, but instead can use transaction or higher layer API to
 * send and receive messages.
 *
 * @{
 */

/**
 * Incoming message buffer.
 * This structure keep all the information regarding the received message. This
 * buffer lifetime is only very short, normally after the transaction has been
 * called, this buffer will be deleted/recycled. So care must be taken when
 * allocating storage from the pool of this buffer.
 */
struct pjsip_rx_data
{
    PJ_DECL_LIST_MEMBER(struct pjsip_rx_data)

    /** Memory pool for this buffer. */
    pj_pool_t		*pool;

    /** Time when the message was received. */
    pj_time_val		 timestamp;

    /** The packet buffer. */
    char		 packet[PJSIP_MAX_PKT_LEN];

    /** The length of the packet received. */
    int			 len;

    /** The source address from which the packet was received. */
    pj_sockaddr_in	 addr;

    /** The length of the source address. */
    int			 addr_len;

    /** The transport object which received this packet. */
    pjsip_transport_t	*transport;

    /** The parsed message, if any. */
    pjsip_msg		*msg;

    /** This the transaction key generated from the message. This key is only
     *  available after the rdata has reached the endpoint. 
     */
    pj_str_t		 key;

    /** The Call-ID header as found in the message. */
    pj_str_t		 call_id;

    /** The From header as found in the message. */
    pjsip_from_hdr	*from;

    /** The tag in the From header as found in the message. */
    pj_str_t		 from_tag;

    /** The To header as found in the message. */
    pjsip_to_hdr	*to;

    /** The To tag header as found in the message. */
    pj_str_t		 to_tag;

    /** The topmost Via header as found in the message. */
    pjsip_via_hdr	*via;

    /** The CSeq header as found in the message. */
    pjsip_cseq_hdr	*cseq;

    /** The list of error generated by the parser when parsing this message. */
    pjsip_parser_err_report parse_err;
};


/**
 * Data structure for sending outgoing message. Application normally creates
 * this buffer by calling #pjsip_endpt_create_tdata.
 *
 * The lifetime of this buffer is controlled by the reference counter in this
 * structure, which is manipulated by calling #pjsip_tx_data_add_ref and
 * #pjsip_tx_data_dec_ref. When the reference counter has reached zero, then
 * this buffer will be destroyed.
 *
 * A transaction object normally will add reference counter to this buffer
 * when application calls #pjsip_tsx_on_tx_msg, because it needs to keep the
 * message for retransmission. The transaction will release the reference
 * counter once its state has reached final state.
 */
struct pjsip_tx_data
{
    PJ_DECL_LIST_MEMBER(struct pjsip_tx_data)

    /** Memory pool for this buffer. */
    pj_pool_t		*pool;

    /** A name to identify this buffer. */
    char		 obj_name[PJ_MAX_OBJ_NAME];

    /** For response message, this contains the reference to timestamp when 
     *  the original request message was received. The value of this field
     *  is set when application creates response message to a request by
     *  calling #pjsip_endpt_create_response.
     */
    pj_time_val		 rx_timestamp;

    /** The transport manager for this buffer. */
    pjsip_transport_mgr *mgr;

    /** The message in this buffer. */
    pjsip_msg 		*msg;

    /** Buffer to the printed text representation of the message. When the
     *  content of this buffer is set, then the transport will send the content
     *  of this buffer instead of re-printing the message structure. If the
     *  message structure has changed, then application must invalidate this
     *  buffer by calling #pjsip_tx_data_invalidate_msg.
     */
    pjsip_buffer	 buf;

    /** Reference counter. */
    pj_atomic_t		*ref_cnt;
};


/**
 * Add reference counter to the transmit buffer. The reference counter controls
 * the life time of the buffer, ie. when the counter reaches zero, then it 
 * will be destroyed.
 *
 * @param tdata	    The transmit buffer.
 */
PJ_DECL(void) pjsip_tx_data_add_ref( pjsip_tx_data *tdata );

/**
 * Decrement reference counter of the transmit buffer.
 * When the transmit buffer is no longer used, it will be destroyed.
 *
 * @param tdata	    The transmit buffer data.
 */
PJ_DECL(void) pjsip_tx_data_dec_ref( pjsip_tx_data *tdata );

/**
 * Invalidate the print buffer to force message to be re-printed. Call
 * when the message has changed after it has been printed to buffer. The
 * message is printed to buffer normally by transport when it is about to be 
 * sent to the wire. Subsequent sending of the message will not cause
 * the message to be re-printed, unless application invalidates the buffer
 * by calling this function.
 *
 * @param tdata	    The transmit buffer.
 */
PJ_DECL(void) pjsip_tx_data_invalidate_msg( pjsip_tx_data *tdata );


/**
 * Flags for SIP transports.
 */
enum pjsip_transport_flags_e
{
    PJSIP_TRANSPORT_RELIABLE	    = 1,    /**< Transport is reliable. */
    PJSIP_TRANSPORT_SECURE	    = 2,    /**< Transport is secure. */
    PJSIP_TRANSPORT_IOQUEUE_BUSY    = 4,    /**< WTH?? */
};

/**
 * Get the transport type from the transport name.
 *
 * @param name	    Transport name, such as "TCP", or "UDP".
 *
 * @return	    The transport type, or PJSIP_TRANSPORT_UNSPECIFIED if 
 *		    the name is not recognized as the name of supported 
 *		    transport.
 */
PJ_DECL(pjsip_transport_type_e) 
pjsip_transport_get_type_from_name(const pj_str_t *name);

/**
 * Get the transport type for the specified flags.
 *
 * @param flag	    The transport flag.
 *
 * @return	    Transport type.
 */
PJ_DECL(pjsip_transport_type_e) 
pjsip_transport_get_type_from_flag(unsigned flag);

/**
 * Get the default SIP port number for the specified type.
 *
 * @param type	    Transport type.
 *
 * @return	    The port number, which is the default SIP port number for
 *		    the specified type.
 */
PJ_DECL(int) 
pjsip_transport_get_default_port_for_type(pjsip_transport_type_e type);


/**
 * Add reference to transport.
 * Transactions or dialogs that uses a particular transport must call this 
 * function to indicate that the transport is being used, thus preventing the
 * transport from being closed.
 *
 * @param transport	The transport.
 */
PJ_DECL(void) 
pjsip_transport_add_ref( pjsip_transport_t *transport );

/**
 * Decrease reference to transport.
 * When the transport reference counter becomes zero, a timer will be started
 * and when this timer expires and the reference counter is still zero, the
 * transport will be released.
 *
 * @param transport	The transport
 */
PJ_DECL(void) 
pjsip_transport_dec_ref( pjsip_transport_t *transport );


/**
 * Macro to check whether the transport is reliable.
 *
 * @param transport	The transport
 *
 * @return		non-zero (not necessarily 1) if transport is reliable.
 */
#define PJSIP_TRANSPORT_IS_RELIABLE(transport)	\
	(pjsip_transport_get_flag(transport) & PJSIP_TRANSPORT_RELIABLE)


/**
 * Macro to check whether the transport is secure.
 *
 * @param transport	The transport
 *
 * @return		non-zero (not necessarily one) if transport is secure.
 */
#define PJSIP_TRANSPORT_IS_SECURE(transport)	\
	(pjsip_transport_get_flag(transport) & PJSIP_TRANSPORT_SECURE)

/**
 * Get the transport type.
 *
 * @param tr		The transport.
 *
 * @return		Transport type.
 */
PJ_DECL(pjsip_transport_type_e) 
pjsip_transport_get_type( const pjsip_transport_t * tr);

/**
 * Get the transport type name (ie "UDP", or "TCP").
 *
 * @param tr		The transport.
 * @return		The string type.
 */
PJ_DECL(const char *) 
pjsip_transport_get_type_name( const pjsip_transport_t * tr);

/**
 * Get the transport's object name.
 *
 * @param tr		The transport.
 * @return		The object name.
 */
PJ_DECL(const char*) 
pjsip_transport_get_obj_name( const pjsip_transport_t *tr );

/**
 * Get the transport's reference counter.
 *
 * @param tr		The transport.
 * @return		The reference count value.
 */
PJ_DECL(int) 
pjsip_transport_get_ref_cnt( const pjsip_transport_t *tr );

/**
 * Get transport flag.
 *
 * @param tr		The transport.
 * @return		Transport flag.
 */
PJ_DECL(unsigned) 
pjsip_transport_get_flag( const pjsip_transport_t * tr );

/**
 * Get the local address of the transport, ie. the address which the socket
 * is bound.
 *
 * @param tr		The transport.
 * @return		The address.
 */
PJ_DECL(const pj_sockaddr_in *) 
pjsip_transport_get_local_addr( pjsip_transport_t * tr );

/**
 * Get the address name of the transport. Address name can be an arbitrary
 * address assigned to a transport. This is usefull for example when STUN
 * is used, then the address name of an UDP transport can specify the public
 * address of the transport. When the address name is not set, then value
 * will be equal to the local/bound address. Application should normally
 * prefer to use the address name instead of the local address.
 *
 * @param tr		The transport.
 * @return		The address name.
 */
PJ_DECL(const pj_sockaddr_in*) 
pjsip_transport_get_addr_name (pjsip_transport_t *tr);

/**
 * Get the remote address of the transport. Not all transports will have 
 * a valid remote address. UDP transports, for example, will likely to have
 * zero has their remote address, because UDP transport can be used to send
 * and receive messages from multiple destinations.
 *
 * @param tr		The transport.
 * @return		The address.
 */
PJ_DECL(const pj_sockaddr_in *) 
pjsip_transport_get_remote_addr( const pjsip_transport_t * tr );

/**
 * Send a SIP message using the specified transport, to the address specified
 * in the outgoing data. This function is only usefull for application when it
 * wants to handle the message statelessly, because otherwise it should create
 * a transaction and let the transaction handles the transmission of the 
 * message.
 *
 * This function will send the message immediately, so application must be
 * sure that the transport is ready to do so before calling this function.
 *
 * @param tr		The transport to send the message.
 * @param tdata		The outgoing message buffer.
 * @param addr		The remote address.
 *
 * @return		The number of bytes sent, or zero if the connection 
 *			has closed, or -1 on error.
 */
PJ_DECL(int) pjsip_transport_send_msg( pjsip_transport_t *tr, 
				       pjsip_tx_data *tdata,
				       const pj_sockaddr_in *addr);


/**
 * @}
 */

/*
 * PRIVATE FUNCTIONS!!!
 *
 * These functions are normally to be used by endpoint. Application should
 * use the variant provided by the endpoint instance.
 *
 * Application normally wouldn't be able to call these functions because it
 * has no reference of the transport manager (the instance of the transport
 * manager is hidden by endpoint!).
 */

/*
 * Create a new transmit buffer.
 *
 * @param mgr		The transport manager.
 * @return		The transmit buffer data, or NULL on error.
 */
pjsip_tx_data* pjsip_tx_data_create( pjsip_transport_mgr *mgr );


/**
 * Create listener.
 *
 * @param mgr		The transport manager.
 * @param type		Transport type.
 * @param local_addr	The address to bind.
 * @param addr_name	If not null, sets the address name. If NULL, 
 *			then the local address will be used.
 *
 * @return		PJ_OK if listener was successfully created.
 */
PJ_DECL(pj_status_t) pjsip_create_listener( pjsip_transport_mgr *mgr,
					    pjsip_transport_type_e type,
					    pj_sockaddr_in *local_addr,
					    const pj_sockaddr_in *addr_name);


/**
 * Create UDP listener.
 *
 * @param mgr		The transport manager.
 * @param sock		The UDP socket.
 * @param addr_name	If not null, sets the address name. If NULL, 
 *			then the local address will be used.
 *
 * @return		PJ_OK if listener was successfully created.
 */
PJ_DECL(pj_status_t) pjsip_create_udp_listener( pjsip_transport_mgr *mgr,
						pj_sock_t sock,
						const pj_sockaddr_in *addr_name);

/** 
 * Type of function to receive asynchronous transport completion for
 * pjsip_transport_get() operation.
 *
 * @param tr		The transport.
 * @param token		Token registered previously.
 * @param status	Status of operation.
 */
typedef void pjsip_transport_completion_callback(pjsip_transport_t *tr, 
						 void *token, 
						 pj_status_t status);

/**
 * Find transport to be used to send message to remote destination. If no
 * suitable transport is found, a new one will be created. If transport
 * can not be available immediately (for example, an outgoing TCP connec()),
 * then the caller will be notified later via the callback.
 *
 * @param mgr		The transport manager.
 * @param pool		Pool to allocate asychronous job (if required).
 * @param type		The transport type.
 * @param remote	The remote address.
 * @param token		The token that will be passed to the callback.
 * @param cb		The callback to be called to report the completion of 
 *			the operation.
 */
PJ_DECL(void) pjsip_transport_get( pjsip_transport_mgr *mgr,
				   pj_pool_t *pool,
				   pjsip_transport_type_e type,
				   const pj_sockaddr_in *remote,
				   void *token,
				   pjsip_transport_completion_callback *cb);

PJ_END_DECL

#endif	/* __PJSIP_SIP_TRANSPORT_H__ */
