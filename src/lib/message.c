#include "message.h"

// TODO: Make this initializable
request* request_create() {
    // TODO: Abstract this
    request_header *req_header = calloc(1, sizeof(request_header));
    req_header->method = calloc(9, sizeof(char));
    req_header->URI = calloc(HEADER_URI_LENGTH, sizeof(char));
    req_header->protocol = calloc(9, sizeof(char));
    req_header->num_fields = 10; // TODO: Abstract this into def
    req_header->fields = calloc(req_header->num_fields, sizeof(header_field));

    request *req = calloc(1, sizeof(request));
    req->header = req_header;
    req->body = calloc(BODY_INITIAL_SIZE, sizeof(char));

    return req;
}

void request_free(request *req) {
    free(req->header->fields);
    free(req->header->protocol);
    free(req->header->URI);

    free(req->header);
    free(req->body);
    free(req);
}

response *response_create(int status_code, char *status_message, char *protocol, char *body) {

    response_header *res_header = calloc(1, sizeof(response_header));
    res_header->protocol = calloc(9, sizeof(char));
    if (protocol == NULL) {
        strcpy(res_header->protocol, "HTTP/1.0");
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

    res_header->num_fields = 10; // TODO: Abstract this into def
    res_header->fields = calloc(res_header->num_fields, sizeof(header_field));

    request *res = calloc(1, sizeof(response));
    res->header = res_header;

    if (body != NULL) {
        res->body = calloc(strlen(body) + 1, sizeof(char));
        strcpy(res->body, body);
    } else {
        res->body = calloc(BODY_INITIAL_SIZE, sizeof(char));
    }

    return res;
}

int response_add_header_field(response *res, char *name, char *value) {
    for (int i = 0; i < res->header->num_fields; i++) {
        if (strlen(res->header->fields[i].name) != 0) continue;

        // TODO: ensure len(name) <= len(fields[i].name)
        strncpy(res->header->fields[i].name, name, strlen(name));
        strncpy(res->header->fields[i].value, value, strlen(value));
        return 0;
    }

    // No empty field exists, so realloc ->fields and use a new one
    int i = res->header->num_fields + 1;

    res->header->fields = realloc(res->header->fields, (res->header->num_fields*2) * sizeof(header_field));
    if (res->header->fields == NULL) return -1;

    res->header->num_fields *= 2; // increase num_fields if realloc was successful

    strncpy(res->header->fields[i].name, name, strlen(name));
    strncpy(res->header->fields[i].value, value, strlen(value));
    return 0;
}

int response_bytesize(response *res) {
    int size = 0;

    size += strlen(res->header->protocol) + 1; // +1 for space
    size += 3; // status code
    size += strlen(res->header->status_message) + 2; // +2 for \r\n

    if (strlen(res->body) > 0) {
        // TODO: check for body size o_o (max 11 digits)
        char *body_bytesize_str = calloc(12, sizeof(char));
        snprintf(body_bytesize_str, 12, "%d", strlen(res->body));
        response_add_header_field(res, "Content-Length", body_bytesize_str);
    }

    for (int i = 0; i < res->header->num_fields; i++) {
        if (strlen(res->header->fields[i].name) == 0) continue;

        size += strlen(res->header->fields[i].name) + 1; // +1 for :
        size += strlen(res->header->fields[i].value) + 2; // +2 for \r\n
    }

    size += 2; // \r\n
    size += strlen(res->body);

    // TODO: Its missing exactly this amount of bytes (Says Valgrind). Why?
    size += 2; // \r\n
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
    strcat(res_str, res->body);

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
