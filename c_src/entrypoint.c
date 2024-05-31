#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "iperf.h"
#include "iperf_api.h"

static int run(struct iperf_test *test);

static jmp_buf sigend_jmp_buf;

static void __attribute__ ((noreturn))
sigend_handler(int sig)
{
    longjmp(sigend_jmp_buf, 1);
}

int run_as_server(int x){
    struct iperf_test *test;

    test = iperf_new_test();
    iperf_set_test_role(test, 's');

    if (run(test) < 0)
        iperf_errexit(test, "error - %s", iperf_strerror(i_errno));

    sleep(10);

    iperf_free_test(test);

    return 0;
}

int run_as_client(int x){
    return 0;
}

static int
run(struct iperf_test *test)
{
    /* Termination signals. */
    iperf_catch_sigend(sigend_handler);
    if (setjmp(sigend_jmp_buf))
	iperf_got_sigend(test);

    /* Ignore SIGPIPE to simplify error handling */
    signal(SIGPIPE, SIG_IGN);

    switch (test->role) {
        case 's':
	    if (test->daemon) {
		int rc;
		rc = daemon(1, 0);
		if (rc < 0) {
		    i_errno = IEDAEMON;
		    iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
		}
	    }
	    if (iperf_create_pidfile(test) < 0) {
		i_errno = IEPIDFILE;
		iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
	    }
            for (;;) {
		int rc;
		rc = iperf_run_server(test);
                test->server_last_run_rc = rc;
		if (rc < 0) {
		    iperf_err(test, "error - %s", iperf_strerror(i_errno));
                    if (test->json_output) {
                        if (iperf_json_finish(test) < 0)
                            return -1;
                    }
                    iflush(test);

		    if (rc < -1) {
		        iperf_errexit(test, "exiting");
		    }
                }
                iperf_reset_test(test);
                if (iperf_get_test_one_off(test) && rc != 2) {
		    /* Authentication failure doesn't count for 1-off test */
		    if (rc < 0 && i_errno == IEAUTHTEST) {
			continue;
		    }
		    break;
		}
            }
	    iperf_delete_pidfile(test);
            break;
	case 'c':
	    if (iperf_create_pidfile(test) < 0) {
		i_errno = IEPIDFILE;
		iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
	    }
	    if (iperf_run_client(test) < 0)
		iperf_errexit(test, "error - %s", iperf_strerror(i_errno));
	    iperf_delete_pidfile(test);
            break;
        default:
            usage();
            break;
    }

    iperf_catch_sigend(SIG_DFL);
    signal(SIGPIPE, SIG_DFL);

    return 0;
}
