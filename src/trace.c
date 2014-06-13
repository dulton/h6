#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "trace.h"

#define TRACE_MAX_BUF  (4 * 1024)

#define print_err(err, name) \
do {\
	int error = (err); 							\
	if (error)	 		 		 			\
		fprintf(stderr, "file %s: line %d (%s): error '%d' during '%s'\r\n",	\
           __FILE__, __LINE__, __FUNCTION__, err, name);				\
} while (0)

static uint32_t
async_ring_buf_get_free_space(async_ring_buf_t *buf)
{
    uint32_t space = 0;

    if (buf->read_reverse == buf->write_reverse)
        space = buf->size - buf->write_pos + buf->read_pos;
    else if ((buf->write_reverse - buf->read_reverse) == 1)
        space = buf->read_pos - buf->write_pos;
        
    return space;
}

static uint32_t
async_ring_buf_get_used_space(async_ring_buf_t *buf)
{
    int datasize = 0;

    if (buf->read_reverse == buf->write_reverse)
        datasize = buf->write_pos - buf->read_pos;
    else if ((buf->write_reverse - buf->read_reverse) == 1)
        datasize = buf->size - buf->read_pos + buf->write_pos;
    else
        datasize = buf->size;
    
    return datasize;    
}


static uint32_t
async_ring_buf_read(async_ring_buf_t *buf, char *dest, size_t len)
{
    uint32_t readbytes, usedbytes;

    assert(buf);
    if (buf->read_reverse == buf->write_reverse)
    {
        usedbytes = buf->write_pos - buf->read_pos;
        readbytes = (len <= usedbytes) ? len : usedbytes;
        strncpy(dest, buf->start + buf->read_pos, readbytes);
    }
    else if ((buf->write_reverse - buf->read_reverse) == 1)
    {
        if (len <= (buf->size - buf->read_pos))
        {
            strncpy(dest, buf->start + buf->read_pos, len);
            readbytes = len;
        }
        else
        {
            usedbytes = buf->size - buf->read_pos + buf->write_pos;
            readbytes = len <= usedbytes ? len : usedbytes;
            
            strncpy(dest, buf->start + buf->read_pos, buf->size - buf->read_pos);
            strncpy(dest + buf->size - buf->read_pos, buf->start, readbytes - buf->size + buf->read_pos);
        }        
    }
    else
    {
        assert(0);
    }

    buf->read_pos += readbytes;
    if (buf->read_pos > buf->size)
    {
        buf->read_pos = buf->read_pos % buf->size;
        buf->read_reverse++;
    }
        
    return readbytes;
}

static void
trace_print_until_zero(async_ring_buf_t *buf)
{
    int     readbytes, cycle;
    char    string[TRACE_LINE_MAX_SIZE];

    readbytes = 0; // initiate every time    
    do
    {
        pthread_mutex_lock(&buf->mutex);
        readbytes = async_ring_buf_get_used_space(buf);
        if (readbytes > 0)
        {
            if (readbytes > sizeof(string) - 1)
            {
                readbytes = sizeof(string) - 1;
                cycle = 1;
            }
            else
                cycle = 0;
    
            readbytes = async_ring_buf_read(buf, string, readbytes);
        }        
        pthread_mutex_unlock(&buf->mutex);
      
        
        if (readbytes > 0)
        {
            string[readbytes] = '\0'; // string terminated with NULL
            fprintf(stdout, "%s", string);
        }
    
    } while (cycle);
    
    return;
}

static void *
trace_thread_worker(void *self)
{
    int     exit_thread;
    async_ring_buf_t *buf = (async_ring_buf_t *)self;

    if (atomic_get(&buf->level) <= TRACE_DETAIL_LEVEL)
        fprintf(stderr, "\r\ntrace thread started...\r\n");

    exit_thread = 0;
    while (!exit_thread)
    {
        assert(buf);        

        sem_wait(buf->sem);
        
        trace_print_until_zero(buf);
        
        exit_thread = atomic_get(&buf->exit);
    }        

    // check whether there are still buffer needing to print
    trace_print_until_zero(buf);
    
    
    if (atomic_get(&buf->level) <= TRACE_DETAIL_LEVEL)
    {
        fprintf(stderr, "\r\ntrace thread exited...\r\n");
    }

    return NULL;
}

async_ring_buf_t*
async_ring_buf_new(uint32_t size)
{
    async_ring_buf_t *buf;
    int result;
    
    buf = (async_ring_buf_t *)calloc(1, sizeof(async_ring_buf_t));
    if (buf == NULL)
        return NULL;

    buf->start = (char *)calloc(size, sizeof(char));
    if (buf->start == NULL)
    {
        free(buf);
        return NULL;
    }
    buf->size = size;

    pthread_mutex_init(&buf->mutex, NULL);

    buf->sem = (sem_t *)malloc(sizeof(sem_t));
    if (buf->sem == NULL)
    {
        free(buf);
        return NULL;
    }    
    sem_init(buf->sem, 0, 0);;
       
    result = pthread_create(&buf->thread, NULL, trace_thread_worker, buf);
    if (result)
    {
        free(buf->start);
        free(buf);
        return NULL;
    }

    atomic_set(&buf->level, TRACE_DETAIL_LEVEL);
    
    return buf;
}

void
async_ring_buf_free(async_ring_buf_t *buf)
{
    assert(buf);

    atomic_set(&buf->exit, 1);
    sem_post(buf->sem);
    
    pthread_join(buf->thread, NULL);
    
    pthread_mutex_destroy(&buf->mutex);    
    sem_destroy(buf->sem);
    free(buf->sem);

    if (buf->start)
        free(buf->start);

    free(buf);
}

uint32_t
async_ring_buf_write(async_ring_buf_t *buf, char *string, size_t len)
{
    uint32_t free_size;
    int32_t sem_value;

    assert(buf);
    assert(len <= TRACE_LINE_MAX_SIZE);

    pthread_mutex_lock(&buf->mutex);
    free_size = async_ring_buf_get_free_space(buf);
    pthread_mutex_unlock(&buf->mutex);
        
    while (free_size < len)
    {
        usleep(10000);   // wait 10 millsecond

        // check whether no semaphore for work thread, obviously if semaphore's
        // value is zero, its free space existe
        sem_getvalue(buf->sem, &sem_value);
        if (sem_value <= 0)
        {
            pthread_mutex_lock(&buf->mutex);
            free_size = async_ring_buf_get_free_space(buf);
            pthread_mutex_unlock(&buf->mutex);
        }
    }

    pthread_mutex_lock(&buf->mutex);
    if(buf->write_pos + len > buf->size)
    {
        memcpy(buf->start + buf->write_pos, string, buf->size - buf->write_pos);
        memcpy(buf->start, string + buf->size - buf->write_pos, 
            len - (buf->size - buf->write_pos));
    }
    else
    {
        memcpy(buf->start + buf->write_pos, string, len);
    }

    buf->write_pos += len;
    if (buf->write_pos > buf->size)
    {
        buf->write_pos = buf->write_pos % buf->size;
        buf->write_reverse++;
    }

    pthread_mutex_unlock(&buf->mutex);
    
    sem_getvalue(buf->sem, &sem_value);
    if (sem_value <= 0)
        sem_post(buf->sem);         

    return free_size;
}

//===========================================================================

static async_ring_buf_t *output;
static char *get_level_str(uint32_t level)
{
    static char *info[5] = {
            "DETAIL: ",
            "DEBUG: ",
            "TRACE: ",
            "WARNING: ",
            "ERROR: "};

    switch(level)
    {
        case TRACE_DETAIL_LEVEL:
            return info[0];            
        case TRACE_DEBUG_LEVEL:
            return info[1];
        case TRACE_TRACE_LEVEL:
            return info[2];
        case TRACE_WARNING_LEVEL:
            return info[3];
        case TRACE_ERROR_LEVEL:
            return info[4];
        default:
            return NULL;
    }

    return NULL;        
}

static uint32_t
get_timestamp_str(int level, char *buf, size_t len)
{
    int             result;
    struct tm       *tmTime;
    struct timeval  timestamp;
    
    if (len < 32)
        return 0;
    
    gettimeofday(&timestamp, NULL);

    tmTime = (struct tm *)malloc(sizeof(struct tm));
    tmTime = localtime_r((time_t *)&timestamp.tv_sec, tmTime);
    
    result = sprintf(buf, "%02d:%02d:%02d.%06ld %s ", 
        tmTime->tm_hour, tmTime->tm_min, tmTime->tm_sec, timestamp.tv_usec,
        get_level_str(level));

    free(tmTime);

    return result;
}

int
async_trace_init(void)
{
    output = async_ring_buf_new(TRACE_MAX_BUF);
    if (output == NULL)
        return -1;

    return 0;
}

void
async_trace_destroy(void)
{
    if (output)
        async_ring_buf_free(output);
}

static void
trace_args(int level, const char *format, va_list args)
{
    char string[TRACE_LINE_MAX_SIZE];
    int  len, result;
    
    if (output == NULL)
        return;
  
    len = get_timestamp_str(level, string, sizeof(string));
    if (len <= 0)
        return;       

    result = vsnprintf(string+len, (sizeof(string) - len), format, args);
    if (result > (sizeof(string) - len))
        result = sizeof(string) - len;

    len += result;
    if (len > 0)
        async_ring_buf_write(output, string, len);

    return;
}

void
trace(int level, const char *format, ...)
{
    va_list args;

    if (output == NULL)
    {
        fprintf(stderr, "You should invoke async_trace_init firstly before invoking function trace.\r\n");
    	return;
    }

    if (level < atomic_get(&output->level))
        return;

    va_start(args, format);
    trace_args(level, format, args);
    va_end(args);    
}

void
trace_dump(int level, char *buffer, size_t size)
{
    int           i;      // used to keep track of line lengths
    char *line;  // used to print char version of data
    char ch;     // also used to print char version of data
    
    if (output == NULL)
    {
        fprintf(stderr, "You should invoke async_trace_init firstly before invoking function trace.\r\n");
        return;
    }

    atomic_get(&output->level);        
    if (output->level > level)
      return;
    
    trace(level, "JPU-DUMP: ");
    if (size > 512)
    {
      trace(level, "buffer size is too big(%u bytes), now only display first 512 bytes.", size);
      size = 512;
    }
    trace(level, "\n");
    
    
    i = 0; 
    line = buffer; 
    
    trace(level, "%08X | ", (int)buffer); // print the address we are pulling from
    while (size-- > 0)
    {
      trace(level, "%02X ", *buffer++); // print each char
      if (!(++i % 16) || (size == 0 && i % 16))
      { 
        // if we come to the end of a line...
         
        // if this is the last line, print some fillers.
        if (size == 0)
        {
          while (i++ % 16)
          { 
            trace(level, "__ ");
          }
        }
    
        trace(level, "| ");
          
        while (line < buffer) // print the character version
        {  
          ch = *line++;
          trace(level, "%c", (ch < 33 || ch == 255) ? 0x2E : ch);
        }
          
        // If we are not on the last line, prefix the next line with the address.
        if (size > 0)
        {
          trace(level, "\n%08X | ", (int)buffer);
        }
      }
    }
    trace(level, "\n\n");
    
}

void
trace_adjust(int level)
{
    assert(output);
    
    atomic_set(&output->level, level);

    return;
}

