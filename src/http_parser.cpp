
#include "http_parser.h"
#include "http_request.h"

int rd_parse_request_line(rd_http_request* r)
{
    enum {
        rd_start = 0,
        rd_method,
        rd_spaces_before_uri,
        rd_after_parse_slash,
        rd_http,
        rd_http_H,
        rd_http_HT,
        rd_http_HTT,
        rd_http_HTTP,
        rd_major_digits,
        rd_digits_or_dot,
        rd_minor_digits,
        rd_parse_end,
        rd_almost_done,
        rd_after_digits
    };

    char ch;
    while(r->parse_byte > 0) {
        ch = *(r->parse_ptr);

        switch (r->get_state()) {

            case rd_start:
                if(ch == CR || ch == LF) 
                    break;
                if((ch < 'A' || ch > 'Z') && ch != '_') {    //??研究研究为啥是'_'
                    return RD_INVALID_REQUEST;
                }
                r->method_start = r->parse_ptr;
                r->set_state(rd_method);
                break;

            case rd_method:
                if(ch == ' ') {

                    if(str3cmp(r->method_start, 'G', 'E', 'T', '\0')) {
                        r->set_method(RD_HTTP_GET);
                    }
                    else if(str3cmp(r->method_start, 'P', 'O', 'S', 'T')) {
                        r->set_method(RD_HTTP_POST);
                    }
                    else if(str3cmp(r->method_start, 'H', 'E', 'A', 'D')) {
                        r->set_method(RD_HTTP_HEAD);
                    }
                    else {
                        r->set_method(RD_HTTP_UNKNOWN);
                        return RD_INVALID_REQUEST;
                    }

                    r->set_state(rd_spaces_before_uri);
                    break;
                }
                
                if((ch < 'A' || ch > 'Z') && ch != '_') 
                    return RD_INVALID_REQUEST;
                break;

            case rd_spaces_before_uri:
                if(ch == '/'){
                    r->set_state(rd_after_parse_slash);
                    r->uri_start = r->parse_ptr;
                    break;
                }
                switch(ch) {
                    case ' ':
                        break;
                    default:
                        return RD_INVALID_REQUEST;
                }
                break;

            case rd_after_parse_slash:
                switch(ch) {
                    case ' ':
                        r->uri_end = r->parse_ptr;
                        r->set_state(rd_http);
                        break;
                    default:
                        break;
                }
                break;
            
            case rd_http:
                switch(ch) {
                    case ' ':
                        break;
                    case 'H':
                        r->set_state(rd_http_H);
                        break;
                    default:
                        return RD_INVALID_REQUEST;
                }
                break;

            case rd_http_H:
                switch(ch) {
                    case 'T':
                        r->set_state(rd_http_HT);
                        break;
                    default:
                        return RD_INVALID_REQUEST;
                }
                break;

            case rd_http_HT:
                switch (ch) {
                    case 'T':
                        r->set_state(rd_http_HTT);
                        break;
                    default:
                        return RD_INVALID_REQUEST;
                }
                break;
            
            case rd_http_HTT:
                switch(ch) {
                    case 'P':
                        r->set_state(rd_http_HTTP);
                        break;
                    default:
                        return RD_INVALID_REQUEST;
                }
                break;

            case rd_http_HTTP:
                switch (ch) {
                    case '/':
                        r->set_state(rd_major_digits);
                        break;
                    default:
                        return RD_INVALID_REQUEST;
                }
                break;

            case rd_major_digits:
                if(ch >= '0' && ch <= '9') {
                    r->set_state(rd_digits_or_dot);
                    break;
                }else {
                    return RD_INVALID_REQUEST;
                }

            case rd_digits_or_dot:
                if(ch >= '0' && ch <= '9') 
                    break;
                if(ch == '.') {
                    r->set_state(rd_minor_digits);
                    break;
                }
                return RD_INVALID_REQUEST;
            
            case rd_minor_digits:
                if(ch >= '0' && ch <= '9') {
                    r->set_state(rd_parse_end);
                    break;
                }
                return RD_INVALID_REQUEST;
            
            case rd_parse_end:
                if(ch == CR) {
                    r->set_state(rd_almost_done);
                    break;
                }
                if(ch == LF) {
                    goto done;
                }
                if(ch == ' ') {
                    r->set_state(rd_after_digits);
                    break;
                }
                if(ch >= '0' && ch <='9') 
                    break;
                return RD_INVALID_REQUEST;
            
            case rd_after_digits:
                switch(ch) {
                    case ' ':
                        break;
                    case CR:
                        r->set_state(rd_almost_done);
                        break;
                    case LF:
                        goto done;
                        break;
                    default:    
                        return RD_INVALID_REQUEST;
                }
                break;

            case rd_almost_done:
                switch(ch) {
                    case LF:
                        goto done;
                        break;
                    default:
                        return RD_INVALID_REQUEST;
                }
            
            default:
                return RD_INVALID_REQUEST;
        }   /* switch */

        r->parse_ptr++;
        r->parse_byte--;
    }   /* while */

    return RD_AGAIN;

done:
    r->set_state(rd_start);
    
    return RD_OK;
}

int rd_parse_request_body(rd_http_request* r) 
{
    enum {
        rd_start = 0,
        rd_key_start,
        rd_spaces_before_colon,
        rd_spaces_after_colon,
        rd_value,
        rd_cr,
        rd_crlf,
        rd_crlfcr,

    };

    int n;
    while(r->parse_byte > 0) {
        char ch = *(r->parse_ptr);

        switch (r->get_state()) {
            case rd_start:
                if(ch == CR || ch == LF || ch == ' ') {
                    break;
                }

                r->cur_key_start = r->parse_ptr;
                r->set_state(rd_key_start);
                break;

            case rd_key_start:
                if(ch == ' ') {
                    r->cur_key_end = r->parse_ptr;
                    r->set_state(rd_spaces_before_colon);
                    break;
                }
                if(ch == ':') {
                    r->cur_key_end = r->parse_ptr;
                    r->set_state(rd_spaces_after_colon);
                    break;
                }
                break;
            
            case rd_spaces_before_colon:
                switch(ch) {
                    case ' ':
                        break;
                    case ':':
                        r->set_state(rd_spaces_after_colon);
                        break;
                    default:
                        return RD_INVALID_REQUEST;
                }
                break;

            case rd_spaces_after_colon:
                if(ch == ' ')
                    break;
                
                r->cur_value_start = r->parse_ptr;
                r->set_state(rd_value);
                break;
            
            case rd_value:
                if(ch == CR) {
                    r->cur_key_end = r->parse_ptr;
                    r->set_state(rd_cr);
                }
                if(ch == LF) {
                    r->cur_key_end = r->parse_ptr;
                    r->set_state(rd_crlf);
                }
                break;

            case rd_cr:
                if(ch == LF) {
                    r->set_state(rd_crlf);
                    rd_http_header* new_header = 
                                new rd_http_header(r->cur_key_start
                                                , r->cur_key_end
                                                , r->cur_value_start
                                                , r->cur_value_end
                                                );
                    add_list(r, new_header);  
                    break;
                }
                else {
                    return RD_INVALID_REQUEST;
                }
            
            case rd_crlf:
                if(ch == CR) {
                    r->set_state(rd_crlfcr);
                    break;
                }
                if(ch == ' ') 
                    break;
                if(ch != LF) {
                    r->cur_key_start = r->parse_ptr;
                    r->set_state(rd_key_start);
                    break;
                }
                return RD_INVALID_REQUEST;

            case rd_crlfcr:
                if(ch == LF)
                    goto done;
                return RD_INVALID_REQUEST;
        }

        r->parse_byte--;
        r->parse_ptr++;
    }

    return RD_AGAIN;

done:
    r->set_state(rd_start);
    
    return RD_OK;
}
