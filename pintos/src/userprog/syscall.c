#include "userprog/syscall.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"

#define arg0(STRUCT, ESP) ( *( (STRUCT *)(ESP + 4) ) )
#define arg1(STRUCT, ESP) ( *( (STRUCT *)(ESP + 8) ) )
#define arg2(STRUCT, ESP) ( *( (STRUCT *)(ESP + 12) ) )
#define MAX_FILENAME_LENGTH 100

static void syscall_handler (struct intr_frame *);
static bool check_valid_args(uint8_t* status);
static bool is_valid_ptr(uint8_t* ptr);
static bool is_string_in_bounds(const char* str, int maxlength);


void
syscall_init (void) 
{
    lock_init(&file_lock);
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static bool
check_valid_args(uint8_t *status)
{   
    switch (*status)
    {
        // 1 arg
        case SYS_EXIT:
        case SYS_EXEC:
        case SYS_WAIT:
        case SYS_REMOVE:
        case SYS_OPEN:
        case SYS_FILESIZE:
        case SYS_TELL:
        case SYS_CLOSE:
            return is_valid_ptr(status + sizeof(int));
            break;

        // 2 arg
        case SYS_CREATE:
        case SYS_SEEK:
            return is_valid_ptr(status + (sizeof(int) * 2));
            break;

        // 3 arg
        case SYS_READ:
        case SYS_WRITE:
            return is_valid_ptr(status + (sizeof(int) * 3));
            break;

        default:
            return false;
            break;
    }
}

static bool
is_valid_ptr(uint8_t *ptr)
{
    if ( ptr >= PHYS_BASE || pagedir_get_page( thread_current()->pagedir, ptr) == NULL)
    {
        return false;
    }
    return true;
}

static bool
is_string_in_bounds(const char* str, int maxlength)
{
    int i = 0;    
    if (maxlength != 0)
    {
        while (str[i] != '\0')
        {
            // string too long
            if (i >= maxlength)
            {
                return false;
            }
            i++;
        }        
    }
    else
    {
        while (str[i] != '\0')
        {
            // string too long
            if (is_valid_ptr(str + i) == false)
            {
                return false;
            }
            i++;
        }        
    }
    return is_valid_ptr(str + i);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{    
    uint8_t* status = (uint8_t*) f->esp; // Get callers first argument from stack pointer
    if ( is_valid_ptr(status) == false || check_valid_args(status) == false )
    {
        exit(-1);
    }
    

    // TODO Handle cases
    switch (*status)
    {
        case SYS_HALT:
            halt ();
            break;

        case SYS_EXIT:
            exit ( arg0(int, status) );
            break;

        case SYS_EXEC:
            f->eax = exec ( arg0(char*, status) );
            break;

        case SYS_WAIT:
            f->eax = wait ( arg0(pid_t, status) );
            break;

        case SYS_CREATE:
            f->eax = create ( arg0(char*, status), arg1(unsigned int, status) );
            break;

        case SYS_REMOVE:
            f->eax = remove ( arg0(char*, status) );
            break;

        case SYS_OPEN:
            f->eax = open ( arg0(char*, status) );
            break;

        case SYS_FILESIZE:
            f->eax = filesize ( arg0(int, status) );
            break;

        case SYS_READ:
            f->eax = read ( arg0(int, status), arg1(void*, status), arg2(unsigned int, status) );
            break;

        case SYS_WRITE:            
            f->eax = write ( arg0(int, status), arg1(void*, status), arg2(unsigned int, status) );
            break;

        case SYS_SEEK:
            seek ( arg0(int, status), arg1(unsigned int, status) );
            break;

        case SYS_TELL:
            f->eax = tell ( arg0(int, status) );
            break;

        case SYS_CLOSE:
            close ( arg0(int, status) );
            break;

        default:
            exit(-1);
            break;
    }
}

/*
    Terminates Pintos by calling power_off() (declared in threads/init.h).
    This should be seldom used, because you lose some information about possible deadlock situations, etc.
*/
void
halt(void)
{   
    shutdown_power_off();
}

/*
    Terminates the current user program, returning status to the kernel.
    If the process's parent waits for it (see below), this is the status that will be returned.
    Conventionally, a status of 0 indicates success and nonzero values indicate errors.
*/
void
exit(int status)
{
    printf("%s: exit(%d)\n", thread_current()->name, status);
    *(thread_current()->exit_status) = status;
    thread_exit();
}

/*
    Runs the executable whose name is given in cmd_line, passing any given arguments,
    and returns the new process's program id (pid). Must return pid -1,
    which otherwise should not be a valid pid, if the program cannot load or run for any reason.
    Thus, the parent process cannot return from the exec until
    it knows whether the child process successfully loaded its executable.
    You must use appropriate synchronization to ensure this.
*/
pid_t
exec(const char* file)
{
    if (is_valid_ptr(file) == false )
    {
        exit(-1);
    }
    
    pid_t newProcess_tid = process_execute(file);
    
    return newProcess_tid;
}

/*
    Waits for a child process pid and retrieves the child's exit status.
    If pid is still alive, waits until it terminates.
    Then, returns the status that pid passed to exit.
    If pid did not call exit(), but was terminated by the kernel (e.g. killed due to an exception),
    wait(pid) must return -1. It is perfectly legal for a parent process to wait for child processes
    that have already terminated by the time the parent calls wait, but the kernel must still allow
    the parent to retrieve its child's exit status, or learn that the child was terminated by the kernel.

    wait must fail and return -1 immediately if any of the following conditions is true:

    pid does not refer to a direct child of the calling process.
    pid is a direct child of the calling process if and only if the calling process received pid
    as a return value from a successful call to exec.
    Note that children are not inherited: if A spawns child B and B spawns child process C,
    then A cannot wait for C, even if B is dead. A call to wait(C) by process A must fail.
    Similarly, orphaned processes are not assigned to a new parent if their parent process exits before they do.

    The process that calls wait has already called wait on pid. That is, a process may wait for any given child at most once.
    Processes may spawn any number of children, wait for them in any order,
    and may even exit without having waited for some or all of their children.
    Your design should consider all the ways in which waits can occur.
    All of a process's resources, including its struct thread, must be freed whether its parent ever waits for it or not,
    and regardless of whether the child exits before or after its parent.

    You must ensure that Pintos does not terminate until the initial process exits.
    The supplied Pintos code tries to do this by calling process_wait() (in userprog/process.c) from main() (in threads/init.c).
    We suggest that you implement process_wait() according to the comment at the top of the function
    and then implement the wait system call in terms of process_wait().

    Implementing this system call requires considerably more work than any of the rest.
*/
int
wait(pid_t pid)
{
    return process_wait(pid); // TODO
}

/*
    Creates a new file called file initially initial_size bytes in size.
    Returns true if successful, false otherwise. Creating a new file does not open it:
    opening the new file is a separate operation which would require a open system call.
*/
bool
create(const char* file, unsigned initial_size)
{
    if (is_valid_ptr(file) == false)
    {
        exit(-1);
    }
    lock_files();
    bool file_creation = filesys_create(file, initial_size);
    unlock_files();
    return file_creation;
}


/*
    Deletes the file called file. Returns true if successful, false otherwise.
    A file may be removed regardless of whether it is open or closed, and removing an open file does not close it.
    See Removing an Open File, for details.
*/
bool
remove(const char* file)
{
    if (is_valid_ptr(file) == false)
    {
        exit(-1);
    }
    return false;
    // TODO
}


/*
    Opens the file called file. Returns a nonnegative integer handle called a "file descriptor" (fd),
    or -1 if the file could not be opened.
    File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is standard input,
    fd 1 (STDOUT_FILENO) is standard output. The open system call will never return either of these file descriptors,
    which are valid as system call arguments only as explicitly described below.

    Each process has an independent set of file descriptors. File descriptors are not inherited by child processes.

    When a single file is opened more than once, whether by a single process or different processes,
    each open returns a new file descriptor. Different file descriptors for a single file are closed independently
    in separate calls to close and they do not share a file position.
*/
int
open(const char* file)
{
    if (is_valid_ptr(file) == false)
    {
        exit(-1);
    }

    /* Took inspiration from: https://github.com/MohamedSamirShabaan/Pintos-Project-2/blob/master/src/userprog/syscall.c */
    lock_files();
    struct file* opened_file = filesys_open( file );
    unlock_files();    
    if (opened_file != NULL)
    {
        struct thread* cur = thread_current();
        struct thread_file_container* file_container = (struct thread_file_container*)malloc(sizeof(struct thread_file_container));        
        file_container->file = opened_file;
        file_container->fid = next_fid(cur);
        lock_thread(cur);
        list_push_back(&cur->my_files, &file_container->elem);
        unlock_thread(cur);
        return file_container->fid;
    }
    else
    {
        return -1;
    }    
}


/*
    Returns the size, in bytes, of the file open as fd.
*/
int
filesize(int fd)
{
    struct thread* t = thread_current();
    if (list_empty(&t->my_files))
    {
        unlock_files();
        return -1;
    }
    struct list_elem *temp;
    for (temp = list_front(&t->my_files); temp != list_end(&t->my_files); temp = list_next(temp))
    {
        struct thread_file_container* f = list_entry(temp, struct thread_file_container, elem);
        if (f->fid == fd)
        {
            unlock_files();
            return file_length(f->file);
        }
    }
    unlock_files();
    return -1;
}


/*
    Reads size bytes from the file open as fd into buffer.
    Returns the number of bytes actually read (0 at end of file),
    or -1 if the file could not be read (due to a condition other than end of file).
    Fd 0 reads from the keyboard using input_getc().
*/
int
read(int fd, void* buffer, unsigned size)
{
    if (is_valid_ptr(buffer) == false )
    {
        exit(-1);
    }
    struct list_elem* temp;
    int bytes = -1;    
    if (size <= 0)
    {
        return bytes;
    }
    if (fd == STDIN_FILENO)
    {        
        lock_files();
        bytes = input_getc();        
        unlock_files();
    }
    else if (fd == STDOUT_FILENO || list_empty(&thread_current()->my_files))
    {
        bytes = 0;
    }
    else
    {
        for (temp = list_front(&thread_current()->my_files); temp != list_end(&thread_current()->my_files); temp = list_next(temp))
        {
            struct thread_file_container* f = list_entry(temp, struct thread_file_container, elem);

            if (f->fid == fd)
            {
                lock_files();
                bytes = file_read(f->file, buffer, size);
                unlock_files();
            }
        }
    }    

    

    return bytes;
}


/*
    Writes size bytes from buffer to the open file fd. Returns the number of bytes actually written,
    which may be less than size if some bytes could not be written.
    Writing past end-of-file would normally extend the file, but file growth is not implemented by the basic file system.
    The expected behavior is to write as many bytes as possible up to end-of-file and return the actual number written,
    or 0 if no bytes could be written at all.

    Fd 1 writes to the console. Your code to write to the console should write all of buffer in one call to putbuf(),
    at least as long as size is not bigger than a few hundred bytes.
    (It is reasonable to break up larger buffers.)
    Otherwise, lines of text output by different processes may end up interleaved on the console,
    confusing both human readers and our grading scripts.
*/
int
write(int fd, const void* buffer, unsigned size)
{
    if (is_valid_ptr(buffer) == false)
    {
        exit(-1);
    }
    #define MAX_CONSOLE_SIZE 200
    struct thread* cur = thread_current();
    int bytes_written = 0;    
    // Write to the console
    if (fd == STDOUT_FILENO)
    {
        int bufferOffset = 0;        
        lock_files();        
        // Break into chunks of 200
        while (bufferOffset < size)
        {
            if ( (size - bufferOffset) > MAX_CONSOLE_SIZE)
            {
                putbuf( (buffer + bufferOffset), MAX_CONSOLE_SIZE );
            }
            else
            {
                putbuf( (buffer + bufferOffset), (size - bufferOffset) );
            }            
            bufferOffset += MAX_CONSOLE_SIZE;
        }
        bytes_written = size;        
        unlock_files();
    }
    else if(fd > STDOUT_FILENO && list_empty(&cur->my_files) == false)
    {       
        struct thread_file_container *file_container;
        struct list_elem *file_elem = list_front(&cur->my_files);
        bool found = false;
        while (found == false && file_elem != list_end(&cur->my_files))
        {
            file_container = list_entry(file_elem, struct thread_file_container, elem);
            if (file_container->fid == fd)
            {
                found = true;
            }
            file_elem = list_next(file_elem);
        }
        if (found == true)
        {
            ASSERT(file_container->fid == fd); // Sanity check;
            lock_files();
            bytes_written = file_write(file_container->file, buffer, size);
            unlock_files();
        }
    }
    return bytes_written;
}

/*
    Changes the next byte to be read or written in open file fd to position,
    expressed in bytes from the beginning of the file. (Thus, a position of 0 is the file's start.)
    A seek past the current end of a file is not an error. A later read obtains 0 bytes, indicating end of file.
    A later write extends the file, filling any unwritten gap with zeros.
    (However, in Pintos files have a fixed length until project 4 is complete, so writes past end of file will return an error.)
    These semantics are implemented in the file system and do not require any special effort in system call implementation.
*/
void
seek(int fd, unsigned position)
{
    struct list_elem* temp;

    lock_files();

    if (list_empty(&thread_current()->my_files) == false)
    {
        for (temp = list_front(&thread_current()->my_files); temp != list_end(&thread_current()->my_files); temp = list_next(temp))
        {
            struct thread_file_container* f = list_entry(temp, struct thread_file_container, elem);
            if (f->fid == fd)
            {
                file_seek(f->file, position);                
                break;
            }
        }
    }

    unlock_files();    
}

/*
    Returns the position of the next byte to be read or written in open file fd, expressed in bytes from the beginning of the file.
*/
unsigned
tell(int fd)
{
    struct thread* t = thread_current();
    struct list_elem *item;
    struct thread_file_container* file;
    bool found = false;
    unsigned pos = 0;
    if (list_empty(&t->my_files))
    {
        return pos;
    }
    for (item = list_begin(&t->my_files); item != list_end(&t->my_files) && found == false; item = list_next(item))
    {
        file = list_entry(item, struct thread_file_container, elem);
        if (file->fid == fd)
        {
            bool found = true;
        }
    }

    if (found == true)
    {
        ASSERT(file->fid == fd) // sanity check
        lock_files();
        pos = file_tell(file->file);
        unlock_files();
    }
    return pos;    
}

/*
    Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open file descriptors,
    as if by calling this function for each one.
*/
void
close(int fd)
{
    struct thread* cur = thread_current();
    if (fd > STDOUT_FILENO && list_empty(&cur->my_files) == false)
    {
        
        struct thread_file_container* file_container;
        struct list_elem* file_elem = list_front(&cur->my_files);
        bool found = false;
        while (found == false && file_elem != list_end(&cur->my_files))
        {
            file_container = list_entry(file_elem, struct thread_file_container, elem);
            if (file_container->fid == fd)
            {
                found = true;
            }
            else
            {
                file_elem = list_next(file_elem);
            }            
        }
        if (found == true)
        {
            ASSERT(file_container->fid == fd); // Sanity check;
            lock_files();
            file_close(file_container->file);
            unlock_files();
            lock_thread(cur);
            list_remove(file_elem);
            unlock_thread(cur);
            free(file_container);
        }
    }
}