
#include "read_write.h"

/* Write "n" bytes to a descriptor. */
ssize_t Writen(int fd, const void *vptr, size_t n) {
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return n;
}


void* encode_tlv(uint8_t type, uint16_t length, void *value, uint16_t *encoded_length) {
    *encoded_length = 1 + 2 + length; // Type (1 byte) + Length (2 bytes) + Value
    uint8_t *encoded = (uint8_t *)malloc(*encoded_length);

    encoded[0] = type;
    encoded[1] = (length >> 8) & 0xFF;
    encoded[2] = length & 0xFF;
    memcpy(encoded + 3, value, length);

    return encoded;
}

int send_tlv(int sockfd, uint8_t type, uint16_t length, void *value) {
    uint16_t encoded_length;
    void *encoded = encode_tlv(type, length, value, &encoded_length);
    
    if (Writen(sockfd, encoded, encoded_length) != encoded_length) {
        syslog (LOG_ERR, "Writen error in send_tlv");
        return 1;
        
    }

    free(encoded);
    return 0;
}



ssize_t Readn(int sockfd, void *buf, size_t n) {
    size_t nleft;
    ssize_t nread;
    char *ptr;

    ptr = buf;
    nleft = n;
    while (nleft > 0) {
        if ((nread = read(sockfd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      // Call read() again
            else
                return -1;      // Error
        } else if (nread == 0)
            break;              // EOF
        nleft -= nread;
        ptr += nread;
    }

    return (n - nleft);         // Return >= 0
}

void decode_tlv(void *encoded, TLV *tlv) {
    uint8_t *data = (uint8_t *)encoded;

    tlv->type = data[0];
    tlv->length = (data[1] << 8) | data[2];
    tlv->value = malloc(tlv->length);
    memcpy(tlv->value, data + 3, tlv->length);
}

void recv_tlv(int sockfd, TLV *tlv) {
    uint8_t header[3];
    if (Readn(sockfd, header, 3) != 3) {
        syslog (LOG_ERR, "Readn error in recv_tlv");
        return;
    }

    tlv->type = header[0];
    tlv->length = (header[1] << 8) | header[2];


    tlv->value = calloc(tlv->length, sizeof(char));
    if (Readn(sockfd, tlv->value, tlv->length) != tlv->length) {
        syslog (LOG_ERR, "Readn error in recv_tlv");
        free(tlv->value);
    }

}