#include "message.h"

request* request_create(char *method, char *URI, char *body) {
    request_header *req_header = calloc(1, sizeof(request_header));
    req_header->method = calloc(HEADER_SPECS_LENGTH, sizeof(char));
    if (method != NULL) {
        strcpy(req_header->method, MIN(method, HEADER_SPECS_LENGTH-1));
    }

    req_header->URI = calloc(HEADER_URI_LENGTH, sizeof(char));
    if (URI != NULL) {
        strcpy(req_header->URI, MIN(URI, HEADER_URI_LENGTH-1));
    }

    req_header->protocol = calloc(HEADER_SPECS_LENGTH, sizeof(char));
    strcpy(req_header->protocol, "HTTP/1.1");

    req_header->num_fields = 10;
    req_header->fields = calloc(req_header->num_fields, sizeof(header_field));

    request *req = calloc(1, sizeof(request));
    req->header = req_header;

    if (body != NULL) {
        req->body = calloc(strlen(body) + 1, sizeof(char));
        strncpy(req->body, body, strlen(body));
    } else {
        req->body = calloc(BODY_INITIAL_SIZE, sizeof(char));
    }

    return req;
}

void request_free(request *req) {
    free(req->header->fields);
    free(req->header->protocol);
    free(req->header->method);
    free(req->header->URI);

    free(req->header);
    free(req->body);
    free(req);
}

response *response_create(int status_code, char *status_message, char *protocol, char *body) {
    response_header *res_header = calloc(1, sizeof(response_header));

    res_header->protocol = calloc(HEADER_SPECS_LENGTH, sizeof(char));
    if (protocol == NULL) {
        strcpy(res_header->protocol, "HTTP/1.1");
    } else {
        strcpy(res_header->protocol, protocol);
    }

    if (status_code != 0) {
        res_header->status_code = status_code;
    }

    res_header->status_message = calloc(HEADER_FIELD_VALUE_LENGTH, sizeof(char));
    if (status_message != NULL) {
        strcpy(res_header->status_message, status_message);
    }

    res_header->num_fields = 10;
    res_header->fields = calloc(res_header->num_fields, sizeof(header_field));

    request *res = calloc(1, sizeof(response));
    res->header = res_header;

    if (body != NULL) {
        res->body = calloc(strlen(body) + 1, sizeof(char));
        strncpy(res->body, body, strlen(body));
    } else {
        res->body = calloc(BODY_INITIAL_SIZE, sizeof(char));
    }

    return res;
}

int add_header_field(void *ptr, char *name, char *value) {
    request *http_msg = ptr;

    if (strlen(name) > HEADER_FIELD_NAME_LENGTH || strlen(value) > HEADER_FIELD_VALUE_LENGTH) {
        perror("Header field name or value too long.");
        return -1;
    }

    for (int i = 0; i < http_msg->header->num_fields; i++) {
        if (strlen(http_msg->header->fields[i].name) != 0) continue;

        strncpy(http_msg->header->fields[i].name, name, MIN(strlen(name), HEADER_FIELD_NAME_LENGTH-1));
        strncpy(http_msg->header->fields[i].value, value, MIN(strlen(value), HEADER_FIELD_VALUE_LENGTH-1));
        return 0;
    }

    // No empty field exists, so realloc ->fields and use a new one
    int i = http_msg->header->num_fields + 1;

    realloc(http_msg->header->fields, (http_msg->header->num_fields * 2) * sizeof(header_field));
    if (http_msg->header->fields == NULL) return -1;

    http_msg->header->num_fields *= 2; // increase num_fields if realloc was successful

    strncpy(http_msg->header->fields[i].name, name, MIN(strlen(name), HEADER_FIELD_NAME_LENGTH-1));
    strncpy(http_msg->header->fields[i].value, value, MIN(strlen(value), HEADER_FIELD_VALUE_LENGTH-1));
    return 0;
}

int has_header_field(void *ptr, char *name, int *field_index) {
    request *http_msg = ptr;

    for (int i = 0; i < http_msg->header->num_fields; i++) {
        if (strlen(http_msg->header->fields[i].name) == 0) continue;

        int cmp = strncmp(http_msg->header->fields[i].name, name, HEADER_FIELD_NAME_LENGTH);
        if (cmp != 0) continue;

        if (NULL != field_index) *field_index = i;
        return 1;
    }

    return 0;
}

int response_bytesize(response *res) {
    unsigned int size = 0;

    size += strlen(res->header->protocol) + 1; // +1 for space
    size += 3; // status code
    size += strlen(res->header->status_message) + 2; // +2 for \r\n

    char *body_bytesize_str = calloc(12, sizeof(char));
    if (strlen(res->body) > 0) {
        if (strlen(res->body) > 99999999999) {
            perror("Body too large.");
            return -1;
        }
    }

    snprintf(body_bytesize_str, 12, "%d", strlen(res->body));
    add_header_field(res, "Content-Length", body_bytesize_str);
    free(body_bytesize_str);

    for (int i = 0; i < res->header->num_fields; i++) {
        if (strlen(res->header->fields[i].name) == 0) continue;

        size += strlen(res->header->fields[i].name) + 1; // +1 for :
        size += strlen(res->header->fields[i].value) + 2; // +2 for \r\n
    }

    size += 2; // \r\n
    size += strlen(res->body);

    // TODO: Its missing exactly this amount of bytes (Says Valgrind). Why?
    size += 3;
    size += strlen(res->body);

    return size;
}

char* response_stringify(response *res) {
    int res_size = response_bytesize(res);

    if (res_size == 0) { // response is empty
        res_size = 1;
    } else if (res_size < 0) {
        perror("Could not stringify response.");
        return NULL;
    }

    char *res_str = calloc(res_size, sizeof(char));
    strcat(res_str, res->header->protocol);
    strcat(res_str, " ");

    char *status_code_str = calloc(4, sizeof(char));
    snprintf(status_code_str, 4, "%d", res->header->status_code);
    strcat(res_str, status_code_str);
    free(status_code_str);

    strcat(res_str, " ");

    strcat(res_str, res->header->status_message);
    strcat(res_str, "\r\n");

    for (int i = 0; i < res->header->num_fields; i++) {
        if (strlen(res->header->fields[i].name) == 0) continue;

        strcat(res_str, res->header->fields[i].name);
        strcat(res_str, ": ");
        strcat(res_str, res->header->fields[i].value);
        strcat(res_str, "\r\n");
    }

    strcat(res_str, "\r\n");

    if (strlen(res->body) > 0) {
        strcat(res_str, res->body);
    }

    return res_str;
}

void response_free(response *res) {
    free(res->header->fields);
    free(res->header->protocol);
    free(res->header->status_message);

    free(res->header);
    free(res->body);
    free(res);
}
