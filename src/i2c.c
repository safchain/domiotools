#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include "string.h"

#include "mem.h"
#include "mqtt.h"
#include "logging.h"
#include "str.h"
#include "devices.h"
#include "homeasy.h"
#include "srts.h"

#define MAGIC1 0x56
#define MAGIC2 0x57

struct i2c_gw_request {
  unsigned char magic1;
  unsigned char magic2;
  unsigned short session;

  unsigned char type;
  unsigned short address1;
  unsigned short address2;
  unsigned short receiver;
  unsigned char ctrl;
  unsigned short code;
} __attribute__ ((__packed__));

struct dlog *DLOG;

int i2c_gw_open(char *filename, int addr)
{
  int fd, rc;

  fd = open(filename, O_RDWR);
  if (fd < 0) {
    return fd;
  }

  rc = ioctl( fd, I2C_SLAVE, addr);
  if (rc < 0) {
    return rc;
  }

  return fd;
}

int i2c_gw_send(int fd, struct i2c_gw_request *request)
{
  struct iovec iov[8];

  request->magic1 = MAGIC1;
  request->magic2 = MAGIC2;

  iov[0].iov_base = (void *)(&(request->magic1));
  iov[0].iov_len = sizeof(request->magic1);

  iov[1].iov_base = (void *)(&(request->magic2));
  iov[1].iov_len = sizeof(request->magic2);

  iov[2].iov_base = (void *)(&(request->session));
  iov[2].iov_len = sizeof(request->session);

  iov[3].iov_base = (void *)(&(request->type));
  iov[3].iov_len = sizeof(request->type);

  iov[4].iov_base = (void *)(&(request->address1));
  iov[4].iov_len = sizeof(request->address1);

  iov[5].iov_base = (void *)(&(request->address2));
  iov[5].iov_len = sizeof(request->address2);

  iov[6].iov_base = (void *)(&(request->receiver));
  iov[6].iov_len = sizeof(request->receiver);

  iov[7].iov_base = (void *)(&(request->ctrl));
  iov[7].iov_len = sizeof(request->ctrl);

  iov[8].iov_base = (void *)(&(request->code));
  iov[8].iov_len = sizeof(request->code);

  return writev (fd, iov, 8);
}

int i2c_gw_receive(int fd, struct i2c_gw_request *request)
{
  int rc;

  rc = read(fd, (char *)request, sizeof(struct i2c_gw_request));
  if (rc < sizeof(struct i2c_gw_request)) {
    return -1;
  }

  if (request->magic1 != MAGIC1) {
    fprintf(stderr, "BAD MAGIC1\n");
    return -1;
  }

  if (request->magic2 != MAGIC2) {
    fprintf(stderr, "BAD MAGIC2\n");
    return -1;
  }

  return 0;
}

static void mqtt_callback(void *obj, const char *topic, const void *payload,
        int payloadlen)
{
  struct i2c_gw_request request;
  char *ptr, *end;
  LIST *els;
  int *fd, receiver, address, code, rc;

  bzero(&request, sizeof(request));

  fd = (int *) obj;

  els = str_split(topic, "/");
  if (hl_list_len(els) != 4) {
    dlog(DLOG, DLOG_ERR, "Topic doesn't match i2c gateway pattern: %s", topic);
    goto clean;
  }

  ptr = hl_list_get(els, 1);
  request.type = device_from_str(ptr);

  ptr = hl_list_get(els, 2);
  receiver = strtol(ptr, &end, 10);
  if (errno == ERANGE && (receiver == LONG_MAX || receiver == LONG_MIN)) {
    dlog(DLOG, DLOG_ERR, "Receiver malformed: %s", topic);
    goto clean;
  }
  request.receiver = receiver;

  ptr = hl_list_get(els, 3);
  address = strtol(ptr, &end, 10);
  if (errno == ERANGE && (address == LONG_MAX || address == LONG_MIN)) {
    dlog(DLOG, DLOG_ERR, "Address malformed: %s", topic);
    goto clean;
  }
  hl_list_free(els);

  request.address1 = (address >> 16) & 0xffff;
  request.address2 = address & 0xffff;

  ptr = xmalloc(payloadlen + 1);
  memset(ptr, 0, payloadlen + 1);
  memcpy(ptr, payload, payloadlen);

  els = str_split(ptr, ";");
  free(ptr);

  if (hl_list_len(els) != 2) {
    dlog(DLOG, DLOG_ERR, "Not enough element in payload: %s", payload);
    goto clean;
  }

  ptr = hl_list_get(els, 1);
  code = strtol(ptr, &end, 10);
  if (errno == ERANGE && (code == LONG_MAX || code == LONG_MIN)) {
    dlog(DLOG, DLOG_ERR, "Code malformed: %s", ptr);
    goto clean;
  }
  request.code = code;

  switch(request.type) {
    case HOMEASY:
      request.ctrl = homeasy_get_ctrl_int(hl_list_get(els, 0));
      break;
    default:
      request.ctrl = 0;
  }

  printf("%d %d/%d %d %d\n", request.receiver,
          request.address1, request.address2, request.code,
          request.ctrl);

  rc = i2c_gw_send(*fd, &request);
  if (rc == -1) {
    dlog(DLOG, DLOG_ERR, "Unable to send an i2c request");
  }

clean:
  hl_list_free(els);
}

int publish_request(const char *endpoint, struct i2c_gw_request *request)
{
  char *output, *payload = NULL;
  const char *ctrl;
  int rc;

  rc = snprintf(NULL, 0, "%s/%s/%d/%d", endpoint,
          device_from_int(request->type),
          request->receiver,
          (request->address1 << 16) + request->address2);
  if (rc < 0) {
    dlog(DLOG, DLOG_ERR, "Unable to build the topic");
    return -1;
  }

  output = xcalloc(1, rc + 1);
  rc = snprintf(output, rc + 1, "%s/%s/%d/%d", endpoint,
          device_from_int(request->type),
          request->receiver,
          (request->address1 << 16) + request->address2);
  if (rc < 0) {
    dlog(DLOG, DLOG_ERR, "Unable to build the topic");
    goto clean;
  }

  if (request->type == HOMEASY) {
    ctrl = homeasy_get_ctrl_str(request->ctrl);
  } else {
    ctrl = srts_get_ctrl_str(request->ctrl);
  }

  rc = snprintf(NULL, 0, "%s;%d", ctrl, request->code);
  if (rc < 0) {
    dlog(DLOG, DLOG_ERR, "Unable to build the payload");
    goto clean;
  }

  payload = xcalloc(1, rc + 1);
  rc = snprintf(payload, rc + 1, "%s;%d", ctrl, request->code);
  if (rc < 0) {
    dlog(DLOG, DLOG_ERR, "Unable to build the payload");
    goto clean;
  }

  rc = mqtt_publish(output, payload);

clean:
  free(output);

  if (payload != NULL) {
    free(payload);
  }

  return rc;
}

int main(int argc, char **argv)
{
  struct i2c_gw_request request;
  char *filename = "/dev/i2c-1";
  int fd, rc;

  DLOG = dlog_init(DLOG_STDERR, DLOG_DEBUG, "i2c_gatewayd");
  assert(DLOG != NULL);

  if ((fd = i2c_gw_open(filename, 0x8)) < 0) {
    perror("Failed to open the i2c bus");
    exit(1);
  }

  mqtt_init();

  if (!mqtt_subscribe("mqtt://localhost:1883/switches/+/+/+", &fd, mqtt_callback)) {
    perror("Unable to subscribe");
    exit(1);
  }

  for(;;) {
    rc = i2c_gw_receive(fd, &request);
    if (rc == 0) {
      printf("----- BEGIN -----\n");
      printf("%d\n", request.ctrl);
      printf("%d\n", request.address1);
      printf("%d\n", request.address2);
      printf("= %d\n", (request.address1 << 16) + request.address2);
      publish_request("mqtt://localhost:1883/switches", &request);
    } else {
      usleep(50000);
    }
  }

  return 0;
}

