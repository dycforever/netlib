/* Base on code from:
 http://archives.seul.org/libevent/users/Sep-2010/msg00050.html
 */

// #include "MITLogModule.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/keyvalq_struct.h>

// (default)
#define HTTP_CONTENT_TYPE_URL_ENCODED   "application/x-www-form-urlencoded"   
// (use for files: picture, mp3, tar-file etc.)                                        
#define HTTP_CONTENT_TYPE_FORM_DATA     "multipart/form-data"                 
// (use for plain text)
#define HTTP_CONTENT_TYPE_TEXT_PLAIN    "text/plain"

#define REQUEST_POST_FLAG               2
#define REQUEST_GET_FLAG                3

struct http_request_get {
    struct evhttp_uri *uri;
    struct event_base *base;
    struct evhttp_connection *cn;
    struct evhttp_request *req;
};

struct http_request_post {
    struct evhttp_uri *uri;
    struct event_base *base;
    struct evhttp_connection *cn;
    struct evhttp_request *req;
    char *content_type;
    char *post_data;
};

/************************** Ahead Declare ******************************/
void http_requset_post_cb(struct evhttp_request *req, void *arg);
void http_requset_get_cb(struct evhttp_request *req, void *arg);
int start_url_request(struct http_request_get *http_req, int req_get_flag);

/************************** Tools Function ******************************/
static inline void print_request_head_info(struct evkeyvalq *header)
{
    struct evkeyval *first_node = header->tqh_first;
    while (first_node) {
        MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"key:%s  value:%s", first_node->key, first_node->value);
        first_node = first_node->next.tqe_next;
    }
}

static inline void print_uri_parts_info(const struct evhttp_uri * http_uri)
{
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"scheme:%s", evhttp_uri_get_scheme(http_uri));
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"host:%s", evhttp_uri_get_host(http_uri));
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"path:%s", evhttp_uri_get_path(http_uri));
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"port:%d", evhttp_uri_get_port(http_uri));
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"query:%s", evhttp_uri_get_query(http_uri));
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"userinfo:%s", evhttp_uri_get_userinfo(http_uri));
    MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"fragment:%s", evhttp_uri_get_fragment(http_uri));
}

/************************** Request Function ******************************/
void http_requset_post_cb(struct evhttp_request *req, void *arg)
{
    struct http_request_post *http_req_post = (struct http_request_post *)arg;
    switch(req->response_code)
    {
        case HTTP_OK:
        {
            struct evbuffer* buf = evhttp_request_get_input_buffer(req);
            size_t len = evbuffer_get_length(buf);
            MITLog_DetPuts(MITLOG_LEVEL_COMMON, "print the head info:");
            print_request_head_info(req->output_headers);
            
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"len:%zu  body size:%zu", len, req->body_size);       
            char *tmp = malloc(len+1);
            memcpy(tmp, evbuffer_pullup(buf, -1), len);
            tmp[len] = '\0';
            MITLog_DetPuts(MITLOG_LEVEL_COMMON, "print the body:");
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"HTML BODY:%s", tmp);
            free(tmp);
            
            event_base_loopexit(http_req_post->base, 0);
            break;
        }
        case HTTP_MOVEPERM:
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "%s", "the uri moved permanently");
            break;
        case HTTP_MOVETEMP:
        {
            const char *new_location = evhttp_find_header(req->input_headers, "Location");
            struct evhttp_uri *new_uri = evhttp_uri_parse(new_location);
            evhttp_uri_free(http_req_post->uri);
            http_req_post->uri = new_uri;
            start_url_request((struct http_request_get *)http_req_post, REQUEST_POST_FLAG);
            return;
        }
            
        default:
            event_base_loopexit(http_req_post->base, 0);
            return;
    }
}
void http_requset_get_cb(struct evhttp_request *req, void *arg)
{
    struct http_request_get *http_req_get = (struct http_request_get *)arg;
    switch(req->response_code)
    {
        case HTTP_OK:
        {
            struct evbuffer* buf = evhttp_request_get_input_buffer(req);
            size_t len = evbuffer_get_length(buf);
            MITLog_DetPuts(MITLOG_LEVEL_COMMON, "print the head info:");
            print_request_head_info(req->output_headers);
            
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"len:%zu  body size:%zu", len, req->body_size);
            char *tmp = malloc(len+1);
            memcpy(tmp, evbuffer_pullup(buf, -1), len);
            tmp[len] = '\0';
            MITLog_DetPuts(MITLOG_LEVEL_COMMON, "print the body:");
            MITLog_DetPrintf(MITLOG_LEVEL_COMMON,"HTML BODY:%s", tmp);
            free(tmp);
            
            event_base_loopexit(http_req_get->base, 0);
            break;
        }
        case HTTP_MOVEPERM:
            MITLog_DetPrintf(MITLOG_LEVEL_ERROR, "%s", "the uri moved permanently");
            break;
        case HTTP_MOVETEMP:
        {
            const char *new_location = evhttp_find_header(req->input_headers, "Location");
            struct evhttp_uri *new_uri = evhttp_uri_parse(new_location);
            evhttp_uri_free(http_req_get->uri);
            http_req_get->uri = new_uri;
            start_url_request(http_req_get, REQUEST_GET_FLAG);
            return;
        }
            
        default:
            event_base_loopexit(http_req_get->base, 0);
            return;
    }
}

int start_url_request(struct http_request_get *http_req, int req_get_flag)
{
    if (http_req->cn)
        evhttp_connection_free(http_req->cn);
    
    int port = evhttp_uri_get_port(http_req->uri);
    http_req->cn = evhttp_connection_base_new(http_req->base,
                                                   NULL,
                                                   evhttp_uri_get_host(http_req->uri),
                                                   (port == -1 ? 80 : port));
    
    /**
     * Request will be released by evhttp connection
     * See info of evhttp_make_request()
     */
    if (req_get_flag == REQUEST_POST_FLAG) {
        http_req->req = evhttp_request_new(http_requset_post_cb, http_req);
    } else if (req_get_flag ==  REQUEST_GET_FLAG) {
        http_req->req = evhttp_request_new(http_requset_get_cb, http_req);
    }
    
    if (req_get_flag == REQUEST_POST_FLAG) {
        const char *path = evhttp_uri_get_path(http_req->uri);
        evhttp_make_request(http_req->cn, http_req->req, EVHTTP_REQ_POST,
                            path ? path : "/");
        /** Set the post data */
        struct http_request_post *http_req_post = (struct http_request_post *)http_req;
        evbuffer_add(http_req_post->req->output_buffer, http_req_post->post_data, strlen(http_req_post->post_data));
        evhttp_add_header(http_req_post->req->output_headers, "Content-Type", http_req_post->content_type);
    } else if (req_get_flag == REQUEST_GET_FLAG) {
        const char *query = evhttp_uri_get_query(http_req->uri);
        const char *path = evhttp_uri_get_path(http_req->uri);
        size_t len = (query ? strlen(query) : 0) + (path ? strlen(path) : 0) + 1;
        char *path_query = NULL;
        if (len > 1) {
            path_query = calloc(len, sizeof(char));
            sprintf(path_query, "%s?%s", path, query);
        }        
        evhttp_make_request(http_req->cn, http_req->req, EVHTTP_REQ_GET,
                             path_query ? path_query: "/");
    }
    /** Set the header properties */
    evhttp_add_header(http_req->req->output_headers, "Host", evhttp_uri_get_host(http_req->uri));
    
    return 0;
}

/************************** New/Free Function ******************************/
/**
 * @param get_flag: refer REQUEST_GET_*
 *
 */
void *http_request_new(struct event_base* base, const char *url, int req_get_flag, \
                       const char *content_type, const char* data)
{
    int len = 0;
    if (req_get_flag == REQUEST_GET_FLAG) {
        len = sizeof(struct http_request_get);
    } else if(req_get_flag == REQUEST_POST_FLAG) {
        len = sizeof(struct http_request_post);
    }
    
    struct http_request_get *http_req_get = calloc(1, len);
    http_req_get->uri = evhttp_uri_parse(url);
    print_uri_parts_info(http_req_get->uri);
    
    http_req_get->base = base;
    
    if (req_get_flag == REQUEST_POST_FLAG) {
        struct http_request_post *http_req_post = (struct http_request_post *)http_req_get;
        if (content_type == NULL) {
            content_type = HTTP_CONTENT_TYPE_URL_ENCODED;
        }
        http_req_post->content_type = strdup(content_type);
        
        if (data == NULL) {
            http_req_post->post_data = NULL;
        } else {
            http_req_post->post_data = strdup(data);
        }
    }
    
    return http_req_get;
}

void http_request_free(struct http_request_get *http_req_get, int req_get_flag)
{
    evhttp_connection_free(http_req_get->cn);
    evhttp_uri_free(http_req_get->uri);
    if (req_get_flag == REQUEST_GET_FLAG) {
        free(http_req_get);
    } else if(req_get_flag == REQUEST_POST_FLAG) {
        struct http_request_post *http_req_post = (struct http_request_post*)http_req_get;
        if (http_req_post->content_type) {
            free(http_req_post->content_type);
        }
        if (http_req_post->post_data) {
            free(http_req_post->post_data);
        }
        free(http_req_post);
    }
    http_req_get = NULL;
}

/************************** Start POST/GET Function ******************************/
/**
 * @param content_type: refer HTTP_CONTENT_TYPE_*       
 */
void *start_http_requset(struct event_base* base, const char *url, int req_get_flag, \
                                                const char *content_type, const char* data)
{
    struct http_request_get *http_req_get = http_request_new(base, url, req_get_flag, content_type, data);
    start_url_request(http_req_get, req_get_flag);
    
    return http_req_get;
}


int main(int argc, char *argv[])
{
    MITLogOpen("LibeventHttpClient");
    struct event_base* base = event_base_new();
    
    struct http_request_post *http_req_post = start_http_requset(base,
                                                                 "http://172.16.239.93:8899/base/truck/delete",
                                                                 REQUEST_POST_FLAG,
                                                                 HTTP_CONTENT_TYPE_URL_ENCODED,
                                                                 "name=winlin&code=1234");
    struct http_request_get *http_req_get = start_http_requset(base,
                                                               "http://127.0.0.1?name=winlin",
                                                               REQUEST_GET_FLAG,
                                                               NULL, NULL);
    
    event_base_dispatch(base);
    
    http_request_free((struct http_request_get *)http_req_post, REQUEST_POST_FLAG);
    http_request_free(http_req_get, REQUEST_GET_FLAG);
    event_base_free(base);
    MITLogClose();
    
    return 0;
}
