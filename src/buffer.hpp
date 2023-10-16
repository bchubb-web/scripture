#ifndef Buffer_H
#define Buffer_H



class Buffer {

    public:
        char* b;
        int len;

        Buffer();
        void append(const char *s, int len);
        void free();
};

#endif
