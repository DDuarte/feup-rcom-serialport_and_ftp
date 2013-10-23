#include "app.h"
#include "misc.h"
#include "link.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>

bool seed_set = false;

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
    fprintf(stderr, " -bcc1e \t Set bcc1 error simulation probability (0 - 100)\n");
    fprintf(stderr, " -bcc2e \t Set bcc2 error simulation probability (0 - 100)\n");
    fprintf(stderr, " -bccseed \t Set bcc error simulation seed\n");

}

int process_optional_args(int initial_idx, int argc, char** argv)
{
	if (initial_idx + 1 >= argc)
		return -1;

    for (; initial_idx < argc; initial_idx += 2)
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
            int baudrate = atoi(argv[initial_idx + 1]);

			if (baudrate < 0)
			{
				ERROR("invalid baudrate value\n");
				return EXIT_FAILURE;
			}

			conf_set_baudrate(baudrate);
		}
        else if (strcmp(argv[initial_idx], "-bcc1e") == 0)
        {
            int bcc1_error_prob = atoi(argv[initial_idx + 1]);

            if (bcc1_error_prob < 0 || bcc1_error_prob > 100)
                fprintf(stderr, "Warning: invalid -bcc1e value was ignored\n");
            else
                conf_bcc1_prob_error(bcc1_error_prob);
        }
        else if (strcmp(argv[initial_idx], "-bcc2e") == 0)
        {
            int bcc2_error_prob = atoi(argv[initial_idx + 1]);

            if (bcc2_error_prob < 0 || bcc2_error_prob > 100)
                fprintf(stderr, "Warning: invalid -bbc2e value was ignored\n");
            else
                conf_bcc2_prob_error(bcc2_error_prob);
        }
        else if (strcmp(argv[initial_idx], "-bbcseed") == 0)
        {
            int bcc_seed = atoi(argv[initial_idx + 1]);
            conf_set_rand_seed(bcc_seed);
            seed_set = true;
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

	if (opt_start_index + 1 < argc)
		if (process_optional_args(opt_start_index, argc, argv) < 0)
		{
			print_usage(argv[0]);
			return EXIT_FAILURE;
		}

    if (!seed_set)
        conf_set_rand_seed(time(NULL));

	if (is_sender)
	{
		if (app_send_file(argv[2], argv[3]) != 0)
		{
			perror("app_send_file");
			return EXIT_FAILURE;
		}
	}
	else
	{
        if (app_receive_file(argv[2], argv[3]) != 0)
		{
			perror("app_receive_file");
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
