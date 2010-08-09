/* $Id$ */
/* 
 * Copyright (C) 2003-2006 Benny Prijono <benny@prijono.org>
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
#include <pjsip-simple/presence.h>
#include <pjsip-simple/errno.h>
#include <pjsip-simple/evsub_msg.h>
#include <pjsip/sip_module.h>
#include <pjsip/sip_endpoint.h>
#include <pjsip/sip_dialog.h>
#include <pj/assert.h>
#include <pj/guid.h>
#include <pj/log.h>
#include <pj/os.h>
#include <pj/pool.h>
#include <pj/string.h>


#define THIS_FILE		    "presence.c"
#define PRES_DEFAULT_EXPIRES	    600

/*
 * Presence module (mod-presence)
 */
static struct pjsip_module mod_presence = 
{
    NULL, NULL,			    /* prev, next.			*/
    { "mod-presence", 12 },	    /* Name.				*/
    -1,				    /* Id				*/
    PJSIP_MOD_PRIORITY_APPLICATION-1,	/* Priority			*/
    NULL,			    /* User data.			*/
    NULL,			    /* load()				*/
    NULL,			    /* start()				*/
    NULL,			    /* stop()				*/
    NULL,			    /* unload()				*/
    NULL,			    /* on_rx_request()			*/
    NULL,			    /* on_rx_response()			*/
    NULL,			    /* on_tx_request.			*/
    NULL,			    /* on_tx_response()			*/
    NULL,			    /* on_tsx_state()			*/
};


/*
 * Presence message body type.
 */
typedef enum content_type
{
    CONTENT_TYPE_NONE,
    CONTENT_TYPE_PIDF,
    CONTENT_TYPE_XPIDF,
} content_type;

/*
 * This structure describe a presentity, for both subscriber and notifier.
 */
struct pjsip_pres
{
    pjsip_evsub		*sub;		/**< Event subscribtion record.	    */
    pjsip_dialog	*dlg;		/**< The dialog.		    */
    content_type	 content_type;	/**< Content-Type.		    */
    pjsip_pres_status	 status;	/**< Presence status.		    */
    pjsip_pres_status	 tmp_status;	/**< Temp, before NOTIFY is answred.*/
    pjsip_evsub_user	 user_cb;	/**< The user callback.		    */
};


typedef struct pjsip_pres pjsip_pres;


/*
 * Forward decl for evsub callback.
 */
static void pres_on_evsub_state( pjsip_evsub *sub, pjsip_event *event);
static void pres_on_evsub_tsx_state( pjsip_evsub *sub, pjsip_transaction *tsx,
				     pjsip_event *event);
static void pres_on_evsub_rx_refresh( pjsip_evsub *sub, 
				      pjsip_rx_data *rdata,
				      int *p_st_code,
				      pj_str_t **p_st_text,
				      pjsip_hdr *res_hdr,
				      pjsip_msg_body **p_body);
static void pres_on_evsub_rx_notify( pjsip_evsub *sub, 
				     pjsip_rx_data *rdata,
				     int *p_st_code,
				     pj_str_t **p_st_text,
				     pjsip_hdr *res_hdr,
				     pjsip_msg_body **p_body);
static void pres_on_evsub_client_refresh(pjsip_evsub *sub);
static void pres_on_evsub_server_timeout(pjsip_evsub *sub);


/*
 * Event subscription callback for presence.
 */
static pjsip_evsub_user pres_user = 
{
    &pres_on_evsub_state,
    &pres_on_evsub_tsx_state,
    &pres_on_evsub_rx_refresh,
    &pres_on_evsub_rx_notify,
    &pres_on_evsub_client_refresh,
    &pres_on_evsub_server_timeout,
};


/*
 * Some static constants.
 */
const pj_str_t STR_EVENT	    = { "Event", 5 };
const pj_str_t STR_PRESENCE	    = { "presence", 8 };
const pj_str_t STR_APPLICATION	    = { "application", 11 };
const pj_str_t STR_PIDF_XML	    = { "pidf+xml", 8};
const pj_str_t STR_XPIDF_XML	    = { "xpidf+xml", 9};
const pj_str_t STR_APP_PIDF_XML	    = { "application/pidf+xml", 20 };
const pj_str_t STR_APP_XPIDF_XML    = { "application/xpidf+xml", 21 };


/*
 * Init presence module.
 */
PJ_DEF(pj_status_t) pjsip_pres_init_module( pjsip_endpoint *endpt,
					    pjsip_module *mod_evsub)
{
    pj_status_t status;
    pj_str_t accept[2];

    /* Check arguments. */
    PJ_ASSERT_RETURN(endpt && mod_evsub, PJ_EINVAL);

    /* Must have not been registered */
    PJ_ASSERT_RETURN(mod_presence.id == -1, PJ_EINVALIDOP);

    /* Register to endpoint */
    status = pjsip_endpt_register_module(endpt, &mod_presence);
    if (status != PJ_SUCCESS)
	return status;

    accept[0] = STR_APP_PIDF_XML;
    accept[1] = STR_APP_XPIDF_XML;

    /* Register event package to event module. */
    status = pjsip_evsub_register_pkg( &mod_presence, &STR_PRESENCE, 
				       PRES_DEFAULT_EXPIRES, 
				       PJ_ARRAY_SIZE(accept), accept);
    if (status != PJ_SUCCESS) {
	pjsip_endpt_unregister_module(endpt, &mod_presence);
	return status;
    }

    return PJ_SUCCESS;
}


/*
 * Get presence module instance.
 */
PJ_DEF(pjsip_module*) pjsip_pres_instance(void)
{
    return &mod_presence;
}


/*
 * Create client subscription.
 */
PJ_DEF(pj_status_t) pjsip_pres_create_uac( pjsip_dialog *dlg,
					   const pjsip_evsub_user *user_cb,
					   pjsip_evsub **p_evsub )
{
    pj_status_t status;
    pjsip_pres *pres;
    pjsip_evsub *sub;

    PJ_ASSERT_RETURN(dlg && p_evsub, PJ_EINVAL);

    pjsip_dlg_inc_lock(dlg);

    /* Create event subscription */
    status = pjsip_evsub_create_uac( dlg,  &pres_user, &STR_PRESENCE, &sub);
    if (status != PJ_SUCCESS)
	goto on_return;

    /* Create presence */
    pres = pj_pool_zalloc(dlg->pool, sizeof(pjsip_pres));
    pres->dlg = dlg;
    if (user_cb)
	pj_memcpy(&pres->user_cb, user_cb, sizeof(pjsip_evsub_user));

    /* Attach to evsub */
    pjsip_evsub_set_mod_data(sub, mod_presence.id, pres);

    *p_evsub = sub;

on_return:
    pjsip_dlg_dec_lock(dlg);
    return status;
}


/*
 * Create server subscription.
 */
PJ_DEF(pj_status_t) pjsip_pres_create_uas( pjsip_dialog *dlg,
					   const pjsip_evsub_user *user_cb,
					   pjsip_rx_data *rdata,
					   pjsip_evsub **p_evsub )
{
    pjsip_accept_hdr *accept;
    pjsip_event_hdr *event;
    pjsip_expires_hdr *expires_hdr;
    unsigned expires;
    content_type content_type = CONTENT_TYPE_NONE;
    pjsip_evsub *sub;
    pjsip_pres *pres;
    pj_status_t status;

    /* Check arguments */
    PJ_ASSERT_RETURN(dlg && rdata && p_evsub, PJ_EINVAL);

    /* Must be request message */
    PJ_ASSERT_RETURN(rdata->msg_info.msg->type == PJSIP_REQUEST_MSG,
		     PJSIP_ENOTREQUESTMSG);

    /* Check that request is SUBSCRIBE */
    PJ_ASSERT_RETURN(pjsip_method_cmp(&rdata->msg_info.msg->line.req.method,
				      &pjsip_subscribe_method)==0,
		     PJSIP_SIMPLE_ENOTSUBSCRIBE);

    /* Check that Event header contains "presence" */
    event = pjsip_msg_find_hdr_by_name(rdata->msg_info.msg, &STR_EVENT, NULL);
    if (!event) {
	return PJSIP_ERRNO_FROM_SIP_STATUS(PJSIP_SC_BAD_REQUEST);
    }
    if (pj_stricmp(&event->event_type, &STR_PRESENCE) != 0) {
	return PJSIP_ERRNO_FROM_SIP_STATUS(PJSIP_SC_BAD_EVENT);
    }

    /* Check that request contains compatible Accept header. */
    accept = pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_ACCEPT, NULL);
    if (accept) {
	unsigned i;
	for (i=0; i<accept->count; ++i) {
	    if (pj_stricmp(&accept->values[i], &STR_APP_PIDF_XML)==0) {
		content_type = CONTENT_TYPE_PIDF;
		break;
	    } else
	    if (pj_stricmp(&accept->values[i], &STR_APP_XPIDF_XML)==0) {
		content_type = CONTENT_TYPE_XPIDF;
		break;
	    }
	}

	if (i==accept->count) {
	    /* Nothing is acceptable */
	    return PJSIP_ERRNO_FROM_SIP_STATUS(PJSIP_SC_NOT_ACCEPTABLE);
	}

    } else {
	/* No Accept header.
	 * Treat as "application/pidf+xml"
	 */
	content_type = CONTENT_TYPE_PIDF;
    }

    /* Check that expires is not too short. */
    expires_hdr=pjsip_msg_find_hdr(rdata->msg_info.msg, PJSIP_H_EXPIRES, NULL);
    if (expires_hdr) {
	if (expires_hdr->ivalue < 5) {
	    return PJSIP_ERRNO_FROM_SIP_STATUS(PJSIP_SC_INTERVAL_TOO_BRIEF);
	}

	expires = expires_hdr->ivalue;
	if (expires > PRES_DEFAULT_EXPIRES)
	    expires = PRES_DEFAULT_EXPIRES;

    } else {
	expires = PRES_DEFAULT_EXPIRES;
    }
    
    /* Lock dialog */
    pjsip_dlg_inc_lock(dlg);


    /* Create server subscription */
    status = pjsip_evsub_create_uas( dlg, &pres_user, rdata, &sub);
    if (status != PJ_SUCCESS)
	goto on_return;

    /* Create server presence subscription */
    pres = pj_pool_zalloc(dlg->pool, sizeof(pjsip_pres));
    pres->dlg = dlg;
    pres->sub = sub;
    pres->content_type = content_type;
    if (user_cb)
	pj_memcpy(&pres->user_cb, user_cb, sizeof(pjsip_evsub_user));

    /* Attach to evsub */
    pjsip_evsub_set_mod_data(sub, mod_presence.id, pres);

    /* Done: */
    *p_evsub = sub;

on_return:
    pjsip_dlg_dec_lock(dlg);
    return status;
}


/*
 * Create SUBSCRIBE
 */
PJ_DEF(pj_status_t) pjsip_pres_initiate( pjsip_evsub *sub,
					 pj_int32_t expires,
					 pjsip_tx_data **p_tdata)
{
    return pjsip_evsub_initiate(sub, &pjsip_subscribe_method, expires, 
				p_tdata);
}


/*
 * Accept incoming subscription.
 */
PJ_DEF(pj_status_t) pjsip_pres_accept( pjsip_evsub *sub,
				       pjsip_rx_data *rdata,
				       int st_code,
				       const pjsip_hdr *hdr_list )
{
    return pjsip_evsub_accept( sub, rdata, st_code, hdr_list );
}


/*
 * Get presence status.
 */
PJ_DEF(pj_status_t) pjsip_pres_get_status( pjsip_evsub *sub,
					   pjsip_pres_status *status )
{
    pjsip_pres *pres;

    PJ_ASSERT_RETURN(sub && status, PJ_EINVAL);

    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_RETURN(pres!=NULL, PJSIP_SIMPLE_ENOPRESENCE);

    if (pres->tmp_status._is_valid)
	pj_memcpy(status, &pres->tmp_status, sizeof(pjsip_pres_status));
    else
	pj_memcpy(status, &pres->status, sizeof(pjsip_pres_status));

    return PJ_SUCCESS;
}


/*
 * Set presence status.
 */
PJ_DEF(pj_status_t) pjsip_pres_set_status( pjsip_evsub *sub,
					   const pjsip_pres_status *status )
{
    unsigned i;
    pjsip_pres *pres;

    PJ_ASSERT_RETURN(sub && status, PJ_EINVAL);

    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_RETURN(pres!=NULL, PJSIP_SIMPLE_ENOPRESENCE);

    for (i=0; i<status->info_cnt; ++i) {
	pres->status.info[i].basic_open = status->info[i].basic_open;
	if (status->info[i].id.slen == 0) {
	    pj_create_unique_string(pres->dlg->pool, 
	    			    &pres->status.info[i].id);
	} else {
	    pj_strdup(pres->dlg->pool, 
		      &pres->status.info[i].id,
		      &status->info[i].id);
	}
	pj_strdup(pres->dlg->pool, 
		  &pres->status.info[i].contact,
		  &status->info[i].contact);
    }

    pres->status.info_cnt = status->info_cnt;

    return PJ_SUCCESS;
}


/*
 * Create PIDF document based on the presence info.
 */
static pjpidf_pres* pres_create_pidf( pj_pool_t *pool,
				      pjsip_pres *pres )
{
    pjpidf_pres *pidf;
    unsigned i;
    pj_str_t entity;

    /* Get publisher URI */
    entity.ptr = pj_pool_alloc(pool, PJSIP_MAX_URL_SIZE);
    entity.slen = pjsip_uri_print(PJSIP_URI_IN_REQ_URI,
				  pres->dlg->local.info->uri,
				  entity.ptr, PJSIP_MAX_URL_SIZE);
    if (entity.slen < 1)
	return NULL;

    /* Create <presence>. */
    pidf = pjpidf_create(pool, &entity);

    /* Create <tuple> */
    for (i=0; i<pres->status.info_cnt; ++i) {

	pjpidf_tuple *pidf_tuple;
	pjpidf_status *pidf_status;

	/* Add tuple id. */
	pidf_tuple = pjpidf_pres_add_tuple(pool, pidf, 
					   &pres->status.info[i].id);

	/* Set <contact> */
	if (pres->status.info[i].contact.slen)
	    pjpidf_tuple_set_contact(pool, pidf_tuple, 
				     &pres->status.info[i].contact);


	/* Set basic status */
	pidf_status = pjpidf_tuple_get_status(pidf_tuple);
	pjpidf_status_set_basic_open(pidf_status, 
				     pres->status.info[i].basic_open);
    }

    return pidf;
}


/*
 * Create XPIDF document based on the presence info.
 */
static pjxpidf_pres* pres_create_xpidf( pj_pool_t *pool,
				        pjsip_pres *pres )
{
    /* Note: PJSIP implementation of XPIDF is not complete!
     */
    pjxpidf_pres *xpidf;
    pj_str_t publisher_uri;

    PJ_LOG(4,(THIS_FILE, "Warning: XPIDF format is not fully supported "
			 "by PJSIP"));

    publisher_uri.ptr = pj_pool_alloc(pool, PJSIP_MAX_URL_SIZE);
    publisher_uri.slen = pjsip_uri_print(PJSIP_URI_IN_REQ_URI,
					 pres->dlg->local.info->uri,
					 publisher_uri.ptr,
					 PJSIP_MAX_URL_SIZE);
    if (publisher_uri.slen < 1)
	return NULL;

    /* Create XPIDF document. */
    xpidf = pjxpidf_create(pool, &publisher_uri);

    /* Set basic status. */
    if (pres->status.info_cnt > 0)
	pjxpidf_set_status( xpidf, pres->status.info[0].basic_open);
    else
	pjxpidf_set_status( xpidf, PJ_FALSE);

    return xpidf;
}


/*
 * Function to print XML message body.
 */
static int pres_print_body(struct pjsip_msg_body *msg_body, 
			   char *buf, pj_size_t size)
{
    return pj_xml_print(msg_body->data, buf, size, PJ_TRUE);
}


/*
 * Function to clone XML document.
 */
static void* xml_clone_data(pj_pool_t *pool, const void *data, unsigned len)
{
    PJ_UNUSED_ARG(len);
    return pj_xml_clone( pool, data);
}


/*
 * Create message body.
 */
static pj_status_t pres_create_msg_body( pjsip_pres *pres, 
					 pjsip_tx_data *tdata)
{
    pjsip_msg_body *body;

    body = pj_pool_zalloc(tdata->pool, sizeof(pjsip_msg_body));
    
    if (pres->content_type == CONTENT_TYPE_PIDF) {

	body->data = pres_create_pidf(tdata->pool, pres);
	body->content_type.type = pj_str("application");
	body->content_type.subtype = pj_str("pidf+xml");

    } else if (pres->content_type == CONTENT_TYPE_XPIDF) {

	body->data = pres_create_xpidf(tdata->pool, pres);
	body->content_type.type = pj_str("application");
	body->content_type.subtype = pj_str("xpidf+xml");

    } else {
	return PJSIP_SIMPLE_EBADCONTENT;
    }


    body->print_body = &pres_print_body;
    body->clone_data = &xml_clone_data;

    tdata->msg->body = body;

    return PJ_SUCCESS;
}


/*
 * Create NOTIFY
 */
PJ_DEF(pj_status_t) pjsip_pres_notify( pjsip_evsub *sub,
				       pjsip_evsub_state state,
				       const pj_str_t *state_str,
				       const pj_str_t *reason,
				       pjsip_tx_data **p_tdata)
{
    pjsip_pres *pres;
    pjsip_tx_data *tdata;
    pj_status_t status;
    
    /* Check arguments. */
    PJ_ASSERT_RETURN(sub, PJ_EINVAL);

    /* Get the presence object. */
    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_RETURN(pres != NULL, PJSIP_SIMPLE_ENOPRESENCE);

    /* Must have at least one presence info. */
    PJ_ASSERT_RETURN(pres->status.info_cnt > 0, PJSIP_SIMPLE_ENOPRESENCEINFO);


    /* Lock object. */
    pjsip_dlg_inc_lock(pres->dlg);

    /* Create the NOTIFY request. */
    status = pjsip_evsub_notify( sub, state, state_str, reason, &tdata);
    if (status != PJ_SUCCESS)
	goto on_return;


    /* Create message body to reflect the presence status. */
    status = pres_create_msg_body( pres, tdata );
    if (status != PJ_SUCCESS)
	goto on_return;


    /* Done. */
    *p_tdata = tdata;


on_return:
    pjsip_dlg_dec_lock(pres->dlg);
    return status;
}


/*
 * Create NOTIFY that reflect current state.
 */
PJ_DEF(pj_status_t) pjsip_pres_current_notify( pjsip_evsub *sub,
					       pjsip_tx_data **p_tdata )
{
    pjsip_pres *pres;
    pjsip_tx_data *tdata;
    pj_status_t status;
    
    /* Check arguments. */
    PJ_ASSERT_RETURN(sub, PJ_EINVAL);

    /* Get the presence object. */
    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_RETURN(pres != NULL, PJSIP_SIMPLE_ENOPRESENCE);

    /* Must have at least one presence info. */
    PJ_ASSERT_RETURN(pres->status.info_cnt > 0, PJSIP_SIMPLE_ENOPRESENCEINFO);


    /* Lock object. */
    pjsip_dlg_inc_lock(pres->dlg);

    /* Create the NOTIFY request. */
    status = pjsip_evsub_current_notify( sub, &tdata);
    if (status != PJ_SUCCESS)
	goto on_return;


    /* Create message body to reflect the presence status. */
    status = pres_create_msg_body( pres, tdata );
    if (status != PJ_SUCCESS)
	goto on_return;


    /* Done. */
    *p_tdata = tdata;


on_return:
    pjsip_dlg_dec_lock(pres->dlg);
    return status;
}


/*
 * Send request.
 */
PJ_DEF(pj_status_t) pjsip_pres_send_request( pjsip_evsub *sub,
					     pjsip_tx_data *tdata )
{
    return pjsip_evsub_send_request(sub, tdata);
}


/*
 * This callback is called by event subscription when subscription
 * state has changed.
 */
static void pres_on_evsub_state( pjsip_evsub *sub, pjsip_event *event)
{
    pjsip_pres *pres;

    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_ON_FAIL(pres!=NULL, {return;});

    if (pres->user_cb.on_evsub_state)
	(*pres->user_cb.on_evsub_state)(sub, event);
}

/*
 * Called when transaction state has changed.
 */
static void pres_on_evsub_tsx_state( pjsip_evsub *sub, pjsip_transaction *tsx,
				     pjsip_event *event)
{
    pjsip_pres *pres;

    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_ON_FAIL(pres!=NULL, {return;});

    if (pres->user_cb.on_tsx_state)
	(*pres->user_cb.on_tsx_state)(sub, tsx, event);
}


/*
 * Called when SUBSCRIBE is received.
 */
static void pres_on_evsub_rx_refresh( pjsip_evsub *sub, 
				      pjsip_rx_data *rdata,
				      int *p_st_code,
				      pj_str_t **p_st_text,
				      pjsip_hdr *res_hdr,
				      pjsip_msg_body **p_body)
{
    pjsip_pres *pres;

    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_ON_FAIL(pres!=NULL, {return;});

    if (pres->user_cb.on_rx_refresh) {
	(*pres->user_cb.on_rx_refresh)(sub, rdata, p_st_code, p_st_text,
				       res_hdr, p_body);

    } else {
	/* Implementors MUST send NOTIFY if it implements on_rx_refresh */
	pjsip_tx_data *tdata;
	pj_str_t timeout = { "timeout", 7};
	pj_status_t status;

	if (pjsip_evsub_get_state(sub)==PJSIP_EVSUB_STATE_TERMINATED) {
	    status = pjsip_pres_notify( sub, PJSIP_EVSUB_STATE_TERMINATED,
					NULL, &timeout, &tdata);
	} else {
	    status = pjsip_pres_current_notify(sub, &tdata);
	}

	if (status == PJ_SUCCESS)
	    pjsip_pres_send_request(sub, tdata);
    }
}

/*
 * Parse PIDF to info.
 */
static pj_status_t pres_parse_pidf( pjsip_pres *pres,
				    pjsip_rx_data *rdata,
				    pjsip_pres_status *pres_status)
{
    pjpidf_pres *pidf;
    pjpidf_tuple *pidf_tuple;

    pidf = pjpidf_parse(rdata->tp_info.pool, 
			rdata->msg_info.msg->body->data,
			rdata->msg_info.msg->body->len);
    if (pidf == NULL)
	return PJSIP_SIMPLE_EBADPIDF;

    pres_status->info_cnt = 0;

    pidf_tuple = pjpidf_pres_get_first_tuple(pidf);
    while (pidf_tuple) {
	pjpidf_status *pidf_status;

	pj_strdup(pres->dlg->pool, 
		  &pres_status->info[pres_status->info_cnt].id,
		  pjpidf_tuple_get_id(pidf_tuple));

	pj_strdup(pres->dlg->pool, 
		  &pres_status->info[pres_status->info_cnt].contact,
		  pjpidf_tuple_get_contact(pidf_tuple));

	pidf_status = pjpidf_tuple_get_status(pidf_tuple);
	if (pidf_status) {
	    pres_status->info[pres_status->info_cnt].basic_open = 
		pjpidf_status_is_basic_open(pidf_status);
	} else {
	    pres_status->info[pres_status->info_cnt].basic_open = PJ_FALSE;
	}

	pidf_tuple = pjpidf_pres_get_next_tuple( pidf, pidf_tuple );
	pres_status->info_cnt++;
    }

    return PJ_SUCCESS;
}

/*
 * Parse XPIDF info.
 */
static pj_status_t pres_parse_xpidf( pjsip_pres *pres,
				     pjsip_rx_data *rdata,
				     pjsip_pres_status *pres_status)
{
    pjxpidf_pres *xpidf;

    xpidf = pjxpidf_parse(rdata->tp_info.pool, 
			  rdata->msg_info.msg->body->data,
			  rdata->msg_info.msg->body->len);
    if (xpidf == NULL)
	return PJSIP_SIMPLE_EBADXPIDF;

    pres_status->info_cnt = 1;
    
    pj_strdup(pres->dlg->pool,
	      &pres_status->info[0].contact,
	      pjxpidf_get_uri(xpidf));
    pres_status->info[0].basic_open = pjxpidf_get_status(xpidf);
    pres_status->info[0].id.slen = 0;

    return PJ_SUCCESS;
}


/*
 * Called when NOTIFY is received.
 */
static void pres_on_evsub_rx_notify( pjsip_evsub *sub, 
				     pjsip_rx_data *rdata,
				     int *p_st_code,
				     pj_str_t **p_st_text,
				     pjsip_hdr *res_hdr,
				     pjsip_msg_body **p_body)
{
    pjsip_ctype_hdr *ctype_hdr;
    pjsip_pres *pres;
    pj_status_t status;

    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_ON_FAIL(pres!=NULL, {return;});

    /* Check Content-Type and msg body are present. */
    ctype_hdr = rdata->msg_info.ctype;

    if (ctype_hdr==NULL || rdata->msg_info.msg->body==NULL) {
	
	pjsip_warning_hdr *warn_hdr;
	pj_str_t warn_text;

	*p_st_code = PJSIP_SC_BAD_REQUEST;

	warn_text = pj_str("Message body is not present");
	warn_hdr = pjsip_warning_hdr_create(rdata->tp_info.pool, 399,
					    pjsip_endpt_name(pres->dlg->endpt),
					    &warn_text);
	pj_list_push_back(res_hdr, warn_hdr);

	return;
    }

    /* Parse content. */

    if (pj_stricmp(&ctype_hdr->media.type, &STR_APPLICATION)==0 &&
	pj_stricmp(&ctype_hdr->media.subtype, &STR_PIDF_XML)==0)
    {
	status = pres_parse_pidf( pres, rdata, &pres->tmp_status);
    }
    else 
    if (pj_stricmp(&ctype_hdr->media.type, &STR_APPLICATION)==0 &&
	pj_stricmp(&ctype_hdr->media.subtype, &STR_XPIDF_XML)==0)
    {
	status = pres_parse_xpidf( pres, rdata, &pres->tmp_status);
    }
    else
    {
	status = PJSIP_SIMPLE_EBADCONTENT;
    }

    if (status != PJ_SUCCESS) {
	/* Unsupported or bad Content-Type */
	pjsip_accept_hdr *accept_hdr;
	pjsip_warning_hdr *warn_hdr;

	*p_st_code = PJSIP_SC_NOT_ACCEPTABLE_HERE;

	/* Add Accept header */
	accept_hdr = pjsip_accept_hdr_create(rdata->tp_info.pool);
	accept_hdr->values[accept_hdr->count++] = STR_APP_PIDF_XML;
	accept_hdr->values[accept_hdr->count++] = STR_APP_XPIDF_XML;
	pj_list_push_back(res_hdr, accept_hdr);

	/* Add Warning header */
	warn_hdr = pjsip_warning_hdr_create_from_status(
				    rdata->tp_info.pool,
				    pjsip_endpt_name(pres->dlg->endpt),
				    status);
	pj_list_push_back(res_hdr, warn_hdr);

	return;
    }

    /* If application calls pres_get_status(), redirect the call to
     * retrieve the temporary status.
     */
    pres->tmp_status._is_valid = PJ_TRUE;

    /* Notify application. */
    if (pres->user_cb.on_rx_notify) {
	(*pres->user_cb.on_rx_notify)(sub, rdata, p_st_code, p_st_text, 
				      res_hdr, p_body);
    }

    
    /* If application responded NOTIFY with 2xx, copy temporary status
     * to main status, and mark the temporary status as invalid.
     */
    if ((*p_st_code)/100 == 2) {
	pj_memcpy(&pres->status, &pres->tmp_status, sizeof(pjsip_pres_status));
    }

    pres->tmp_status._is_valid = PJ_FALSE;

    /* Done */
}

/*
 * Called when it's time to send SUBSCRIBE.
 */
static void pres_on_evsub_client_refresh(pjsip_evsub *sub)
{
    pjsip_pres *pres;

    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_ON_FAIL(pres!=NULL, {return;});

    if (pres->user_cb.on_client_refresh) {
	(*pres->user_cb.on_client_refresh)(sub);
    } else {
	pj_status_t status;
	pjsip_tx_data *tdata;

	status = pjsip_pres_initiate(sub, -1, &tdata);
	if (status == PJ_SUCCESS)
	    pjsip_pres_send_request(sub, tdata);
    }
}

/*
 * Called when no refresh is received after the interval.
 */
static void pres_on_evsub_server_timeout(pjsip_evsub *sub)
{
    pjsip_pres *pres;

    pres = pjsip_evsub_get_mod_data(sub, mod_presence.id);
    PJ_ASSERT_ON_FAIL(pres!=NULL, {return;});

    if (pres->user_cb.on_server_timeout) {
	(*pres->user_cb.on_server_timeout)(sub);
    } else {
	pj_status_t status;
	pjsip_tx_data *tdata;
	pj_str_t reason = { "timeout", 7 };

	status = pjsip_pres_notify(sub, PJSIP_EVSUB_STATE_TERMINATED,
				   NULL, &reason, &tdata);
	if (status == PJ_SUCCESS)
	    pjsip_pres_send_request(sub, tdata);
    }
}
