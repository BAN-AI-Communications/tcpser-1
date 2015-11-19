#include <string.h>

#include "debug.h"
#include "ip.h"
#include "nvt.h"

int nvt_init_config(nvt_vars *vars) {
  int i; 

  vars->binary_xmit = FALSE;
  vars->binary_recv = FALSE;
  for (i = 0; i < 256; i++)
    vars->term[i] = 0;

  return 0;
}

unsigned char get_nvt_cmd_response(unsigned char action, unsigned char type) {
  unsigned char rc=0;

  if(type == TRUE) {
    switch (action) {
      case NVT_DO:
        rc=NVT_WILL_RESP;
        break;
      case NVT_DONT:
        rc=NVT_WONT_RESP;
        break;
      case NVT_WILL:
        rc=NVT_DO_RESP;
        break;
      case NVT_WONT:
        rc=NVT_DONT_RESP;
        break;
    }
  } else {
    switch (action) {
      case NVT_DO:
      case NVT_DONT:
        rc=NVT_WONT_RESP;
        break;
      case NVT_WILL:
      case NVT_WONT:
        rc=NVT_DONT_RESP;
        break;
    }
  }
  return rc;
}

int parse_nvt_subcommand(int fd, nvt_vars *vars , unsigned char * data, int len) {
  // overflow issue, again...
  int opt=data[2];
  unsigned char resp[100];
  unsigned char *response = resp + 3;
  int resp_len = 0;
  int response_len = 0;
  unsigned char tty_type[]="VT100";
  int rc;
  int slen=0;

  for (rc = 2; rc < len - 1; rc++) {
    if (NVT_IAC == data[rc])
      if (NVT_SE == data[rc + 1]) {
        rc += 2;
        break;
      }
  }

  if (rc > 5 && (NVT_SB_SEND == data[4])) {
    switch(opt) {
      case NVT_OPT_TERMINAL_TYPE:
      case NVT_OPT_X_DISPLAY_LOCATION:  // should not have to have these
      case NVT_OPT_ENVIRON:             // but telnet seems to expect.
      case NVT_OPT_NEW_ENVIRON:         // them.
      case NVT_OPT_TERMINAL_SPEED:
        response[response_len++] = NVT_SB_IS;
        switch(opt) {
          case NVT_OPT_TERMINAL_TYPE:
            slen=strlen((char *)tty_type);
            strncpy((char *)response + response_len,(char *)tty_type,slen);
            response_len += slen;
            break;
        }
        break;
    }
  }

  if (response_len) {
    resp[resp_len++]=NVT_IAC;
    resp[resp_len++]=NVT_SB;
    resp[resp_len++]=opt;
    resp_len += response_len;
    resp[resp_len++]=NVT_IAC;
    resp[resp_len++]=NVT_SE;
    ip_write(fd,resp,resp_len);
  }
  return rc;
}

int send_nvt_command(int fd, nvt_vars *vars, unsigned char action, unsigned char opt) {
  unsigned char cmd[3];
  cmd[0]= NVT_IAC;
  cmd[1] = action;
  cmd[2]= opt;

  ip_write(fd,cmd,3);
  vars->term[opt] = action;

  return 0;
}


int parse_nvt_command(int fd, nvt_vars *vars, unsigned char action, unsigned char opt) {
  unsigned char resp[3];
  resp[0]= NVT_IAC;
  resp[2]= opt;

  switch (opt) {
    case NVT_OPT_TRANSMIT_BINARY :
      switch (action) {
        case NVT_DO :
          LOG(LOG_INFO,"Enabling telnet binary xmit");
          vars->binary_xmit = TRUE;
          break;
        case NVT_DONT :
          LOG(LOG_INFO,"Disabling telnet binary xmit");
          vars->binary_xmit = FALSE;
          break;
        case NVT_WILL :
          LOG(LOG_INFO,"Enabling telnet binary recv");
          vars->binary_recv = TRUE;
          break;
        case NVT_WONT :
          LOG(LOG_INFO,"Disabling telnet binary recv");
          vars->binary_recv = FALSE;
          break;
      }
      // fall through to get response

    case NVT_OPT_NAWS:
    case NVT_OPT_TERMINAL_TYPE:
    case NVT_OPT_SUPPRESS_GO_AHEAD:
    case NVT_OPT_ECHO:
    case NVT_OPT_X_DISPLAY_LOCATION:  // should not have to have these
    case NVT_OPT_ENVIRON:             // but telnet seems to expect.
    case NVT_OPT_NEW_ENVIRON:         // them.
    case NVT_OPT_TERMINAL_SPEED:
      resp[1] = get_nvt_cmd_response(action,TRUE);
      break;
    default:
      resp[1] = get_nvt_cmd_response(action,FALSE);
      break;
  }
  ip_write(fd,resp,3);
  return 0;
}

