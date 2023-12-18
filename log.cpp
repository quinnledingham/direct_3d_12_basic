enum
{
    OUTPUT_DEFAULT,
    OUTPUT_ERROR,
    OUTPUT_WARNING,
};

static u32 output_type;

#ifdef WINDOWS

void output_string(u32 output_type, const char *string) {
    static char *output_buffer = 0;
    if (output_buffer == 0) {
        output_buffer = (char*)malloc(100);
    }

    for (int i = 0; i < 100; i++) {
        output_buffer[i] = 0;
    }

    int i = 0;
    while(string[i] != 0) {
        output_buffer[i] = string[i];
        i++;
    }

    OutputDebugStringA((LPCSTR)output_buffer);
}

void output_ch(u32 output_type, const char ch) {
    static char *output_buffer = 0;
    if (output_buffer == 0) {
        output_buffer = (char*)malloc(2);
        output_buffer[1] = 0;
    }
    output_buffer[0] = ch;
    OutputDebugStringA((LPCSTR)output_buffer);
}

inline const char *
float_to_char_array(f32 f) {
    u32 size = 64;
    char *buffer = (char*)malloc(size);
    memset(buffer, 0, size);
    u32 ret = snprintf(buffer, size, "%f", f);
    if (ret < 0) {
        error(0, "ftos(): failed");
        return 0;
    }
    //if (ret >= size) warning(0, "ftos(): result was truncated");
    return buffer;
}

void output_list(u32 output_type, const char *msg, va_list valist) {
    const char *msg_ptr = msg;
    while (*msg_ptr != 0) {
        if (*msg_ptr == '%') {
            msg_ptr++;
            switch(*msg_ptr) {
                case 's': {
                    const char *string = va_arg(valist, const char*);
                    output_string(output_type, string);
                } break;
                case 'f': {
                    double f = va_arg(valist, double);
                    const char *f_string = float_to_char_array((float)f);
                    output_string(output_type, f_string);
                    free((void*)f_string);
                };
            }
        } else {
            output_ch(output_type, *msg_ptr);
        }
        msg_ptr++;
    }
}

#endif // WINDOWS

#ifdef SDL

void output(FILE *stream, const char* msg, va_list valist)
{
    const char *msg_ptr = msg;
    while (*msg_ptr != 0) {
        if (*msg_ptr == '%') {
            msg_ptr++;
            if (*msg_ptr == 's') {
                const char *string = va_arg(valist, const char*);
                fprintf(stream, "%s", string);
            } else if (*msg_ptr == 'd') {
                int integer = va_arg(valist, int);
                fprintf(stream, "%d", integer);
            } else if (*msg_ptr == 'f') {
                double f = va_arg(valist, double);
                fprintf(stream, "%f", f);
            }
        } else {
            fputc(*msg_ptr, stream);
        }
        msg_ptr++;
    }
}

#endif // SDL

void output(const char *msg, ...) {
    u32 output_type = OUTPUT_DEFAULT;
    va_list list;

    va_start(list, msg);
    output_list(output_type, msg, list);
    va_end(list);

    output_ch(output_type, '\n');
}

void error(int line_num, const char* msg, ...)
{
    u32 output_type = OUTPUT_ERROR;

    output_string(output_type, "error: ");
    
    va_list valist;
    va_start(valist, msg);
    output_list(output_type, msg, valist);
    
    if (line_num != 0) fprintf(stderr, " @ or near line %d ", line_num);
    output_ch(output_type, '\n');
}

#ifdef OPENGL

void GLAPIENTRY opengl_debug_message_callback(GLenum source, GLenum type, GLuint id,  GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    log("GL CALLBACK:");
    log("message: %s\n", message);
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               log("type: ERROR");               break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: log("type: DEPRECATED_BEHAVIOR"); break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  log("type: UNDEFINED_BEHAVIOR");  break;
        case GL_DEBUG_TYPE_PORTABILITY:         log("type: PORTABILITY");         break;
        case GL_DEBUG_TYPE_PERFORMANCE:         log("type: PERFORMANCE");         break;
        case GL_DEBUG_TYPE_OTHER:               log("type: OTHER");               break;
    }
    log("id: %d", id);
    switch(severity)
    {
        case GL_DEBUG_SEVERITY_LOW:    log("severity: LOW");    break;
        case GL_DEBUG_SEVERITY_MEDIUM: log("severity: MEDIUM"); break;
        case GL_DEBUG_SEVERITY_HIGH:   log("severity: HIGH");   break;
    }
}

void opengl_debug(int type, int id)
{
    GLint length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
    if (length > 0)
    {
        GLchar info_log[512];
        GLint size;
        
        switch(type)
        {
            case GL_SHADER:  glGetShaderInfoLog (id, 512, &size, info_log); break;
            case GL_PROGRAM: glGetProgramInfoLog(id, 512, &size, info_log); break;
        }
        
        log(info_log);
    }
}

#endif // OPENGL