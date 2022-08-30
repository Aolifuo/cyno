#ifndef CYNO_PIMPL_H_
#define CYNO_PIMPL_H_

#define CLASS_PIMPL_DECLARE(CLASS) \
public: \
    CLASS(CLASS &&); \
    CLASS& operator=(CLASS &&); \
    ~CLASS(); \
private: \
    struct Data; \
    CLASS(Data*); \
    Data* data; \

#define CLASS_PIMPL_IMPLEMENT(CLASS) \
CLASS::CLASS(Data* d) { \
    data = d; \
} \
CLASS::CLASS(CLASS&& other) { \
    data = other.data; \
    other.data = nullptr; \
} \
CLASS& CLASS::operator=(CLASS&& other) { \
    data = other.data; \
    other.data = nullptr; \
    return *this; \
} \
CLASS::~CLASS() { \
    if (data) { \
        delete data; \
    } \
}

#endif