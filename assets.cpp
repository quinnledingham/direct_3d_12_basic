//
// File
//

function File
read_file(const char *filename)
{
    File result = {};
    
    FILE *in = fopen(filename, "rb");
    if(in) 
    {
        fseek(in, 0, SEEK_END);
        result.size = ftell(in);
        fseek(in, 0, SEEK_SET);
        
        result.memory = malloc(result.size);
        fread(result.memory, result.size, 1, in);
        fclose(in);
    }
    else error(0, "Cannot open file %s", filename);
    
    return result;
}

// returns the file with a 0 at the end of the memory.
// useful if you want to read the file like a string immediately.
function File
read_file_terminated(const char *filename) {
    File result = {};
    File file = read_file(filename);

    //result = file;
    result.size = file.size + 1;
    result.path = filename;
    result.memory = malloc(result.size);
    memcpy(result.memory, file.memory, file.size);

    char *r = (char*)result.memory;
    r[file.size] = 0; // last byte in result.memory
    
    return result;
}