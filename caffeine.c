#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <wordexp.h>

#include "config.h"

#ifndef CAFFEINE_VERSION
#define CAFFEINE_VERSION "unknown"
#endif

// flags for sig handler
volatile sig_atomic_t daemon_active  = false;
volatile sig_atomic_t daemon_closing = false;

// global vars (expanded from config, RO except from init function)
char* g_pidfile;
char* g_activefile;

/* COMMON CODE */

/**
 * Check if a file exists
 * @param filename
 * @return true if it exists
 */
bool file_exists(const char* filename) {
    // attempt to open the file for reading
    FILE* file;
    if ((file = fopen(filename, "r"))) {
        fclose(file);
        return true;
    }
    return false;
}

/**
 * Remove a file.
 * @param filename file path to remove
 * @return true if successful
 */
bool file_remove(const char* filename) {
    if (unlink(filename) == -1) {
        return false;
    }
    return true;
}

/**
 * Write to a file.
 * @param filename file path to write to
 * @param contents contents to write
 * @return true if successful
 */
bool file_write(const char* filename, const char* contents) {
    FILE* file;
    if ((file = fopen(filename, "w"))) {
        fprintf(file, "%s", contents);
        fclose(file);
        return true;
    }
    return false;
}

/**
 * Write an int to a file.
 * @param filename file path to write to
 * @param integer  int to write
 * @return true if successful
 */
bool file_write_int(const char* filename, int integer) {
    FILE* file;
    if ((file = fopen(filename, "w"))) {
        fprintf(file, "%d", integer);
        fclose(file);
        return true;
    }
    return false;
}


/**
 * If the daemon is running, get the PID.
 * @return daemon PID, otherwise -1
 */
int get_daemon_pid() {
    unsigned int fpos = 0;
    char ch;
    char contents[100];

    // attempt to open the file for reading
    FILE* file;
    if ((file = fopen(g_pidfile, "r"))) {

        // read the contents of the file into the buffer
        while((ch = fgetc(file)) != EOF && fpos < 100)
            contents[fpos++] = (char)ch;

        // close the file
        fclose(file);


        // parse the contents of the file
        int pid = atoi(contents);
        
        if (kill(pid, 0) == -1) {
            return -1;
        } else {
            return pid;
        }
    }

    return -1;
}

/* DAEMON CODE */

void signal_handler(int sig) {
    // catch all the signals
    switch (sig) {
        case SIGUSR1:
            daemon_active = true;
            file_write(g_activefile, "");
            break;
        case SIGUSR2:
            daemon_active = false;
            file_remove(g_activefile);
            break;
        case SIGINT:
            daemon_closing = true;
            break;
        default:
            break;
    }

}
/**
 * Init the daemon (write pidfile, create lock, register signal handlers, etc.)
 */
void init_daemon() {
    // check if a pidfile exists. if not, write one
    if (get_daemon_pid() != -1) {
        printf("caffeine daemon already running!\n");
        exit(1);
    } else {
        file_write_int(g_pidfile, getpid());
    }

    // remove the active file. we don't care if it succeeds, bc we known now
    // that we are the only daemon on the system.
    file_remove(g_activefile);

    // register signal handlers
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1 ||
        sigaction(SIGUSR2, &sa, NULL) == -1 ||
        sigaction(SIGINT,  &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // daemon main loop
    while (!daemon_closing) {
        usleep(caffeine_wait);

        // check if we're active (flag from setter)
        if (daemon_active) {
            system("xdotool mousemove_relative --sync --  1 0 && xdotool mousemove_relative --sync -- -1 0");
        }
    }

    file_remove(g_pidfile);
    file_remove(g_activefile);
}


/**
 * Expand the paths in the configuration (sets globals).
 */
void init_config_vars() {
    wordexp_t exp_result;

    // pidfile first (expand, alloc, copy)
    wordexp(caffeine_pidfile, &exp_result, 0);
    g_pidfile = malloc(strlen(exp_result.we_wordv[0]));
    strcpy(g_pidfile, exp_result.we_wordv[0]);

    // next, do the activefile
    wordexp(caffeine_activefile, &exp_result, 0);
    g_activefile = malloc(strlen(exp_result.we_wordv[0]));
    strcpy(g_activefile, exp_result.we_wordv[0]);

    // free everything afterwards
    wordfree(&exp_result);
}

/* STATUS TOOLS */
bool get_daemon_active() {
    if (get_daemon_pid() != -1 && file_exists(g_activefile)) {
        return true;
    }
    return false;
}

bool status_signal(int sig) {
    int dpid = get_daemon_pid();

    // check if it's running
    if (dpid == -1) {
        printf("Daemon is not running!\n");
        return false;
    }

    // send the signal
    kill(dpid, sig);

    return true;
}

bool status_start() {
    return status_signal(SIGUSR1);
}

bool status_stop() {
    return status_signal(SIGUSR2);
}

bool status_toggle() {
    if (get_daemon_active()) {
        return status_stop();
    } else {
        return status_start();
    }
}


int main(int argc, char** argv) {

    init_config_vars();

    if (argc != 2) {
        printf("Usage: caffeine (daemon|query|start|stop|toggle|diagnostic|status|version)\n");
        return 1;
    }

    if (strcmp(argv[1], "daemon") == 0) {
        init_daemon();
    } else if (strcmp(argv[1], "start") == 0) {
        status_start();
    } else if (strcmp(argv[1], "stop") == 0) {
        status_stop();
    } else if (strcmp(argv[1], "toggle") == 0) {
        status_toggle();
    } else if (strcmp(argv[1], "query") == 0) {
        return !get_daemon_active();
    } else if (strcmp(argv[1], "status") == 0) {
        if (get_daemon_active()) {
            printf("Caffeine active.\n");
        } else {
            printf("Caffeine not active.\n");
        }
    } else if (strcmp(argv[1], "diagnostic") == 0) {
        printf("daemon pid file: %s\ndaemon pid: %d\n", g_pidfile, get_daemon_pid());
        printf("daemon active file: %s\n", g_activefile);
        printf("daemon sleep time: %g seconds\n", (double)caffeine_wait / (1000000));
        printf("daemon active? ");
        if (get_daemon_active()) {
            printf("yes\n");
        } else {
            printf("no\n");
        }
    } else if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("caffeine version %s, built %s %s\n", CAFFEINE_VERSION, __TIME__, __DATE__);
        printf("(c) 2019 Patrick Kage (https://kagelabs.org)\n");
    } else {
        printf("Unknown command \"%s\".\n", argv[1]);
    }

    return 0;
}
