#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

// Function to print the usage of the program
void print_usage() {
    printf("Usage: ./file [process_id] [root_process] [OPTION]\n");
}

// Function to check if a process is a descendant of another process
int is_descendant(pid_t root_pid, pid_t pid) {
    char command[1024];
    sprintf(command, "pgrep -P %d", root_pid);
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    int child_pid;
    while (fscanf(pipe, "%d", &child_pid) != EOF) {
        if (child_pid == pid || is_descendant(child_pid, pid)) {
            pclose(pipe);
            return 1;
        }
    }

    pclose(pipe);
    return 0;
}

// Function to kill a process
void kill_process(pid_t pid) {
    if (kill(pid, SIGKILL) == 0) {
        printf("Process with PID %d killed.\n", pid);
    } else {
        perror("kill");
    }
}

// Function to list immediate descendants of a process
int list_immediate_descendants(pid_t pid) {
    char command[1024];
    sprintf(command, "pgrep -P %d", pid);
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    int child_pid;
    while (fscanf(pipe, "%d", &child_pid) != EOF) {
        printf("%d\n", child_pid);
        count++;
    }

    pclose(pipe);
    return count;
}

// Function to list sibling processes of a process
void list_sibling_processes(pid_t pid) {
    char command[1024];
    sprintf(command, "pgrep -P $(ps -o ppid= -p %d) | grep -v %d | grep -v %d", pid, pid, getpid());
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    int sibling_pid;
    int count = 0;
    while (fscanf(pipe, "%d", &sibling_pid) != EOF) {
        printf("%d\n", sibling_pid);
        count++;
    }

    pclose(pipe);
    if (count == 0) {
        printf("No sibling/s\n");
    }
}

// Function to list defunct descendants of a process
void list_defunct_descendants(pid_t pid) {
    char command[1024];
    sprintf(command, "pgrep -P %d", pid);
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    int child_pid;
    int found_zombies = 0; // Flag to track if any zombie processes were found
    while (fscanf(pipe, "%d", &child_pid) != EOF) {
        char cmd[1024];
        sprintf(cmd, "ps -o state= -p %d", child_pid);
        FILE* ps_pipe = popen(cmd, "r");
        if (!ps_pipe) {
            perror("popen");
            exit(EXIT_FAILURE);
        }

        char state[1024];
        fscanf(ps_pipe, "%s", state);
        if (state[0] == 'Z') {
            printf("%d\n", child_pid);
            found_zombies = 1;
        }

        pclose(ps_pipe);
    }

    pclose(pipe);

    if (!found_zombies) {
        printf("No descendant zombie process/es\n");
    }
}

// Function to list grandchildren of a process
void list_grandchildren(pid_t pid) {
    char command[1024];
    sprintf(command, "pgrep -P %d", pid);
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    int child_pid;
    int found_grandchildren = 0; // Flag to track if any grandchildren were found
    while (fscanf(pipe, "%d", &child_pid) != EOF) {
        char gc_command[1024];
        sprintf(gc_command, "pgrep -P %d", child_pid);
        FILE* gc_pipe = popen(gc_command, "r");
        if (!gc_pipe) {
            perror("popen");
            exit(EXIT_FAILURE);
        }

        int grandchild_pid;
        while (fscanf(gc_pipe, "%d", &grandchild_pid) != EOF) {
            printf("%d\n", grandchild_pid);
            found_grandchildren = 1;
        }

        pclose(gc_pipe);
    }

    pclose(pipe);

    if (!found_grandchildren) {
        printf("No grandchildren\n");
    }
}

// Function to print the status of a process (defunct or not)
void print_status(pid_t pid) {
    char command[1024];
    sprintf(command, "ps -o state= -p %d", pid);
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    char state[1024];
    fscanf(pipe, "%s", state);
    if (state[0] == 'Z') {
        printf("Defunct\n");
    } else {
        printf("Not Defunct\n");
    }

    pclose(pipe);
}

// Function to list non-direct descendants of a process
void list_non_direct_descendants(pid_t pid) {
    char command[1024];
    sprintf(command, "pgrep -P %d", pid);
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    int child_pid;
    while (fscanf(pipe, "%d", &child_pid) != EOF) {
        // Check if the current child has any descendants
        if (list_immediate_descendants(child_pid) > 0) {
            // List the descendants of this child
            char desc_command[1024];
            sprintf(desc_command, "pgrep -P %d", child_pid);
            FILE* desc_pipe = popen(desc_command, "r");
            if (!desc_pipe) {
                perror("popen");
                exit(EXIT_FAILURE);
            }

            int desc_pid;
            while (fscanf(desc_pipe, "%d", &desc_pid) != EOF) {
                printf("%d\n", desc_pid);
                count++;
            }

            pclose(desc_pipe);
        }
    }

    pclose(pipe);

    if (count == 0) {
        printf("No non-direct descendants\n");
    }
}

// Main function
int main(int argc, char *argv[]) {
    if (argc == 3) {
        // If the program is run with 3 arguments, check if the first process is a descendant of the second process
        pid_t process_id = atoi(argv[1]);
        pid_t root_process = atoi(argv[2]);
        if (!is_descendant(root_process, process_id)) {
            printf("Process with PID %d does not belong to the process tree rooted at %d.\n", process_id, root_process);
            return 1;
        } else {
            printf("%d %d\n", process_id, getppid());
            return 0;
        }
    } else if (argc < 4) {
        // If the program is run with less than 4 arguments, print the usage and exit
        print_usage();
        return 1;
    }

    // Parse the arguments
    pid_t process_id = atoi(argv[1]);
    pid_t root_process = atoi(argv[2]);
    char *option = argv[3];

    // Check if the first process is a descendant of the second process
    if (!is_descendant(root_process, process_id)) {
        printf("Process with PID %d does not belong to the process tree rooted at %d.\n", process_id, root_process);
        return 1;
    }

    // Handle different options
    if (strcmp(option, "-pr") == 0) {
        // Kill the root process
        kill_process(root_process);
        return 0;
    }

    if (strcmp(option, "-rp") == 0) {
        // Kill the specified process
        kill_process(process_id);
        return 0;
    }

    switch (option[0]) {
        case '-':
            switch (option[1]) {
                case 'r':
                    // Kill the specified process
                    kill_process(process_id);
                    break;
                case 'p':
                    // Kill the root process
                    kill_process(root_process);
                    break;
                case 'x':
                    if (strcmp(option, "-xt") == 0) {
                        // Pause the specified process
                        kill(process_id, SIGSTOP);
                        printf("Process with PID %d is paused with SIGSTOP.\n", process_id);
                    } else if (strcmp(option, "-xc") == 0) {
                        // Continue all paused processes
                        char command[1024];
                        sprintf(command, "pkill -CONT -P %d", getppid());
                        system(command);
                        printf("Continued all paused processes.\n");
                    } else if (strcmp(option, "-xg") == 0) {
                        // List grandchildren of the specified process
                        list_grandchildren(process_id);
                    } else if (strcmp(option, "-xn") == 0) {
                        // List non-direct descendants of the specified process
                        list_non_direct_descendants(process_id);
                    } else if (strcmp(option, "-xd") == 0) {
                        // List immediate descendants of the specified process
                        int count = list_immediate_descendants(process_id);
                        if (count == 0) {
                            printf("No direct descendants\n");
                        }
                    } else if (strcmp(option, "-xs") == 0) {
                        // List sibling processes of the specified process
                        list_sibling_processes(process_id);
                    } else if (strcmp(option, "-xz") == 0) {
                        // List defunct descendants of the specified process
                        list_defunct_descendants(process_id);
                    } else {
                        printf("Invalid option.\n");
                        print_usage();
                        return 1;
                    }
                    break;
                case 'z':
                    if (strcmp(option, "-zs") == 0) {
                        // Print the status of the specified process (defunct or not)
                        print_status(process_id);
                    }
                    break;
                default:
                    printf("Invalid option.\n");
                    print_usage();
                    return 1;
            }
            break;
        default:
            printf("Invalid option.\n");
            print_usage();
            return 1;
    }

    return 0;
}
