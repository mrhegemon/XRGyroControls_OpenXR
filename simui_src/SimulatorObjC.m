@import Darwin;
@import ObjectiveC;
@import UniformTypeIdentifiers;

#include "simui_types.h"

void *shm_addr = NULL;

int redirect_nslog(const char *prefix, const char *buffer, int size)
{
    NSLog(@"%s (%d bytes): %.*s", prefix, size, size, buffer);
    return size;
}

int stderr_redirect_nslog(void *inFD, const char *buffer, int size)
{
    return redirect_nslog("stderr", buffer, size);
}

int stdout_redirect_nslog(void *inFD, const char *buffer, int size)
{
    return redirect_nslog("stdout", buffer, size);
}

OBJC_EXPORT void ObjCBridge_Startup()
{
    setlinebuf(stdout);
    setlinebuf(stderr);
    stdout->_write = stdout_redirect_nslog;
    stderr->_write = stderr_redirect_nslog;

    // Open the shared memory, with required access mode and permissions.
    // This is analagous to how we open file with open()
    int fd = shm_open("/tmp/Sim2OpenXR_shmem", O_CREAT | O_RDWR /* open flags */, S_IRUSR | S_IWUSR /* mode */);

    // extend or shrink to the required size (specified in bytes)
    ftruncate(fd, 1048576); // picking 1MB size as an example

    // map the shared memory to an address in the virtual address space
    // with the file descriptor of the shared memory and necessary protection.
    // The flags to mmap must be set to MAP_SHARED for other process to access.
    shm_addr = mmap(NULL, 1048576, PROT_READ | PROT_WRITE /*protection*/, MAP_SHARED /*flags*/, fd, 0);

    // read from/write to the virtual address backing the shared memory

    *(uint32_t*)shm_addr = 0x1234ABCD;
    printf("SimUI startup: %u %p %x\n", fd, shm_addr, *(uint32_t*)shm_addr);


}

OBJC_EXPORT void ObjCBridge_Shutdown()
{
    // when done, unmap the virtual address mapped to the shared memory
    munmap(shm_addr, 1048576);

    // remove the shared memory when it is no longer needed.
    shm_unlink("/tmp/Sim2OpenXR_shmem");
}

OBJC_EXPORT sharedmem_data* ObjCBridge_Loop()
{
    sharedmem_data* ret = (sharedmem_data*)shm_addr;
    //printf("%f %f %f\n", ret->l_x, ret->l_y, ret->l_z);
    return ret;
}