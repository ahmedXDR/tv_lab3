#include <iostream>
#include <sched.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib> 
#define STACK_SIZE 1048576

// Error checking helper function
void assert_success(bool condition, const char* message) {
    if (!condition) {  
        perror(message); 
        exit(EXIT_FAILURE);
    }
}

// Set the host name of the current process
void set_host_name(const std::string& hostname) {
    assert_success(sethostname(hostname.c_str(), hostname.size()) == 0, "sethostname failed");
}

// Set environment variables for the process
void setup_variables() {
    clearenv();
    setenv("TERM", "xterm-256color", 1);
    setenv("PATH", "/bin/:/sbin/:usr/bin:/usr/sbin", 1);
}

// Execute system command and check for errors
void run_system(const char* cmd) {
    assert_success(system(cmd) == 0, "System command failed");
}

// Execute system command and return output
std::string run_system_output(const char* cmd, size_t buffer_size) {
    FILE* pipe = popen(cmd, "r");
    assert_success(pipe != nullptr, "Failed to open pipe");

    char buffer[buffer_size];
    assert_success(fgets(buffer, buffer_size, pipe) != nullptr, "Failed to read output");
    assert_success(pclose(pipe) == 0, "Failed to close pipe");

    return std::string(buffer);
}

// Setup filesystem for the container
std::string setup_filesystem() {
    const char* loopbackFileName = "loopbackfile.img";
    if (access(loopbackFileName, F_OK) != 0) {
        run_system("dd if=/dev/zero of=loopbackfile.img bs=100M count=10 >/dev/null 2>/dev/null");
        run_system("mkfs.ext4 loopbackfile.img >/dev/null 2>/dev/null");
    }

    std::string loop_device = run_system_output("losetup -f --show loopbackfile.img", 64);
    loop_device.erase(loop_device.find_last_not_of(" \t\n\r\f\v") + 1);
    mkdir("mnt", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    run_system(("mount -o loop " + loop_device + " mnt").c_str());
    return loop_device;
}

// Cleanup the filesystem and unmount devices
void unsetup_filesystem(const std::string& loop_device) {
    run_system("umount mnt");
    rmdir("mnt");
    run_system(("losetup -d " + loop_device).c_str());
}

// Main function to launch a shell within the container
int main_container(void* args) {
    const char* exec_args[] = {"/bin/sh", nullptr};  // Use const char* for string literals
    return execvp("/bin/sh", const_cast<char* const*>(exec_args));
}


// Child process setup for the container
int child_fn(void* args) {
    set_host_name("container");
    setup_variables();
    assert_success(chroot("mnt") == 0, "Failed to chroot");
    assert_success(chdir("/") == 0, "Failed to chdir");
    mount("proc", "/proc", "proc", 0, 0);

    char stack[STACK_SIZE];
    assert_success(clone(main_container, stack + STACK_SIZE, SIGCHLD, NULL) >= 0, "Failed to create process");
    wait(nullptr);
    umount("/proc");
    return EXIT_SUCCESS;
}

// Main function to setup the container environment
int main() {
    std::string loop_device = setup_filesystem();
    char stack[STACK_SIZE];
    assert_success(clone(child_fn, stack + STACK_SIZE, CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNET | CLONE_NEWNS | SIGCHLD, NULL) >= 0, "Failed to create container");
    wait(nullptr);
    unsetup_filesystem(loop_device);
    return EXIT_SUCCESS;
}
