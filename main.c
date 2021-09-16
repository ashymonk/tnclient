#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <termios.h>
#include <fcntl.h>

static struct termios term;

void negotiate(int sock, unsigned char *buf, int len) {
	printf("%d: %d\n", buf[1], buf[2]);

	if (buf[1] == DO)
		buf[1] = WONT;
	else if (buf[1] == DONT)
		buf[1] = WONT;
	else if (buf[1] == WILL)
		buf[1] = DONT;
	else if (buf[1] == WONT)
		buf[1] = DONT;

	if (send(sock, buf, len , 0) < 0)
		exit(1);
}

void echo_disable(void)
{
	struct termios term_tmp;

	tcgetattr(STDIN_FILENO, &term);
	memcpy(&term_tmp, &term, sizeof(struct termios));
	term_tmp.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &term_tmp);
}

void echo_enable(void)
{
	// restore terminal upon exit
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int main(int argc, char **argv)
{
	int sock;
	struct sockaddr_in server;
	unsigned char buf[512];
	int len;
	int i;

	if (argc < 2) {
		printf("Usage: %s address\n", argv[0]);
		return 1;
	}
	int port = 23;
	if (argc == 3)
		port = atoi(argv[2]);

	//Create socket
	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1) {
		perror("Could not create socket. Error");
		return 1;
	}

	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("connect failed. Error");
		return 1;
	}
	puts("Connected...\n");

	// set terminal
//	echo_disable();
//	atexit(echo_enable);

	struct timeval ts;
	ts.tv_sec = 1; // 1 second
	ts.tv_usec = 0;

	while (1) {
		// select setup
		fd_set fds;
		FD_ZERO(&fds);
		if (sock != 0)
			FD_SET(sock, &fds);
		FD_SET(0, &fds);

		// wait for data
		int nready = select(sock + 1, &fds, (fd_set *) 0, (fd_set *) 0, &ts);
		if (nready < 0) {
			perror("select. Error");
			return 1;
		}
		else if (nready == 0) {
			ts.tv_sec = 1; // 1 second
			ts.tv_usec = 0;
		}
		else if (sock != 0 && FD_ISSET(sock, &fds)) {
			// start by reading a single byte
			int rv;
			if ((rv = recv(sock , buf , 1 , 0)) < 0)
				return 1;
			else if (rv == 0) {
				printf("Connection closed by the remote end\n\r");
				return 0;
			}

			if (buf[0] == IAC) {
				// read 2 more bytes
				len = recv(sock , buf + 1 , 2 , 0);
				if (len  < 0)
					return 1;
				else if (len == 0) {
					printf("Connection closed by the remote end\n\r");
					return 0;
				}
negotiate(sock, buf, 3);
			}
			else {
				len = 1;
				buf[len] = '\0';
				printf("%s", buf);
				fflush(0);
			}
		}

		else if (FD_ISSET(0, &fds)) {
			buf[0] = getc(stdin); //fgets(buf, 1, stdin);
			if (send(sock, buf, 1, 0) < 0)
				return 1;
			if (buf[0] == '\n') // with the terminal in raw mode we need to force a LF
				putchar('\r');
		}
	}
	close(sock);
	return 0;
}
