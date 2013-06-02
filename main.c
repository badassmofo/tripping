#include "main.h"

int main(int argc, const char *argv[]) {
	srand((unsigned int)time(NULL));
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);

	modes_e mode           = M_SINGLE;
	int extra_args         = 2;
	unsigned int total_gen = 0;
	char *mine_regex       = NULL;

	if (argc > 1) {
		if (strcmp(argv[1], "help") == 0) {
			print_help();
			return EXIT_SUCCESS;
		}
		else if (strcmp(argv[1], "test") == 0)
			mode = M_TEST;
		else if (strcmp(argv[1], "gen") == 0) {
			mode = M_GEN;
			if (argc >= 2) {
				total_gen = (unsigned)atoi(argv[2]);
				++extra_args;
			}
			else {
				printf("ERROR! Generate mode requires another argument!\n");
				return EXIT_FAILURE;
			}
		}
		else if (strcmp(argv[1], "mine") == 0) {
			mode = M_MINE;
			if (argc >= 2) {
				size_t arg_len = strlen(argv[2]);
				mine_regex = (char*)malloc(arg_len * sizeof(char));
				strncpy(mine_regex, argv[2], arg_len);
				++extra_args;
			}
			else {
				printf("ERROR! Mine mode requires another argument!\n");
				return EXIT_FAILURE;
			}
		}
		else {
			printf("ERROR! Invalid mode entered!\n\n");
			print_help();
			return EXIT_FAILURE;
		}
	}

#if defined DEBUGGING
	printf("MODE:       %d\n", mode);
	printf("EXTRA_ARGS: %d\n", extra_args);
	printf("TOTAL_GEN:  %d\n", total_gen);
	printf("MINE_REGEX: %s\n\n", mine_regex);
#endif

	unsigned int threads = 1,
		     min_rnd = 1,
		     max_rnd = 14,
		     timeout = 0;
	bool benchmark       = false,
	     no_sjis         = false,
	     mine_timeout    = false,
	     non_stop_gen    = false;
	
	if (argc > extra_args && mode != M_SINGLE && mode != M_TEST) {
		for (int i = extra_args; i < argc; ++i) {
			if (strcmp("--benchmark", argv[i]) == 0 || strcmp("-b", argv[i]) == 0)
				benchmark = true;
			else if (strcmp("--no-sjis", argv[i]) == 0 || strcmp("-ns", argv[i]) == 0)
				no_sjis = true;
			else if (strcmp("--dont-stop", argv[i]) == 0 || strcmp("-ds", argv[i]) == 0)
				non_stop_gen = true;
			else if (strcmp("--threads", argv[i]) == 0 || strcmp("-pt", argv[i]) == 0) {
				if ((i + 1) > (argc - 1)) {
					printf("WARNING! No argument passed after %s! Skipping!\n", argv[i]);
					continue;
				}

				threads = (unsigned)atoi(argv[++i]);
				if (threads <= 1) {
					printf("WARNING! Invalid number of threads entered! Using single-threading!\n");
					threads = 1;
				}
			}
			else if (strcmp("--timeout", argv[i]) == 0 || strcmp("-t", argv[i]) == 0) {
				if ((i + 1) > (argc - 1)) {
					printf("WARNING! No argument passed after %s! Skipping!\n", argv[i]);
					continue;
				}
				
				timeout      = (unsigned)atoi(argv[++i]);
				mine_timeout = true;
				if (timeout <= 0) {
					printf("WARNING! Invalid timeout entered! Disabling timeout!\n");
					timeout      = 0;
					mine_timeout = false;
				}
			}
			else if (strcmp("--min-rnd", argv[i]) == 0 || strcmp("-min", argv[i]) == 0) {
				if ((i + 1) > (argc - 1)) {
					printf("WARNING! No argument passed after %s! Skipping!\n", argv[i]);
					continue;
				}
				min_rnd = (unsigned)atoi(argv[++i]);
			}
			else if (strcmp("--max-rnd", argv[i]) == 0 || strcmp("-max", argv[i]) == 0) {
				if ((i + 1) > (argc - 1)) {
					printf("WARNING! No argument passed after %s! Skipping!\n", argv[i]);
					continue;
				}
				max_rnd = (unsigned)atoi(argv[++i]);
			}
			else printf("WARNING! %s is not a valid argument!\n", argv[i]);
		}
	}

	if (min_rnd > max_rnd) {
		printf("WARNING! min-rnd was greater than max-rnd! Resettings to default values (1 & 14)\n");
		min_rnd = 1;
		max_rnd = 14;
	}

#if defined DEBUGGING
	printf("THREADS:      %d\n", threads);
	printf("MIN_RND:      %d\n", min_rnd);
	printf("MAX_RND:      %d\n", max_rnd);
	printf("BENCHMARK:    %d\n", benchmark);
	printf("NO_SJIS:      %d\n", no_sjis);
	printf("MINE_TIMEOUT: %d\n", mine_timeout);
	printf("TIMEOUT:      %d\n", timeout);
	printf("NON_STOP_GEN: %d\n", non_stop_gen);
#endif

	long start_time          = get_time();
	unsigned int total_trips = 0;
	switch (mode) {
		case M_MINE:
			if (strlen(mine_regex) <= 0) {
				printf("ERROR! Blank regex passed!\n");
				return EXIT_FAILURE;
			}
			break;
		case M_TEST:
			test_mode();
			break;
		case M_GEN:
			if (total_gen <= 0) {
				printf("WARNING! Total generation amount is 0, enabling non-stop mode!\n");
				non_stop_gen = true;
			}
			if (threads > total_gen) {
				printf("WARNING! Total generation amount is less than total threads! Capping total threads!\n");
				threads = total_gen;
			}
			break;
		case M_SINGLE:
			single_mode();
			break;
	}

	if (benchmark) {
		float exec_time = ((get_time() - start_time) / 1000000.f);
		printf("\nexec time:   %f\ntotal trips: %d\ntrips p/s:   %f\n",
				exec_time, total_trips, (total_trips / exec_time));
	}
	return EXIT_SUCCESS;
}

void print_help() {
	printf("help!\n");
}

void signal_handler(int signal) {
	switch (signal) {
		case SIGTERM:
			printf(" EXITING: SIGTERM (terminated)\n");
			break;
		case SIGINT:
			printf(" EXITING: SIGINT (interrupted)\n");
			break;
		default:
			printf(" EXITING: UNKNOWN (%d)\n", signal);
			break;
	}
	exit_loops = 1;
}

long get_time() {
#if ((__APPLE__ & __MACH__) || macintosh || Macintosh)
	static mach_timebase_info_data_t freq = {0, 0};
	if (freq.denom == 0)
		mach_timebase_info(&freq);
	return (mach_absolute_time() * freq.numer / freq.denom) / 1000;
#elif (__WIN32__ || _WIN32 || _WIN64 || __WINDOWS__)
	HANDLE cur_thread   = GetCurrentThread();
	DWORD_PTR prev_mask = SetThreadAffinityMask(cur_thread, 1);
	
	static LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	
	SetThreadAffinityMask(cur_thread, prev_mask);
	return (1000000 * time.QuadPart / freq.QuadPart);
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((long)(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000);
#endif
}

void single_mode() {

}

void test_mode() {
	char* test_str = "*驂出s深";
	iconv_t cd = iconv_open("SJIS//IGNORE", "UTF-8");
	char* trip = gen_trip_sjis(cd, test_str, strlen(test_str));

	printf("%s\n", trip);
	free(trip);
}

