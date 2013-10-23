#include "app.h"
#include "misc.h"
#include "link.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>

void print_usage(char* program)
{
	ERROR("Usage:");
	ERRORF("\t- %s [send | receive] <terminal_filename> <input_file | output_file> [[-t t_arg | -r r_arg | -m m_arg | -b b_arg] ... ]", program);
	ERROR("Note: errors and other messages are printed to stderr");

	fprintf(stderr, "Configuration:\n");
	fprintf(stderr, " -t \t Set timeout value\n");
	fprintf(stderr, " -r \t Set number of retries\n");
	fprintf(stderr, " -m \t Set maximum information frame size\n");
	fprintf(stderr, " -b \t Set baudrate value\n");

}

int process_optional_args(int initial_idx, int argc, char** argv)
{
	if (initial_idx + 1 >= argc)
		return -1;

	for(; initial_idx < argc; initial_idx += 2)
	{
		if (strcmp(argv[initial_idx], "-t") == 0)
		{
			int timeout = atoi(argv[initial_idx + 1]);
			conf_set_timeout(timeout);
		}
		else if (strcmp(argv[initial_idx], "-r") == 0)
		{
			int retries = atoi(argv[initial_idx + 1]);
			conf_set_retries(retries);
		}
		else if (strcmp(argv[initial_idx], "-m") == 0)
		{
			int max_frame_size = atoi(argv[initial_idx + 1]);
			conf_set_max_info_frame_size(max_frame_size);
		}
		else if (strcmp(argv[initial_idx], "-b") == 0)
		{
			int baudrate = get_proper_baudrate(atoi(argv[initial_idx + 1]));

			if (baudrate < 0)
			{
				ERROR("invalid baudrate value\n");
				return EXIT_FAILURE;
			}

			conf_set_baudrate(baudrate);
		}
		else
		{
			print_usage(argv[0]);
			return -1;
		}
	}

	return 0;
}

int main(int argc, char* argv[])
{ LOG

	if (argc < 2)
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	int opt_param_number = argc - 1, opt_start_index = 0;
	bool is_sender = false;
	if (strcmp(argv[1], "send") == 0 || strcmp(argv[1], "receive") == 0)
	{
		if (argc < 4)
		{
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}

		opt_param_number -= 3;
		opt_start_index = 4;
	}
	else
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	is_sender = (strcmp(argv[1], "send") == 0);

	//printf("number options: %d\n", opt_param_number);
	//printf("option_start_index: %d\n", opt_start_index);
	//printf("argc: %d\n", argc);

	if (opt_start_index + 1 < argc)
		if (process_optional_args(opt_start_index, argc, argv) < 0)
		{
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}

	if (is_sender)
	{
		//printf("Sender: port: %s \t file: %s\n", argv[2], argv[3]);
		if (app_send_file(argv[2], argv[3]) != 0)
		{
			perror("app_send_file");
			return EXIT_FAILURE;
		}
	}
	else
	{
		int fd = open(argv[3], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);

		if (fd < 0)
		{
			perror("opening destination file");
			return EXIT_FAILURE;
		}

		if (dup2(fd, STDOUT_FILENO) < 0)
		{
			perror("duplicate file descriptors");
			return EXIT_FAILURE;
		}

		if (app_receive_file(argv[2]) != 0)
		{
			perror("app_receive_file");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
