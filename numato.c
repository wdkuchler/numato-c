/*
 ============================================================================
 Name        : numato.c
 Author      : Willy Kuchler
 Version     : 1.0
 Copyright   : Numato Systems Pvt. Ltd.
 Product     : Numato Lab 1 Channel USB Powered Relay Module
 Device      : idVendor=2a19, idProduct=0c05
============================================================================

- To compile on Linux: gcc -Wall -g -o numato numato.c -lcurl

- To enable device read/write, create/edit file: sudo vi /etc/udev/rules.d/70-numato.rules an isert folow line: 
ACTION=="add", KERNEL=="ttyACM[0-9]*", ATTRS{idVendor}=="2a19", ATTRS{idProduct}=="0c05", MODE="0666"

- To run
on Linux - ./numato -d /dev/ttyACM0 -c pulse

on WSL - ./numato -d /dev/ttyS4 -c pulse

*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>

static const char wakeUp[] = "\r";
static const char openIt[] = "relay on 0\r";
static const char closeIt[] = "relay off 0\r";

struct url_data
{
	size_t size;
	char *data;
};

static size_t write_data(char *buffer, size_t size, size_t nitems, void *userp)
{
	struct url_data *data = (struct url_data *)userp;
	size_t index = data->size;
	size_t n = (size * nitems);
	data->size += (size * nitems);
	char *tmp = realloc(data->data, data->size + 1);
	if(tmp)
	{
		data->data = tmp;
	}
	else
	{
		if(data->data)
			free(data->data);
		fprintf(stderr, "Failed to allocate memory.\n");
		return EXIT_FAILURE;
	}
	memcpy(data->data + index, buffer, n);
	data->data[data->size] = '\0';
	return size *nitems;
}

void EbanxReset(void)
{
	CURL *curl;
	CURLcode res;

	struct url_data data;

	data.size = 0;
	data.data = (char *)malloc(4096); /* reasonable size initial buffer */
	bzero(data.data, 4096);

	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
		curl_easy_setopt(curl, CURLOPT_URL, "http://ipkiss.pragmazero.com/reset");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
		struct curl_slist *headers = NULL;
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		else
		{
			fprintf(stderr, "Data:[%s] Ret:[%s]\n", data.data, curl_easy_strerror(res));
		}
	}
	curl_easy_cleanup(curl);
}

void usage(char *progname)
{
	fprintf(stderr, "Usage\n%s -d device -c command\n", progname);
	fprintf(stderr, "Where:\n");
	fprintf(stderr, "\ttype device name, Ex.: /dev/ttyACM0\n");
	fprintf(stderr, "\ttype command, Ex.: \"pulse\" or \"on\" and \"off\"\n");
}

int wakeup(char *device)
{
	int fd;
	
	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		printf("Error opening device: %s - %s\n", device, strerror(errno));
		return(0);
	}
	if (!write(fd, wakeUp, strlen(wakeUp)))
	{
		close(fd);
		printf("Error: Unable to write to the specified device\n");
		return (0);
	}
	close(fd);
	return (1);
}

int activate(char *device)
{
	int fd;
	
	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		printf("Error opening device: %s - %s\n", device, strerror(errno));
		return(0);
	}

	if(!write(fd, openIt, strlen(openIt)))
	{
		close(fd);
		printf("Error: Unable to write to the specified device\n");
		return(0);
	}
	close(fd);
	return(1);
}

int deactivate(char *device)
{
	int fd;
	
	fd = open(device, O_RDWR);
	if (fd < 0)
	{
		printf("Error opening device: %s - %s\n", device, strerror(errno));
		return 0;
	}

	if (!write(fd, closeIt, strlen(closeIt)))
	{
		close(fd);
		printf("Error: Unable to write to the specified device\n");
		return(0);
	}
	close(fd);
	return(1);
}

int main(int argc, char **argv)
{
	int i;
	char *device = NULL;
	char *command = NULL;

	// EbanxReset();
	// exit(0);

	/* Parse arguments */
	if (argc > 1)
	{
		for (i = 1; i < argc; i++)
		{
			if ((argv[i][0] == '-') || (argv[i][0] == '/'))
			{
				switch (tolower(argv[i][1]))
				{
				case 'd':
					device = argv[++i];
					break;
				case 'c':
					command = argv[++i];
					break;
				default:
					usage(argv[0]);
					break;
				}
			}
			else
			{
				exit(1);
			}
		}
	}

	printf("USB Relay Module Controller - USBPOWRL002\n");

	/* verifying parameters... */
	if (device == NULL || command == NULL)
	{
		usage(argv[0]);
		exit(1);
	}

	/* wakeup serial interface */
	printf("let's wake up interface\n");
	if (!wakeup(device))
	{
		return 1;
	}
	usleep(50);

	/* Processing Command */
	if (strcmp(command, "on") == 0)
	{
		printf("let's activate relay\n");
		if (!activate(device))
		{
			return 1;
		}
	}
	else if (strcmp(command, "off") == 0)
	{
		printf("let's deactivate relay\n");
		if (!deactivate(device))
		{
			return 1;
		}
	}
	else if (strcmp(command, "pulse") == 0)
	{
		printf("let's activate relay\n");
		if (!activate(device))
		{
			return 1;
		}

		sleep(1);

		printf("let's deactivate relay\n");
		if (!deactivate(device))
		{
			return 1;
		}
	}
	else
	{
		printf("Error: invalid command argument \"%s\"\n", command);
		usage(argv[0]);
		return 1;
	}

	return (EXIT_SUCCESS);
}
