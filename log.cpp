#ifdef WINDOWS

void output_string(const char *string) {
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

void output_ch(const char ch) {
    static char *output_buffer = 0;
    if (output_buffer == 0) {
        output_buffer = (char*)malloc(2);
        output_buffer[1] = 0;
    }
    output_buffer[0] = ch;
    OutputDebugStringA((LPCSTR)output_buffer);
}

void output_list(const char *msg, va_list valist) {
    const char *msg_ptr = msg;
    while (*msg_ptr != 0) {
        if (*msg_ptr == '%') {
            msg_ptr++;
            switch(*msg_ptr) {
                case 's': {
                    const char *string = va_arg(valist, const char*);
                    output_string(string);
                } break;
            }
        } else {
            output_ch(*msg_ptr);
        }
        msg_ptr++;
    }
}

void log(const char* msg, ...)
{
    va_list valist;
    va_start(valist, msg);
    output_list(msg, valist);
    output_ch('\n');
}

void error(int line_num, const char* msg, ...)
{
    output_string("error: ");
    
    va_list valist;
    va_start(valist, msg);
    output_list(msg, valist);
    
    //if (line_num != 0) fprintf(stderr, " @ or near line %d ", line_num);
    output_ch('\n');
}

#endif // WINDOWS

#ifdef SDL

void output(FILE *stream, const char* msg, va_list valist)
{
    const char *msg_ptr = msg;
    while (*msg_ptr != 0) {
        if (*msg_ptr == '%') {
            msg_ptr++;
            if (*msg_ptr == 's')
            {
                const char *string = va_arg(valist, const char*);
                fprintf(stream, "%s", string);
            }
            else if (*msg_ptr == 'd')
            {
                int integer = va_arg(valist, int);
                fprintf(stream, "%d", integer);
            }
            else if (*msg_ptr == 'f')
            {
                double f = va_arg(valist, double);
                fprintf(stream, "%f", f);
            }
        } else {
            fputc(*msg_ptr, stream);
        }
        msg_ptr++;
    }
}

void log(const char* msg, ...)
{
    va_list valist;
    va_start(valist, msg);
    output(stdout, msg, valist);
    fputc('\n', stdout);
}

void error(int line_num, const char* msg, ...)
{
    output(stderr, "error: ", 0);
    
    va_list valist;
    va_start(valist, msg);
    output(stderr, msg, valist);
    
    if (line_num != 0) fprintf(stderr, " @ or near line %d ", line_num);
    fputc('\n', stderr);
}

void error(const char* msg, ...) 
{ 
    fprintf(stderr, "error: ");
    va_list valist;
    va_start(valist, msg);
    output(stderr, msg, valist);
    fputc('\n', stderr);
}

void warning(int line_num, const char* msg, ...)
{
    fprintf(stderr, "warning: ");
    
    va_list valist;
    va_start(valist, msg);
    output(stderr, msg, valist);
    
    if (line_num != 0) fprintf(stderr, "@ or near line %d ", line_num);
    fputc('\n', stderr);
}

#endif // SDL

#ifdef OPENGL

void GLAPIENTRY opengl_debug_message_callback(
    GLenum source, GLenum type, GLuint id,  GLenum severity, GLsizei length, 
    const GLchar* message, const void* userParam )
{
    SDL_Log("GL CALLBACK:");
    SDL_Log("message: %s\n", message);
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