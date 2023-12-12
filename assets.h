struct File
{
    u32 size;
    const char *path;
    const char *ch; // for functions like get_char(File *file);
    void *memory;
};