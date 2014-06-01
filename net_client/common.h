

//#define _DEBUG

#ifdef _DEBUG

#define NEW new(__FILE__, __LINE__)
inline void * operator new(size_t size, const char *file, int line)
{
    void *ptr = (void *)malloc(size);
    dyc::addTrack(ptr, size, file, line);
    return(ptr);
}

inline void operator delete(void *p, int)
{
    if (dyc::RemoveTrack(p)) {
//        printf("call free %p\n", p);
        free(p);
    }
}

inline void * operator new[](size_t size, const char *file, int line)
{
    void *ptr = (void *)malloc(size);
    dyc::addTrack(ptr, size, file, line);
    return(ptr);
}

inline void operator delete[](void *p, int)
{
    if (dyc::RemoveTrack(p)) {
//        printf("call delete[]\n");
        free((void*)p);
    }
}
#else  // _DEBUG
#define NEW new(std::nothrow)
#endif // _DEBUG


#define DELETE(pointer) \
    do { \
        if ((pointer) != NULL) { \
            operator delete (((void*)pointer), 0); \
            (pointer) = NULL; \
        } \
    } while (0)

#define DELETES(pointer) \
    do { \
        if ((pointer) != NULL) { \
            operator delete[] (((void*)pointer), 0); \
            (pointer) = NULL; \
        } \
    } while (0)


